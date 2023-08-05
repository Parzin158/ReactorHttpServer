#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include <string>
#include <sys/select.h>

using namespace std;

class SelectDispatcher : public Dispatcher {
public:
	// ȫ�ֱ���SelectDispatcher����EventLoop��ʹ��
// ��ʼ��ʵ������Dispatcherָ��Ϊselect���ͣ����������������ָ��
	// ��ʼ��IO����ģ��ʹ�õ����ݿ飬����ֵ��������ݲ�ͬ�����ݿ�
	// epoll(epoll��)��poll(pollfd�ṹ������)��select(fd_set����)
	// ��ʼ��selectģ��ʹ�õ����ݿ飬����ֵ��������ݲ�ͬ�����ݿ顣 select(fd_set����)

	SelectDispatcher(EventLoop* evLoop);
	~SelectDispatcher();

	// fd��ӵ�IO����ģ����
	int add() override;
	// ɾ��fd
	int remove() override;
	// �޸�fd
	int modify() override;
	// fd���¼���⣨��д�¼���
	int dispatch(int timeout = 2) override; // ��ʱʱ��2s

private:
	// ����channel�¼����ͣ���fd��ӵ���/д������
	void setFdSet();
	// ����channel�¼����ͣ���fd�Ӷ�/д������ɾ��
	void clearFdSet();


private:
	fd_set m_readSet; // ������
	fd_set m_writeSet; // д����
	// �����쳣����
	const int m_maxSize = 1024;
	// int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
};

