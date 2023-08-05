#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

// ���̶߳�Ӧ�Ľṹ��
class WorkerThread
{
public:
	WorkerThread(int index);
	~WorkerThread();

	// �����߳�
	void run();
	inline EventLoop* getEventLoop() {
		return m_evLoop;
	}

private:
	// ���̵߳Ļص�����
	void running();


private:
	thread* m_thread; // �����̵߳�ַ
	thread::id m_threadID; 
	string m_name;
	mutex m_mutex; // ������
	condition_variable m_cond; // ��������
	EventLoop* m_evLoop; // ��Ӧ��ģ��
};


