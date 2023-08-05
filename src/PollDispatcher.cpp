#include "Dispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "PollDispatcher.h"


// 初始化PollDispatcher实例并将该子类的evLoop传递给父类，父类调用构造函数保存evLoop的地址
PollDispatcher::PollDispatcher(EventLoop* evLoop):Dispatcher(evLoop) {
	m_maxfd = 0;
	m_fds = new struct pollfd[m_maxNode];
	for (int i = 0; i < 1024; ++i) {
		m_fds[i].fd = -1;
		m_fds[i].events = 0;
		m_fds[i].revents = 0;
	}
	m_name = "Poll";
}

PollDispatcher::~PollDispatcher(){
	delete[]m_fds;
}

int PollDispatcher::add()
{
	short int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) {
		events |= POLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) {
		events |= POLLOUT;
	}
	int i = 0;
	while (i < m_maxNode) {
		if (-1 == m_fds[i].fd) {
			m_fds[i].events = events;
			m_fds[i].fd = m_channel->getSocket();
			m_maxfd = i > m_maxfd ? i : m_maxfd;
			break;
		}
		++i;
	}
	if (i >= m_maxNode) {
		return -1;
	}
	return 0;
}

int PollDispatcher::remove()
{
	int i = 0;
	while (i < m_maxNode) {
		if (m_channel->getSocket() == m_fds[i].fd) {
			m_fds[i].events = 0;
			m_fds[i].revents = 0;
			m_fds[i].fd = -1;
			break;
		}
		++i;
	}
	// 通过channel释放对应的TcpConnection资源
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg()));
	if (i >= m_maxNode) {
		return -1;
	}
	return 0;
}

int PollDispatcher::modify()
{
	short int events = 0;
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent) {
		events |= POLLIN;
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent) {
		events |= POLLOUT;
	}
	int i = 0;
	while (i < m_maxNode) {
		if (m_channel->getSocket() == m_fds[i].fd) {
			m_fds[i].events = events;
			break;
		}
		++i;
	}
	if (i >= m_maxNode) {
		return -1;
	}
	return 0;
}

// fd的事件检测（读写事件）
int PollDispatcher::dispatch(int timeout)
{
	int count = poll(m_fds, m_maxfd + 1, timeout * 1000);
	if (-1 == count) {
		perror("poll");
		exit(0);
	}
	for (int i = 0; i <= m_maxfd; ++i) {
		if (-1 == m_fds[i].fd) {
			continue;
		}
		if (m_fds[i].revents & POLLIN) {
			m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::ReadEvent);
		}
		if (m_fds[i].revents & POLLOUT) {
			m_evLoop->eventActivate(m_fds[i].fd, (int)FDEvent::WriteEvent);
		}
	}
	return 0;
}
