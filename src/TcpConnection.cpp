#include "TcpConnection.h"
#include "HttpRequest.h"
#include <stdlib.h>
#include <stdio.h>
#include "Log.h"
#include <iostream>


// channel中对应fd读事件触发
int TcpConnection::processRead(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 接收数据
	int socket = conn->m_channel->getSocket();
	ssize_t count = conn->m_readBuf->socketRead(socket);
	printf("Receving http data: %s", conn->m_readBuf->data());
	if (count > 0) {
		// 接收到了http请求数据，解析该请求
#ifdef MSG_SEND_AUTO
		// 数据全部解析完毕再发送
		conn->m_channel->wtireEventEnable(true);
		conn->m_evLoop->addTask(conn->channel, ElemType::MODIFY); // 添加fd的读写事件到检测集合中
		// 程序不会再执行precessWrite函数
#endif 
		// 边解析边发送
		bool flag = conn->m_request->parseHttpRequest(conn->m_readBuf, conn->m_response, conn->m_writeBuf, socket);
		if (!flag) {
			// 解析失败，回复错误html
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else if (count == 0) {
#ifdef MSG_SEND_AUTO
		// 断开连接 
		conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE); 
#endif
	}
#ifndef MSG_SEND_AUTO
	// 断开连接 
	conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}


// channel中对应fd写事件触发
int TcpConnection::processWrite(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 发送数据
	ssize_t count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0) {
		// 判断数据是否全部发送出去
		if (conn->m_writeBuf->readableSize() == 0) {
			//// 1. 不再检测写事件 -- 修改channel中保存的事件
			conn->m_channel->writeEventEnable(false);
			//// 2. 修改dispatcher检测的集合 -- 添加任务节点MODIFY
			conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
			// 3. 删除该节点
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
	// 通信fd读事件触发，用processRead回调函数处理接收数据
	m_evLoop->addTask(m_channel, ElemType::ADD);  // 添加到任务队列中
}

TcpConnection::~TcpConnection()
{
	if (m_readBuf && m_readBuf->readableSize() == 0 && m_writeBuf && m_writeBuf->readableSize() == 0) {
		// readBuf和writeBuf内都没有数据，则释放conn占用内存，但不释放evloop
		delete m_readBuf;
		delete m_writeBuf;
		delete m_request;
		delete m_response;
		cout << "free TcpConnection" << endl;
	}
}
