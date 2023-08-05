#include "Dispatcher.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "EpollDispatcher.h"
#include <iostream>


// ��ʼ��EpollDispatcherʵ�������������evLoop���ݸ����࣬������ù��캯������evLoop�ĵ�ַ
EpollDispatcher::EpollDispatcher(EventLoop* evLoop) : Dispatcher(evLoop) {
	m_epfd = epoll_create(10);
	if (-1 == m_epfd) {
		perror("epoll_create");
		exit(0);
	}
	m_events = new struct epoll_event[m_maxNode]; // ���·����ڴ沢��ʼ��Ϊ0
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
	// ͨ��m_channel�ͷŶ�Ӧ��TcpConnection��Դ
	m_channel->destroyCallback(const_cast<void*>(m_channel->getArg())); // ��ȥ��getArg()��ֻ������
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
			// EPOLLERR���Զ˶Ͽ������쳣��  EPOLLHUP���Ͽ����ԺͶԶ�ͨ���쳣
			// ���������쳣��ɾ��fd��remove();
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
	// ͨ��channel��event�ı�ǣ���epoll_ctl������Ӧ�Ĳ���ֵ
	ev.events = events;
	int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
	return ret;
}
