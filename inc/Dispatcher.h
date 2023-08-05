#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>

using namespace std;

class EventLoop; // ���EventLoop.h��Dispatcher.h�ظ���������

class Dispatcher {
public:
	// ��ʼ��IO����ģ��ʹ�õ����ݿ飬����ֵ��������ݲ�ͬ�����ݿ�
	// epoll(epoll��)��poll(pollfd�ṹ������)��select(fd_set����)
	Dispatcher(EventLoop* evLoop);
	virtual ~Dispatcher();

	// fd��ӵ�IO����ģ����
	virtual int add();
	// ɾ��fd
	virtual int remove();
	// �޸�fd
	virtual int modify();
	// fd���¼���⣨��д�¼���
	virtual int dispatch(int timeout = 2); // ��ʱʱ��2s

	// ����m_channel
	inline void setChannel(Channel* channel) {
		m_channel = channel;
	}

protected:
	string m_name = string(); // ����ָ����������
	Channel* m_channel;
	EventLoop* m_evLoop;
};


