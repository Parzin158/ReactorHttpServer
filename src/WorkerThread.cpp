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
	// 创建子线程
	m_thread = new thread(&WorkerThread::running, this); // 参数1需为子线程执行函数地址，参数2代表所属类别
	// 阻塞主线程，等待子线程完成初始化EventLoop
	unique_lock<mutex> locker(m_mutex);
	while (m_evLoop == nullptr) {
		m_cond.wait(locker);
	}
	// 当程序结束locker的生命周期结束，其中的m_mutex自动解锁
}

// 子线程的回调函数
void WorkerThread::running()
{
	m_mutex.lock();
	m_evLoop = new EventLoop(m_name); // m_name作为参数构造EventLoop实例
	m_mutex.unlock();
	m_cond.notify_one(); // 通知主线程解除阻塞
	m_evLoop->run();
}
