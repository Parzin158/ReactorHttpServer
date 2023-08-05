#pragma once
#include "Dispatcher.h"
#include "Channel.h"
#include <thread>
#include <queue>
#include <map>
#include <mutex>

using namespace std;

// ����ڵ���channel�ķ�ʽ
enum class ElemType:char{ ADD, DELETE, MODIFY };

// ����������еĽ�����ͣ���type��channel���
struct ChannelElement {
	ElemType type;		// ����ڵ���channel�ķ�ʽ
	Channel* channel;
};

class Dispatcher; // ���EventLoop.h��Dispatcher.h�ظ���������

// ÿ��EventLoop��Ӧһ��dispatcher
class EventLoop {

public:
	EventLoop(); // ���߳�
	EventLoop(const string threadName); // ���߳�
	~EventLoop();

	// ������Ӧ��ģ��
	int run();
	// ���������fd
	int eventActivate(int fd, int event);
	// ��ӽڵ㵽��Ӧ��ʵ�������������
	int addTask(struct Channel* channel, ElemType type);
	// ����������е�����
	int processTaskQ();
	// ����diapatcher�еĽڵ�
	int add(Channel* channel);
	int remove(Channel* channel);
	int modify(Channel* channel);
	// �ͷ�channel
	int freeChannel(Channel* channel); 
	// ������
	static int readLocalMessage(void* arg); 
	// �ú�����ΪChannel�ĳ�ʼ����������EventLoop����������Ϊ��̬��Ա�������Ա�EventLoop��δʵ����ʱ��Channelʹ��
	// ���ڵľ�̬��Ա����������ĳ������ֻҪ���ж��壬�ú����ʹ���
	int readMessage(); // ��һ��ʵ�ַ�ʽ
	// ��ȡ�����߳�ID
	inline thread::id getThreadID() {
		return m_threadID;
	}

private:
	// д����
	void taskWakeup();

private:
	// EventLoop����
	bool m_isQuit;
	// ��ָ��ָ�������ʵ�� epoll, poll, select
	Dispatcher* m_dispatcher; 
	// ������У������ڵ�����ΪChannelElement�ṹ��
	queue<ChannelElement*> m_taskQ;
	// �洢fd��channel��ӳ���ϵ
	map<int, Channel*> m_channelMap;  // key--int, value--channel
	// �߳�id
	thread::id m_threadID;  
	// �߳���
	string m_threadName;	
	// �����������������������
	mutex m_mutex; 
	// �洢����ͨ�ŵ�fd��ͨ��socketPair������ʼ��
	int m_socketPair[2]; 
};


