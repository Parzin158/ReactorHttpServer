#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
#include <string>
using namespace std;

// ����״̬��
enum class statusCode {
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};

// ����һ������ָ�룬������֯�ظ��ͻ��˵����ݿ�
typedef void (*responseBody)(const char* fileName, struct Buffer* sendBuf, int socket);
// fileName--�����ļ�������Ӧ����Ŀ¼���Ŀ¼��
// sendBuf--�������ݵ����ݿ�
// socket--cfd

class HttpResponse {
public:
	HttpResponse();
	~HttpResponse();

	// ���ݿ飨ʹ�ú���ָ��������
	function<void(const string, struct Buffer*, int)> sendDataFunc;

	// �����Ӧͷ
	void addHeader(const string key, const string value);

	// ��֯http��Ӧ����
	void prepareMsg(struct Buffer* sendBuf, int socket);

	inline void setFileName(string name) {
		m_fileName = name;
	}
	inline void setStatusCode(statusCode code) {
		m_statusCode = code;
	}

private:
	// ״̬�У�״̬�룬״̬���������httpЭ��汾
	statusCode m_statusCode;
	string m_fileName;

	// ״̬���״̬�����Ķ�Ӧ��ϵ
	const map<int, string> m_info = {
		{200,"OK"},
		{301,"MovedPermanently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};

	// ��Ӧͷ��ֵ��
	map<string, string> m_headers;
};
