#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>

using namespace std;

class PollDispatcher : public Dispatcher {
public:
	// 全局变量EpollDispatcher，在EventLoop中使用
	// 初始化实例，将Dispatcher指定为poll类型，传入其各个函数的指针
	// 初始化IO复用模型使用的数据块，返回值类型需兼容不同的数据块，poll(pollfd结构体数组)
	PollDispatcher(EventLoop* evLoop);
	~PollDispatcher();

	// fd添加到IO复用模型中
	int add() override;
	// 删除fd
	int remove() override;
	// 修改fd
	int modify() override;
	// fd的事件检测（读写事件）
	int dispatch(int timeout = 2) override; // 超时时长2s

private:
	int m_maxfd;
	struct pollfd *m_fds;
	const int m_maxNode = 1024;
};


//struct pollfd {
//	int   fd;         /* file descriptor */
//	short events;     /* requested events */
//	short revents;    /* returned events */
//};