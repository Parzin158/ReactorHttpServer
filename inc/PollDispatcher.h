#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <poll.h>

using namespace std;

class PollDispatcher : public Dispatcher {
public:
	// ȫ�ֱ���EpollDispatcher����EventLoop��ʹ��
	// ��ʼ��ʵ������Dispatcherָ��Ϊpoll���ͣ����������������ָ��
	// ��ʼ��IO����ģ��ʹ�õ����ݿ飬����ֵ��������ݲ�ͬ�����ݿ飬poll(pollfd�ṹ������)
	PollDispatcher(EventLoop* evLoop);
	~PollDispatcher();

	// fd��ӵ�IO����ģ����
	int add() override;
	// ɾ��fd
	int remove() override;
	// �޸�fd
	int modify() override;
	// fd���¼���⣨��д�¼���
	int dispatch(int timeout = 2) override; // ��ʱʱ��2s

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