#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/epoll.h>

using namespace std;

class EpollDispatcher : public Dispatcher {
public:
	// ��ʼ��IO����ģ��ʹ�õ����ݿ飬����ֵ��������ݲ�ͬ�����ݿ�
	// epoll(epoll��)��poll(pollfd�ṹ������)��select(fd_set����)
	EpollDispatcher(EventLoop* evLoop);
	 ~EpollDispatcher();

	// fd��ӵ�IO����ģ����
	int add() override;
	// ɾ��fd
	int remove() override;
	// �޸�fd
	int modify() override;
	// fd���¼���⣨��д�¼���
	int dispatch(int timeout = 2) override; // ��ʱʱ��2s

private:
	// ʵ�ֶ�fd����ӡ�ɾ�����޸Ĳ���
	int epollCtl(int op);

private:
	int m_epfd;
	struct epoll_event* m_events;
	const int m_maxNode = 520;
};


