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
	// 初始化监听fd
	void setListen();
	// 启动服务器
	void run();
	// 回调函数，建立连接
	static int acceptConnection(void* arg);
	// 设置资源目录
	inline void setDirectory(string dir) {
		m_directory = dir;
	}

private:
	int m_threadNum;
	EventLoop* m_mainLoop;  // 主反应堆模型
	ThreadPool* m_threadPool;  // 线程池
	int m_lfd;
	unsigned short m_port;	
	string m_directory;
};


