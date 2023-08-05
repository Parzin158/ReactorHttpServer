#include "ThreadPool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_index = 0;
	m_isStart = false;
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThread.clear();
}

ThreadPool::~ThreadPool()
{
	for (auto item : m_workerThread) {
		delete item;
	}
}

void ThreadPool::run()
{
	assert(!m_isStart); // �����иú���ǰ���̳߳������������
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		// �����̳߳ص��̷߳����̣߳��������
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum > 0) {
		// ��һ��ʼ�����߳�ʵ��������
		for (int i = 0; i < m_threadNum; ++i) {
			WorkerThread* subThread = new WorkerThread(i);
			subThread->run();
			m_workerThread.push_back(subThread);
		}
	}
	// �̳߳��е����̸߳���Ϊ0����ʹ�����̹߳���
}

EventLoop* ThreadPool::takeWorkerEventLoop()
{
	// ���߳����
	assert(m_isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		// ��ǰ�̷߳����̣߳��������
		exit(0);
	}
	// ���̳߳���ѡ��һ�����̣߳�ȡ�����̵߳ķ�Ӧ��ʵ��
	EventLoop* evLoop = m_mainLoop; // �����̸߳���Ϊ0ʱ��ʹ�����̷߳�Ӧ��ʵ��
	if (m_threadNum > 0) {
		evLoop = m_workerThread[m_index]->getEventLoop();
		m_index = ++m_index % m_threadNum; // ȡ�����
	}
	return evLoop;
}
