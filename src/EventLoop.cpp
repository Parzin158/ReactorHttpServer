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
	// ί�й���
}

EventLoop::EventLoop(const string threadName){
	m_isQuit = true; // Ĭ�Ϲر�
	m_threadID = this_thread::get_id(); // ��ȡ��ǰ�߳�ID
	m_threadName = threadName == string() ? "MainThread" : threadName;  
	cout << m_threadName << ":" << m_threadID << endl;
	m_dispatcher = new EpollDispatcher(this); // ��ǰ���ʵ��this��Ϊ��������ʼ��ָ��Ϊepollģ��
	m_channelMap.clear();
	// ��������ͨ�ŵ�fd
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
	if (-1 == ret) {
		perror("socketpair");
		exit(0);
	}
	// ָ�����͹���: m_socketPair[0] �������ݣ�m_socketPair[1] �������ݣ���ӵ�dispatcher����fd�����У�
	// ͨ��ʹ��socketPair[0]��socketPair[1]�������ݣ����select/poll/epoll������
#if 0
	Channel* channel =new Channel(m_socketPair[1], FDEvent::ReadEvent, readLocalMessage, nullptr, nullptr, this);
#else
	// �� - bind��c++11�ɵ��ö��������
	auto obj = bind(&EventLoop::readMessage, this);
	Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, obj, nullptr, nullptr, this);

#endif
	// channel ��ӵ��������
	addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop(){

}

int EventLoop::run()
{
	m_isQuit = false;
	if (m_threadID != this_thread::get_id()) {
		// �߳�ID��һ�������
		return -1;
	}		
	// ѭ�������¼�����
	while (!m_isQuit) {
		m_dispatcher->dispatch(); // ��ʱʱ�� 2s
		processTaskQ(); // ���߳�������������������ڵ㣬֪ͨ���̴߳������߳��ڽ�������󣬴�����������
	}
	return 0;
}

int EventLoop::eventActivate(int fd, int event)
{
	if (fd < 0) {
		return -1;
	}
	// ȡ��channel
	Channel* channel = m_channelMap[fd];
	assert(channel->getSocket() == fd);

	// �ж�event�Ƕ��¼�����д�¼������ö�Ӧ�Ļص�����
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
	// �ӻ���������ӽڵ㵽���������
	m_mutex.lock();
	ChannelElement* node = new ChannelElement;
	node->channel = channel;
	node->type = type;	
	m_taskQ.push(node);	
	m_mutex.unlock();

	// �ڵ㴦��
	/*
		���赱ǰEventLoop��Ӧ���������̣߳�
			1. ��ӽڵ���̣߳������ǵ�ǰ�߳�Ҳ�����������̣߳����̣߳�
				1). �޸�fd���¼�����ǰ���̷߳��𲢴���
				2). �����fd���������ڵ�Ĳ��������̷߳���
			2. ���߳���ӵĽڵ㣬���߳��޷�����������У����ɵ�ǰ���̴߳���
	*/
	if (m_threadID == this_thread::get_id()) {
		// ��ǰ�߳�Ϊ���̣߳���������������
		processTaskQ();
	}
	else {
		// ��ǰ�߳�Ϊ���߳� -- ��֪ͨ���̴߳���������е�����
		/* ���̵߳�״̬��1.���߳����ڹ�����
						 2. ���̱߳�����������Ҫ��һ�������fd�������select/poll/epoll������ */
		taskWakeup();
		// ���̵߳��øú�����socketPair[0]д���ݣ���ʱchannel�е�fd��socketPair[1]�����
		// ����readLocalMessage�������������¼������������
		// ��dispatcher����λ�ý�������󣬵���eventLoopProcessTask()�����������̴߳����������
	}
	return 0;
}

int EventLoop::processTaskQ()
{
	// ȡ��ͷ��㣬��������
	while (!m_taskQ.empty()) {
		// �ӻ�����������������Դ
		m_mutex.lock();
		ChannelElement* node = m_taskQ.front();
		m_taskQ.pop(); // ɾ���ýڵ�
		m_mutex.unlock();
		Channel* channel = node->channel;
		if (node->type == ElemType::ADD) {
			// ���
			add(channel);
		}
		else if (node->type == ElemType::DELETE) {
			// ɾ��
			remove(channel);
		}
		else if (node->type == ElemType::MODIFY) {
			// �޸�
			modify(channel);
		}
		delete node;
	}
	return 0;
}

int EventLoop::add(Channel* channel)
{
	// ����������еĽڵ���ӵ�dispatcher��Ӧ�ļ�⼯����
	int fd = channel->getSocket();
	// �ҵ�fd��Ӧ����Ԫ��λ�ã����洢fd:channel��Ӧ��ϵ
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		m_channelMap.insert(make_pair(fd, channel));
		m_dispatcher->setChannel(channel); // ��̬����channel���µ�dispatcher��
		int ret = m_dispatcher->add(); // ��fd��ӵ���Ӧ��fd��⼯����
		return ret;
	}
	return -1;
}

int EventLoop::remove(Channel* channel)
{
	// ����������еĽڵ��dispatcher��Ӧ�ļ�⼯����ɾ��
	int fd = channel->getSocket();
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		// channelMap����fd��Ӧchannel����fd����dispatcher����fd������
		return -1;
	}
	m_dispatcher->setChannel(channel); // ��̬����channel���µ�dispatcher��
	int ret = m_dispatcher->remove();
	return ret;
}

int EventLoop::modify(Channel* channel)
{
	// �޸�dispatcher��Ӧ�ļ�⼯���е�fd
	int fd = channel->getSocket();
	if (m_channelMap.find(fd) == m_channelMap.end()) {
		// channelMap����fd��Ӧchannel����fd����dispatcher����fd������
		return -1;
	}
	m_dispatcher->setChannel(channel); // ��̬����channel���µ�dispatcher��
	int ret =m_dispatcher->modify();
	return ret;
}

int EventLoop::freeChannel(Channel* channel){
	// �������޶�Ӧ�ļ�ֵ��
	auto it = m_channelMap.find(channel->getSocket());
	if (it != m_channelMap.end()) {
		// �ҵ���fd��Ӧ��channel�����ü�ֵ��ɾ��
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
