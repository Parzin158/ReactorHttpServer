#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>

using namespace std;

// 处理节点中channel的方式
enum class ElemType:char{ ADD, DELETE, MODIFY };

// 定义任务队列的结点类型：由type和channel组成
struct ChannelElement {
	ElemType type;		// 处理节点中channel的方式
	Channel* channel;
};

class Dispatcher; // 解决EventLoop.h和Dispatcher.h重复包含问题

// 每次EventLoop对应一次dispatcher
class EventLoop {

public:
	EventLoop(); // 主线程
	EventLoop(const string threadName); // 子线程
	~EventLoop();

	// 启动反应堆模型
	int run();
	// 处理被激活的fd
	int eventActivate(int fd, int event);
	// 添加节点到反应堆实例的任务队列中
	int addTask(struct Channel* channel, ElemType type);
	// 处理任务队列的任务
	int processTaskQ();
	// 处理diapatcher中的节点
	int add(Channel* channel);
	int remove(Channel* channel);
	int modify(Channel* channel);
	// 释放channel
	int freeChannel(Channel* channel); 
	// 读数据
	static int readLocalMessage(void* arg); 
	// 该函数作为Channel的初始化参数，在EventLoop类内需声明为静态成员函数，以便EventLoop类未实例化时给Channel使用
	// 类内的静态成员函数不属于某个对象，只要类有定义，该函数就存在
	int readMessage(); // 另一种实现方式
	// 获取所属线程ID
	inline thread::id getThreadID() {
		return m_threadID;
	}

private:
	// 写数据
	void taskWakeup();

private:
	// EventLoop开关
	bool m_isQuit;
	// 该指针指向子类的实例 epoll, poll, select
	Dispatcher* m_dispatcher; 
	// 任务队列：链表，节点类型为ChannelElement结构体
	queue<ChannelElement*> m_taskQ;
	// 存储fd和channel的映射关系
	map<int, Channel*> m_channelMap;  // key--int, value--channel
	// 线程id
	thread::id m_threadID;  
	// 线程名
	string m_threadName;	
	// 互斥锁，用来保护任务队列
	mutex m_mutex; 
	// 存储本地通信的fd，通过socketPair函数初始化
	int m_socketPair[2]; 
};


