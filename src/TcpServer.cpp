#include "TcpServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Channel.h"
#include "Log.h"
#include <iostream>


// 回调函数，建立连接
int TcpServer::acceptConnection(void* arg) {
	// 若acceptConnection是静态函数，则在该函数内部无法访问TcpServer类中的实例对象，需要将该server对象通过参数传递进来
	TcpServer* server = static_cast<TcpServer*>(arg);
	// 和客户端建立连接
	int cfd = accept(server->m_lfd, NULL, NULL);
	cout << "connection accepted.." << endl;

	// 从线程池中取出一个子线程反应堆实例，去处理cfd	
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// 将cfd放到 TcpConnection中处理
	new TcpConnection(cfd, evLoop); // 匿名对象
	return 0;
}

TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop; // 当前是主线程实例化EventLoop对象无需参数
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum); // 实例化ThreadPool对象
	setListen();
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
	// 1. 创建监听fd
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == m_lfd) {
		perror("socket");
		return;
	}
	// 2. 设置端口复用
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (-1 == ret) {
		perror("setsockopt");
		return;
	}
	// 3. 绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	socklen_t len = sizeof(addr);
	ret = bind(m_lfd, (struct sockaddr*)&addr, len);
	if (-1 == ret) {
		perror("bind");
		return;
	}
	// 4. 设置监听
	ret = listen(m_lfd, 128);
	if (-1 == ret) {
		perror("listen");
		return;
	}
}

void TcpServer::run()
{	
	// 启动线程池
	m_threadPool->run();	
	// 初始化一个channel实例监听lfd读事件，触发读事件调用acceptConnection
	Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, nullptr, this);
	// 添加检测的任务
	m_mainLoop->addTask(channel, ElemType::ADD);
	// 启动反应堆模型
 	m_mainLoop->run();
}
