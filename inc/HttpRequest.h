#pragma once 
#include "Buffer.h"
#include <stdbool.h>
#include "HttpResponse.h"
#include <map>
#include "TcpServer.h"
using namespace std;


// 枚举当前解析状态
enum class PrecessState:char {
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone
};


// 定义http请求结构体
class  HttpRequest{
public:
	HttpRequest();
	~HttpRequest();

	// 重置httprequest
	void reset();

	// 获取处理状态
	inline PrecessState getState() {
		return m_curState;
	}
	inline void setState(PrecessState state) {
		m_curState = state;
	}

	// 添加请求头到reqHeaders数组
	void addHeader(const string key, const string value);

	// 根据key得到请求头的value
	string getHeader(const char* key);

	// 解析请求行
	bool parseRequestLine(Buffer* readBuf);

	// 解析请求头
	bool parseRequestHeader(Buffer* readBuf);

	// 解析http请求协议
	bool parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);

	// 处理http请求协议
	bool processHttpRequest(HttpResponse* response);

	// 对被utf-8编码的特殊字符进行解码，如：Linux%E5%86%85%E6%A0%B8.jpg
	string decodeMsg(string msg);

	// 获取文件对应的content-type 
	const string getFileType(const string name);

	// 发送目录 
	static void sendDir(const string dirName, Buffer* sendBuf, int cfd);

	// 发送文件 
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
	// 拆分请求行，最后一个参数为可调用对象包装器（返回值void，参数类型string）
	char* splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)>callback);
	// 将十六进制字符转换为十进制整形数
	int hexToDec(char c);

private:
	PrecessState m_curState;

	// 请求行
	string m_method;
	string m_url;
	string m_version; 

	// 请求头键值对
	map<string, string> m_reqHeaders;
};

