#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>

using namespace std;

class EpollDispatcher : public Dispatcher {
public:
	// 初始化IO复用模型使用的数据块，返回值类型需兼容不同的数据块
	// epoll(epoll树)，poll(pollfd结构体数组)，select(fd_set集合)
	EpollDispatcher(EventLoop* evLoop);
	 ~EpollDispatcher();

	// fd添加到IO复用模型中
	int add() override;
	// 删除fd
	int remove() override;
	// 修改fd
	int modify() override;
	// fd的事件检测（读写事件）
	int dispatch(int timeout = 2) override; // 超时时长2s

private:
	// 实现对fd的添加、删除和修改操作
	int epollCtl(int op);

private:
	int m_epfd;
	struct epoll_event* m_events;
	const int m_maxNode = 520;
};


