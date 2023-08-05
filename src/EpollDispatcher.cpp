#include "Dispatcher.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "EpollDispatcher.h"
#include <iostream>


// 初始化EpollDispatcher实例并将该子类的evLoop传递给父类，父类调用构造函数保存evLoop的地址
EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop) {
	m_epfd = epoll_create(10);
	if (-1 == m_epfd) {
		perror("epoll_create");
		exit(0);
	}
	m_events = new struct epoll_event[m_maxNode]; // 重新分配内存并初始化为0
	m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher(){
	close(m_epfd);
	delete[]m_events;
}

int EpollDispatcher::add()
{
	int ret = epollCtl(EPOLL_CTL_ADD);
	if (-1 == ret) {
		perror("epoll_ctl add");
		exit(0);
	}
	return ret;
}

int EpollDispatcher::remove()
{
	int ret = epollCtl(EPOLL_CTL_DEL);
	if (-1 == ret) {
		perror("epoll_ctl delete");
		exit(0);
	}
	// 通过m_channel释放对应的TcpConnection资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg())); // 需去除getArg()的只读属性
	cout << "delete fd" << endl;
	return ret;
}

int EpollDispatcher::modify()
{
	int ret = epollCtl(EPOLL_CTL_MOD);
	if (-1 == ret) {
		perror("epoll_ctl modify");
		exit(0);
	}
	return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
	int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);
	for (int i = 0; i < count; ++i) {
		int events = m_events[i].events;
		int fd = m_events[i].data.fd;
		cout << "get fd: " << fd << " events: " << events << endl;
		if (events & EPOLLERR || events & EPOLLHUP) {
			// EPOLLERR：对端断开连接异常；  EPOLLHUP：断开后仍和对端通信异常
			// 出现两个异常，删除fd，remove();
			continue;
		}
		if (events & EPOLLIN) {
			m_evLoop->eventActivate(fd, (int)FDEvent::ReadEvent);
		}
		if (events & EPOLLOUT) {
			m_evLoop->eventActivate(fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}

int EpollDispatcher::epollCtl(int op){
	struct epoll_event ev;
	ev.data.fd = m_channel->getSocket();
	int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) {
		events |= EPOLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) {
		events |= EPOLLOUT;
	}
	// 通过channel中event的标记，给epoll_ctl函数对应的操作值
	ev.events = events;
	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}
