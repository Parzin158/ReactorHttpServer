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
	assert(!m_isStart); // 在运行该函数前，线程池已启动则出错
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		// 启动线程池的线程非主线程，则出错返回
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum > 0) {
		// 逐一初始化子线程实例并运行
		for (int i = 0; i < m_threadNum; ++i) {
			WorkerThread* subThread = new WorkerThread(i);
			subThread->run();
			m_workerThread.push_back(subThread);
		}
	}
	// 线程池中的子线程个数为0，则使用主线程工作
}

EventLoop* ThreadPool::takeWorkerEventLoop()
{
	// 主线程完成
	assert(m_isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id()) {
		// 当前线程非主线程，则出错返回
		exit(0);
	}
	// 从线程池中选择一个子线程，取出子线程的反应堆实例
	EventLoop* evLoop = m_mainLoop; // 当子线程个数为0时，使用主线程反应堆实例
	if (m_threadNum > 0) {
		evLoop = m_workerThread[m_index]->getEventLoop();
		m_index = ++m_index % m_threadNum; // 取余操作
	}
	return evLoop;
}
