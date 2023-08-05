#pragma once 
#include "Buffer.h"
#include <stdbool.h>
#include "HttpResponse.h"
#include <map>
#include "TcpServer.h"
using namespace std;


// ö�ٵ�ǰ����״̬
enum class PrecessState:char {
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone
};


// ����http����ṹ��
class  HttpRequest{
public:
	HttpRequest();
	~HttpRequest();

	// ����httprequest
	void reset();

	// ��ȡ����״̬
	inline PrecessState getState() {
		return m_curState;
	}
	inline void setState(PrecessState state) {
		m_curState = state;
	}

	// �������ͷ��reqHeaders����
	void addHeader(const string key, const string value);

	// ����key�õ�����ͷ��value
	string getHeader(const char* key);

	// ����������
	bool parseRequestLine(Buffer* readBuf);

	// ��������ͷ
	bool parseRequestHeader(Buffer* readBuf);

	// ����http����Э��
	bool parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);

	// ����http����Э��
	bool processHttpRequest(HttpResponse* response);

	// �Ա�utf-8����������ַ����н��룬�磺Linux%E5%86%85%E6%A0%B8.jpg
	string decodeMsg(string msg);

	// ��ȡ�ļ���Ӧ��content-type 
	const string getFileType(const string name);

	// ����Ŀ¼ 
	static void sendDir(const string dirName, Buffer* sendBuf, int cfd);

	// �����ļ� 
	static void sendFile(const string fileName, Buffer* sendBuf, int cfd);

	inline void setMethod(string method) {
		m_method = method;
	}
	inline void setUrl(string url) {
		m_url = url;
	}
	inline void setVersion(string version) {
		m_version = version;
	}


private:
	// ��������У����һ������Ϊ�ɵ��ö����װ��������ֵvoid����������string��
	char* splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)>callback);
	// ��ʮ�������ַ�ת��Ϊʮ����������
	int hexToDec(char c);

private:
	PrecessState m_curState;

	// ������
	string m_method;
	string m_url;
	string m_version; 

	// ����ͷ��ֵ��
	map<string, string> m_reqHeaders;
};

