#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
using namespace std;


// #define MSG_SEND_AUTO  

/* ���ķ������ݷ�ʽ��
	1 һ���Է��������ļ�
	0 һ���ļ��ֶ�η���
*/

class TcpConnection {
public:
	TcpConnection(int fd, EventLoop* evloop);
	~TcpConnection();

	static int processRead(void* arg);
	static int processWrite(void* arg);
	static int connDestroy(void* arg);


private:
	string m_name;
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;
	Buffer* m_writeBuf;
	HttpRequest* m_request;
	HttpResponse* m_response;
};