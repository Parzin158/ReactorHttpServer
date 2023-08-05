#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "SelectDispatcher.h"
#include "PollDispatcher.h"
#include "EpollDispatcher.h"
#include <iostream>


EventLoop::EventLoop():EventLoop(string()){
	// 委托构造
}

EventLoop::EventLoop(const string threadName){
	m_isQuit = true; // 默认关闭
	m_threadID = this_thread::get_id(); // 获取当前线程ID
	m_threadName = threadName == string() ? "MainThread" : threadName;  
	cout << m_threadName << ":" << m_threadID << endl;
	m_dispatcher = new EpollDispatcher(this); // 当前类的实例this作为参数，初始化指定为epoll模型
	m_channelMap.clear();
	// 创建本地通信的fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
	if (-1 == ret) {
		perror("socketpair");
		exit(0);
	}
	// 指定发送规则: m_socketPair[0] 发送数据，m_socketPair[1] 接收数据（添加到dispatcher检测的fd集合中）
	// 通过使用socketPair[0]往socketPair[1]发送数据，解除select/poll/epoll的阻塞
#if 0
	Channel* channel =new Channel(m_socketPair[1], FDEvent::ReadEvent, readLocalMessage, nullptr, nullptr, this);
#else
	// 绑定 - bind（c++11可调用对象绑定器）
	auto obj = bind(&EventLoop::readMessage, this);
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, obj, nullptr, nullptr, this);

#endif
	// channel 添加到任务队列
	addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop(){

}

int EventLoop::run()
{
	m_isQuit = false;
	if (m_threadID != this_thread::get_id()) {
		// 线程ID不一致则出错
		return -1;
	}		
	// 循环进行事件处理
	while (!m_isQuit) {
		m_dispatcher->dispatch(); // 超时时长 2s
		processTaskQ(); // 主线程在任务队列中添加任务节点，通知子线程处理，子线程在解除阻塞后，处理该任务队列
	}
	return 0;
}

int EventLoop::eventActivate(int fd, int event)
{
	if (fd < 0) {
		return -1;
	}
	// 取出channel
	Channel* channel = m_channelMap[fd];
	assert(channel->getSocket() == fd);

	// 判断event是读事件还是写事件，调用对应的回调函数
	if (event & (int)FDEvent::ReadEvent && channel->readCallback) {
		channel->readCallback(const_cast<void*>(channel->getArg()));
	}
	if (event & (int)FDEvent::WriteEvent && channel->writeCallback) {
		channel->writeCallback(const_cast<void*>(channel->getArg()));
	}
	return 0;
}

int EventLoop::addTask(Channel* channel, ElemType type)
{
	// 加互斥锁，添加节点到任务队列中
	m_mutex.lock();
	ChannelElement* node = new ChannelElement;
	node->channel = channel;
	node->type = type;	
	m_taskQ.push(node);	
	m_mutex.unlock();

	// 节点处理
	/*
		假设当前EventLoop反应堆属于子线程：
			1. 添加节点的线程，可能是当前线程也可能是其他线程（主线程）
				1). 修改fd的事件，当前子线程发起并处理
				2). 添加新fd，添加任务节点的操作由主线程发起
			2. 主线程添加的节点，主线程无法处理任务队列，需由当前子线程处理
	*/
	if (m_threadID == this_thread::get_id()) {
		// 当前线程为子线程，遍历链表处理任务
		processTaskQ();
	}
	else {
		// 当前线程为主线程 -- 需通知子线程处理任务队列的任务
		/* 子线程的状态：1.子线程正在工作；
						 2. 子线程被阻塞，则需要用一个特殊的fd，来解除select/poll/epoll的阻塞 */
		taskWakeup();
		// 主线程调用该函数往socketPair[0]写数据，此时channel中的fd，socketPair[1]被激活，
		// 调用readLocalMessage函数，触发读事件，解除阻塞。
		// 在dispatcher函数位置解除阻塞后，调用eventLoopProcessTask()函数，让子线程处理任务队列
	}
	return 0;
}

int EventLoop::processTaskQ()
{
	// 取出头结点，遍历链表
	while (!m_taskQ.empty()) {
		// 加互斥锁，保护共享资源
		m_mutex.lock();
		ChannelElement* node = m_taskQ.front();
		m_taskQ.pop(); // 删除该节点
		m_mutex.unlock();
		Channel* channel = node->channel;
		if (node->type == ElemType::ADD) {
			// 添加
			add(channel);
		}
		else if (node->type == ElemType::DELETE) {
			// 删除
			remove(channel);
		}
		else if (node->type == ElemType::MODIFY) {
			// 修改
			modify(channel);
		}
		delete node;
	}
	return 0;
}

int EventLoop::add(Channel* channel)
{
	// 将任务队列中的节点添加到dispatcher对应的检测集合中
	int fd = channel->getSocket();
	// 找到fd对应数组元素位置，并存储fd:channel对应关系
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		m_channelMap.insert(make_pair(fd, channel));
		m_dispatcher->setChannel(channel); // 动态对象channel更新到dispatcher中
		int ret = m_dispatcher->add(); // 将fd添加到对应的fd检测集合中
		return ret;
	}
	return -1;
}

int EventLoop::remove(Channel* channel)
{
	// 将任务队列中的节点从dispatcher对应的检测集合中删除
	int fd = channel->getSocket();
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		// channelMap中无fd对应channel，则fd不在dispatcher检测的fd集合中
		return -1;
	}
	m_dispatcher->setChannel(channel); // 动态对象channel更新到dispatcher中
	int ret = m_dispatcher->remove();
	return ret;
}

int EventLoop::modify(Channel* channel)
{
	// 修改dispatcher对应的检测集合中的fd
	int fd = channel->getSocket();
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		// channelMap中无fd对应channel，则fd不在dispatcher检测的fd集合中
		return -1;
	}
	m_dispatcher->setChannel(channel); // 动态对象channel更新到dispatcher中
	int ret =m_dispatcher->modify();
	return ret;
}

int EventLoop::freeChannel(Channel* channel){
	// 搜索有无对应的键值对
	auto it = m_channelMap.find(channel->getSocket());
	if (it != m_channelMap.end()) {
		// 找到了fd对应的channel，将该键值对删除
		m_channelMap.erase(it);
		close(channel->getSocket());
		delete channel;
	}
	return 0;
}

int EventLoop::readLocalMessage(void* arg)
{
	EventLoop* evLoop = static_cast<EventLoop*>(arg);
	char buf[256];
	read(evLoop->m_socketPair[1], buf, sizeof(buf));
	return 0;
}
int EventLoop::readMessage()
{
	char buf[256];
	read(m_socketPair[1], buf, sizeof(buf));
	return 0;
}

void EventLoop::taskWakeup()
{
	const char* msg = "1024";
	write(m_socketPair[0], msg, strlen(msg));
}
