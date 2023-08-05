#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>

using namespace std;

class EventLoop; // 解决EventLoop.h和Dispatcher.h重复包含问题

class Dispatcher {
public:
	// 初始化IO复用模型使用的数据块，返回值类型需兼容不同的数据块
	// epoll(epoll树)，poll(pollfd结构体数组)，select(fd_set集合)
	Dispatcher(EventLoop* evLoop);
	virtual ~Dispatcher();

	// fd添加到IO复用模型中
	virtual int add();
	// 删除fd
	virtual int remove();
	// 修改fd
	virtual int modify();
	// fd的事件检测（读写事件）
	virtual int dispatch(int timeout = 2); // 超时时长2s

	// 更新m_channel
	inline void setChannel(Channel* channel) {
		m_channel = channel;
	}

protected:
	string m_name = string(); // 保存指向对象的名称
	Channel* m_channel;
	EventLoop* m_evLoop;
};


