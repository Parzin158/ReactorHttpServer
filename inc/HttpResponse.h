#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
#include <string>
using namespace std;

// 定义状态码
enum class statusCode {
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};

// 定义一个函数指针，用来组织回复客户端的数据块
typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);
// fileName--根据文件名做响应处理（目录或非目录）
// sendBuf--发送数据的数据块
// socket--cfd

class HttpResponse {
public:
	HttpResponse();
	~HttpResponse();

	// 数据块（使用函数指针来处理）
	function<void(const string, struct Buffer*, int)> sendDataFunc;

	// 添加响应头
	void addHeader(const string key, const string value);

	// 组织http响应数据
	void prepareMsg(struct Buffer* sendBuf, int socket);

	inline void setFileName(string name) {
		m_fileName = name;
	}
	inline void setStatusCode(statusCode code) {
		m_statusCode = code;
	}

private:
	// 状态行：状态码，状态码的描述，http协议版本
	statusCode m_statusCode;
	string m_fileName;

	// 状态码和状态描述的对应关系
	const map<int, string> m_info = {
		{200,"OK"},
		{301,"MovedPermanently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};

	// 响应头键值对
	map<string, string> m_headers;
};
