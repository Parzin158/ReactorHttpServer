#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
using namespace std;


#define DIRECTORY "/root/projects/ReactorHttp-Cpp/Boji"
#define PORT 9999

class TcpServer {
public:
	TcpServer(unsigned short port, int threadNum);
	~TcpServer();
	// ��ʼ������fd
	void setListen();
	// ����������
	void run();
	// �ص���������������
	static int acceptConnection(void* arg);
	// ������ԴĿ¼
	inline void setDirectory(string dir) {
		m_directory = dir;
	}

private:
	int m_threadNum;
	EventLoop* m_mainLoop;  // ����Ӧ��ģ��
	ThreadPool* m_threadPool;  // �̳߳�
	int m_lfd;
	unsigned short m_port;	
	string m_directory;
};


