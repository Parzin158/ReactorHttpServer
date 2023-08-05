#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

// 子线程对应的结构体
class WorkerThread
{
public:
	WorkerThread(int index);
	~WorkerThread();

	// 启动线程
	void run();
	inline EventLoop* getEventLoop() {
		return m_evLoop;
	}

private:
	// 子线程的回调函数
	void running();


private:
	thread* m_thread; // 保存线程地址
	thread::id m_threadID; 
	string m_name;
	mutex m_mutex; // 互斥锁
	condition_variable m_cond; // 条件变量
	EventLoop* m_evLoop; // 反应堆模型
};


