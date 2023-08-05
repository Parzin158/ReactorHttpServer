#include "WorkerThread.h"
#include <stdio.h>


WorkerThread::WorkerThread(int index)
{
	m_evLoop = nullptr;
	m_thread = nullptr;
	m_threadID = thread::id();
	m_name = "SubThread-" + to_string(index);
}

WorkerThread::~WorkerThread()
{
	if (m_thread != nullptr) {
		delete m_thread;
	}
}

void WorkerThread::run()
{
	// �������߳�
	m_thread = new thread(&WorkerThread::running, this); // ����1��Ϊ���߳�ִ�к�����ַ������2�����������
	// �������̣߳��ȴ����߳���ɳ�ʼ��EventLoop
	unique_lock<mutex> locker(m_mutex);
	while (m_evLoop == nullptr) {
		m_cond.wait(locker);
	}
	// ���������locker���������ڽ��������е�m_mutex�Զ�����
}

// ���̵߳Ļص�����
void WorkerThread::running()
{
	m_mutex.lock();
	m_evLoop = new EventLoop(m_name); // m_name��Ϊ��������EventLoopʵ��
	m_mutex.unlock();
	m_cond.notify_one(); // ֪ͨ���߳̽������
	m_evLoop->run();
}
