#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/select.h>

using namespace std;

class SelectDispatcher : public Dispatcher {
public:
	// 全局变量SelectDispatcher，在EventLoop中使用
// 初始化实例，将Dispatcher指定为select类型，传入其各个函数的指针
	// 初始化IO复用模型使用的数据块，返回值类型需兼容不同的数据块
	// epoll(epoll树)，poll(pollfd结构体数组)，select(fd_set集合)
	// 初始化select模型使用的数据块，返回值类型需兼容不同的数据块。 select(fd_set集合)

	SelectDispatcher(EventLoop* evLoop);
	~SelectDispatcher();

	// fd添加到IO复用模型中
	int add() override;
	// 删除fd
	int remove() override;
	// 修改fd
	int modify() override;
	// fd的事件检测（读写事件）
	int dispatch(int timeout = 2) override; // 超时时长2s

private:
	// 依据channel事件类型，将fd添加到读/写集合中
	void setFdSet();
	// 依据channel事件类型，将fd从读/写集合中删除
	void clearFdSet();


private:
	fd_set m_readSet; // 读集合
	fd_set m_writeSet; // 写集合
	// 忽略异常集合
	const int m_maxSize = 1024;
	// int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
};

