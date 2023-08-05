#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "Log.h"
#include <iostream>


// channel�ж�Ӧfd���¼�����
int TcpConnection::processRead(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// ��������
	int socket = conn->m_channel->getSocket();
	ssize_t count = conn->m_readBuf->socketRead(socket);
	printf("Receving http data: %s", conn->m_readBuf->data());
	if (count > 0) {
		// ���յ���http�������ݣ�����������
#ifdef MSG_SEND_AUTO
		// ����ȫ����������ٷ���
		conn->m_channel->wtireEventEnable(true);
		conn->m_evLoop->addTask(conn->channel, ElemType::MODIFY); // ���fd�Ķ�д�¼�����⼯����
		// ���򲻻���ִ��precessWrite����
#endif 
		// �߽����߷���
		bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
		if (!flag) {
			// ����ʧ�ܣ��ظ�����html
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else if (count == 0) {
#ifdef MSG_SEND_AUTO
		// �Ͽ����� 
		conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE); 
#endif
	}
#ifndef MSG_SEND_AUTO
	// �Ͽ����� 
	conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}


// channel�ж�Ӧfdд�¼�����
int TcpConnection::processWrite(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// ��������
	ssize_t count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0) {
		// �ж������Ƿ�ȫ�����ͳ�ȥ
		if (conn->m_writeBuf->readableSize() == 0) {
			//// 1. ���ټ��д�¼� -- �޸�channel�б�����¼�
			conn->m_channel->writeEventEnable(false);
			//// 2. �޸�dispatcher���ļ��� -- �������ڵ�MODIFY
			conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
			// 3. ɾ���ýڵ�
			conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}


int TcpConnection::connDestroy(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	if (conn != nullptr) {
		delete conn;
	}
	return 0;
}


TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240);
	m_writeBuf = new Buffer(10240);
	m_request = new HttpRequest;
	m_response = new HttpResponse;
	m_name = "Connection-" + to_string(fd);	
	m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, connDestroy, this);
	// ͨ��fd���¼���������processRead�ص����������������
	m_evLoop->addTask(m_channel, ElemType::ADD);  // ��ӵ����������
}

TcpConnection::~TcpConnection()
{
	if (m_readBuf && m_readBuf->readableSize() == 0 && m_writeBuf && m_writeBuf->readableSize() == 0) {
		// readBuf��writeBuf�ڶ�û�����ݣ����ͷ�connռ���ڴ棬�����ͷ�evloop
		delete m_readBuf;
		delete m_writeBuf;
		delete m_request;
		delete m_response;
		cout << "free TcpConnection" << endl;
	}
}
