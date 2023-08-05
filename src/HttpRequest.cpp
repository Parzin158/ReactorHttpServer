// #define _GNU_SOURCE
#include "HttpRequest.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include "TcpConnection.h"
#include "Log.h"


HttpRequest::HttpRequest()
{
	reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{	
	m_curState = PrecessState::ParseReqLine;
	m_method = m_url = m_version = string();
	m_reqHeaders.clear();
}

void HttpRequest::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty()) {
		return;
	}
	m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const char* key)
{
	return m_reqHeaders.find(key) == m_reqHeaders.end() ? string() : m_reqHeaders[key];
	// �ҳ�keyֵ��Ӧ��value�����򷵻ؿ��ַ�
}

bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	// ���������У������ַ���������ַ
	char* end = readBuf->findCRLF();
	// �����ַ�����ʼ��ַ
	char* start = readBuf->data();
	// �������ܳ���
	long int lineSize = end - start;
	if (lineSize > 0) {
		// get /xxx/xx.xxx http/1.1
		// �������������У�һ�εõ�method/url/version��������requestʵ���Ķ�Ӧ��Ա������
		// �ɵ��ö�����ֱ�Ӱ�װ���ڵķǾ�̬��Ա������setMethod/setUrl/setVersion��Ҫ�ð�����
		// ��setMethod/setUrl/setVersion���ݲ�����ռλ��placeholders::_1��ʾ
		auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1); 
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
		splitRequestLine(start, end, NULL, versionFunc);
		// ����buffer��ָ��λ�ã�+2 �������������е�\r\n
		readBuf->readPosIncrease(lineSize + 2);
		// �޸�״̬����������ͷ
		setState(PrecessState::ParseReqHeaders);
		return true;
	}
	return false; 
}

bool HttpRequest::parseRequestHeader(Buffer* readBuf)
{
	char* end = readBuf->findCRLF();
	if (end != nullptr) {
		char* start = readBuf->data();
		size_t lineSize = end - start;
		// ����":"�����ַ���
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));
		if (middle != NULL) {
			size_t keyLen = middle - start;
			size_t valueLen = end - middle - 2;
			if (keyLen > 0 && valueLen > 0) {
				string key(start, keyLen);
				string value(middle + 2, valueLen);
				addHeader(key, value);
			}
			// ����buffer��ָ��λ�ã�+2 �������������е�\r\n
			readBuf->readPosIncrease(lineSize + 2);
		}
		else {
			// ����ͷ��������������ֻ��\r\n����������
			readBuf->readPosIncrease(2);
			// �޸Ľ���״̬ΪParseReqDone��ֻ����get���󣬺���post����
			setState(PrecessState::ParseReqDone);
		}
		return true;
	}
	return false;
}

bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	while (m_curState != PrecessState::ParseReqDone) {
		switch (m_curState)
		{
		case PrecessState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case PrecessState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case PrecessState::ParseReqBody:
			// get������body
			break;
		default:
			break;
		}
		if (!flag) {
			// flagΪfalse��ֱ�ӽ���
			return flag;
		}
		// �ж��Ƿ������ϣ�����ϣ���׼���ָ�������
		if (m_curState == PrecessState::ParseReqDone) {
			// 1. ���ݽ�������ԭʼ���ݣ�������ͻ�������
			processHttpRequest(response);
			// 2. ��֯��Ӧ���ݲ����͸��ͻ���
			response->prepareMsg(sendBuf, socket);
		}
	}
	m_curState = PrecessState::ParseReqLine; // ��ԭ��ʼ״̬�����㴦��������� 
	return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	// ���������� get /xxx/xx.xxx http/1.1
	if (strcasecmp(m_method.data(), "get") != 0) { // strcasecmp�����ִ�Сд
		return -1; // ��get���󣬷���-1
	}
	// ����������͵������ַ���ʽ���н���
	m_url = decodeMsg(m_url);

	// ����ͻ�������ľ�̬��Դ��Ŀ¼���ļ���
	const char* file = NULL;
	if (0 == strcmp(m_url.data(), "/")) {
		file = "./"; // ����·��ת�������·��
	}
	else {
		file = m_url.data() + 1;
	}

	// ��ȡ�ļ����ԣ���Ŀ¼�����ļ���
	struct stat st; // ʹ��stat�ṹ������ļ�����
	int ret = stat(file, &st);
	// �ļ������� -- ��ͻ��˻ظ�404
	if (-1 == ret) {
		response->setFileName("404.html"); // �ظ�״̬��
		response->setStatusCode(statusCode::NotFound); // �ظ���Ӧͷ
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile; // ������ַ���浽�ɵ��ö�����
		return 0;
	}
	// �ļ����ڣ��ж��ļ�����
	// ��֯״̬��
	response->setFileName(file);
	response->setStatusCode(statusCode::OK);

	if (S_ISDIR(st.st_mode)) {
		// ��Ŀ¼����Ŀ¼�����ݷ��͸��ͻ���
		response->addHeader("Content-type", getFileType(".html")); 
		response->sendDataFunc = sendDir; // ������ַ���浽�ɵ��ö�����
	}
	else {
		// ���ļ������ļ�����
		char tmp[128];
		sprintf(tmp, "%ld", st.st_size);
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile; // ������ַ���浽�ɵ��ö�����
	}
	return false;
}


void HttpRequest::sendDir(const string dirName, Buffer* sendBuf, int cfd)
{
	// ��index.html�ļ�
	string indexfile = DIRECTORY;
	indexfile.append("/index.html");
	ifstream ofs(indexfile, ios::in);
	assert(ofs.is_open());
	ostringstream tmp;
	tmp << ofs.rdbuf();
	string buf = tmp.str();
	tmp.str("");
	tmp.clear();
	ofs.close();
	// ��<table>��ǩ������ļ�Ŀ¼�������
	size_t pos = buf.find("</table>");
	if (buf.length() <= 0 || pos == string::npos) {
		exit(-1);
	}
	struct dirent** namelist; // ����ָ�룬ָ��һ��ָ������ struct dirent* tmp[]
	int num = scandir(dirName.data(), &namelist, NULL, alphasort); // �ڶ��������Ƕ���ָ��namelist�ĵ�ַ
	// ��ȡ��Ŀ¼���ļ��ĸ����������ļ����ļ���������namelistָ���ָ�����鵱��
	for (int i = 0; i < num; ++i) {
		// ȡ���ļ���
		string name = namelist[i]->d_name;
		// �ж����ļ�����Ŀ¼
		struct stat st;
		string subPath;
		subPath.append(dirName);
		subPath.append(name); // Ŀ¼�����ļ���ƴ�ӳ���Ե�ַ
		// char subPath[1024] = { 0 };
		// sprintf(subPath, "%s/%s", dirName.data(), name); 
		stat(subPath.data(), &st);
		string table;
		string size = to_string(st.st_size);
		// ƴ��html�ļ���body����ʾdirNameĿ¼�������ļ����ļ������ļ���С
		if (S_ISDIR(st.st_mode)) {
			// name��һ��Ŀ¼
			// a��ǩ <a href="(��ת����)">name</a>
			table = "<tr><td><a href = " + name + "/>" + name + "</a></td><td>" + size + "</td></tr>";
			buf.insert(pos, table);
			pos += table.length();
		}
		else {
			// name��һ���ļ�
			table = "<tr><td><a href = " + name + ">" + name + "</a></td><td>" + size + "</td></tr>";
			buf.insert(pos, table);
			pos += table.length();
		}
		free(namelist[i]);
	}
	sendBuf->appendString(buf);
	buf = string("");
	sendBuf->sendData(cfd);
	free(namelist);
}

void HttpRequest::sendFile(const string fileName, Buffer* sendBuf, int cfd)
{
	// 1. ���ļ�
	FILE* fp = fopen(fileName.data(), "r");
	if (fp == NULL) {
		perror("fopen");
		return;
	} // ͨ�����Ժ����ϸ��жϴ��ļ��Ƿ����
	size_t bufSize = 40960;
	char fileBuf[bufSize];
	// 2. �����ļ�����
#if 1
	while (1) {
		size_t len = fread(fileBuf, sizeof(char), bufSize, fp);
		if (len > 0) {
			sendBuf->appendString(fileBuf, len);
#ifndef MSG_SEND_AUTO
			sendBuf->sendData(cfd);
#endif
		}
		else if (len == 0) {
			break; //���������˳�
		}
		else {
			fclose(fp);
			perror("fread");
		}
	}
#else
	off_t offset = 0; // ÿ�η����ļ�sendfile��ƫ����
	ssize_t size = lseek(fd, 0, SEEK_END); // ��ͷ��β�����ļ���С
	lseek(fd, 0, SEEK_SET); // ���½�lseekָ���β���ƶ�����ͷ
	while (offset < size) {
		int ret = sendfile(cfd, fd, &offset, size - offset);
		// ��Ϊcfd���óɷ�����ģʽ����fd��д������д���ݵ��ٶȽ�����cfd�����û�����ݿɶ������
		if (-1 == ret) {
			perror("sendfile");
		}
	}
#endif
	fclose(fp);
}


char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)
{
	// start--�ַ�����ʼ��ַ��end--�ַ�����β��ַ��sub--��ǵ��ַ����ո����NULL����callback--�ɵ��ö����װ��������ص�����
	char* space = const_cast<char*>(end);
	if (sub != nullptr) { // ��������ʽ�;�̬��Դ
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub))); // ���ؿո�λ�õĵ�ַ
		assert(space != nullptr);
	}
	size_t length = space - start; // ��ȡ���ǰ�ַ��ĳ���
	callback(string(start, length)); 
	// ��string(start, length)��Ϊ��������method/url/version������Ӧ��set���������浽��Ӧ���Ա������
	return space + 1;
}


string HttpRequest::decodeMsg(string msg)
{
	string str = string();
	const char* from = msg.data();
	while (*from != '\0') {
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char, �� B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2;
		}
		else {
			str.append(1, *from);
		}
		++from;
	}
	str.append(1, '\0');
	return str;
}


int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}


const string HttpRequest::getFileType(const string name)
{
	// ����a.jpg a.mp4 a.html
	// ����������ҡ�.���ַ�, �粻���ڷ���NULL
	const char* dot = strrchr(name.data(), '.'); //��������ļ��������������"."
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mp4") == 0)
		return "video/mpeg4";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8"; // �����ļ����ش��ı�
}