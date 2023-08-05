#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"
#include <vector>
using namespace std;



// 定义线程池
class ThreadPool {
public:
	ThreadPool(EventLoop* mainLoop, int count);
	~ThreadPool();

	// 启动线程池
	void run();
	// 取出线程池中某个子线程的反应堆实例
	EventLoop* takeWorkerEventLoop();

private:
	// 主线程反应堆模型，只负责和客户端建立连接，除非线程池中没有子线程
	EventLoop* m_mainLoop;
	// 线程池开关
	bool m_isStart;  
	// 子线程总数
	int m_threadNum;  
	// 存储子线程实例
	vector<WorkerThread*> m_workerThread;  
	// 标记工作线程
	int m_index;  
};


