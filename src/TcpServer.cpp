#include "TcpServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Channel.h"
#include "Log.h"
#include <iostream>


// �ص���������������
int TcpServer::acceptConnection(void* arg) {
	// ��acceptConnection�Ǿ�̬���������ڸú����ڲ��޷�����TcpServer���е�ʵ��������Ҫ����server����ͨ���������ݽ���
	TcpServer* server = static_cast<TcpServer*>(arg);
	// �Ϳͻ��˽�������
	int cfd = accept(server->m_lfd, NULL, NULL);
	cout << "connection accepted.." << endl;

	// ���̳߳���ȡ��һ�����̷߳�Ӧ��ʵ����ȥ����cfd	
	EventLoop* evLoop = server->m_threadPool->takeWorkerEventLoop();
	// ��cfd�ŵ� TcpConnection�д���
	new TcpConnection(cfd, evLoop); // ��������
	return 0;
}

TcpServer::TcpServer(unsigned short port, int threadNum)
{
	m_port = port;
	m_mainLoop = new EventLoop; // ��ǰ�����߳�ʵ����EventLoop�����������
	m_threadNum = threadNum;
	m_threadPool = new ThreadPool(m_mainLoop, threadNum); // ʵ����ThreadPool����
	setListen();
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
	// 1. ��������fd
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == m_lfd) {
		perror("socket");
		return;
	}
	// 2. ���ö˿ڸ���
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (-1 == ret) {
		perror("setsockopt");
		return;
	}
	// 3. ��
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
	// 4. ���ü���
	ret = listen(m_lfd, 128);
	if (-1 == ret) {
		perror("listen");
		return;
	}
}

void TcpServer::run()
{	
	// �����̳߳�
	m_threadPool->run();	
	// ��ʼ��һ��channelʵ������lfd���¼����������¼�����acceptConnection
	Channel* channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection, nullptr, nullptr, this);
	// ��Ӽ�������
	m_mainLoop->addTask(channel, ElemType::ADD);
	// ������Ӧ��ģ��
 	m_mainLoop->run();
}
