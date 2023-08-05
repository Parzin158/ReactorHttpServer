#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include "WorkerThread.h"
#include <vector>
using namespace std;



// �����̳߳�
class ThreadPool {
public:
	ThreadPool(EventLoop* mainLoop, int count);
	~ThreadPool();

	// �����̳߳�
	void run();
	// ȡ���̳߳���ĳ�����̵߳ķ�Ӧ��ʵ��
	EventLoop* takeWorkerEventLoop();

private:
	// ���̷߳�Ӧ��ģ�ͣ�ֻ����Ϳͻ��˽������ӣ������̳߳���û�����߳�
	EventLoop* m_mainLoop;
	// �̳߳ؿ���
	bool m_isStart;  
	// ���߳�����
	int m_threadNum;  
	// �洢���߳�ʵ��
	vector<WorkerThread*> m_workerThread;  
	// ��ǹ����߳�
	int m_index;  
};


