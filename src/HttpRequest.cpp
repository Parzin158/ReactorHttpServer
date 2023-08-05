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
	// 找出key值对应的value，否则返回空字符
}

bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	// 读出请求行，保存字符串结束地址
	char* end = readBuf->findCRLF();
	// 保存字符串起始地址
	char* start = readBuf->data();
	// 请求行总长度
	long int lineSize = end - start;
	if (lineSize > 0) {
		// get /xxx/xx.xxx http/1.1
		// 解析三次请求行，一次得到method/url/version，并存入request实例的对应成员变量中
		// 可调用对象不能直接包装类内的非静态成员函数，setMethod/setUrl/setVersion需要用绑定器绑定
		// 给setMethod/setUrl/setVersion传递参数用占位符placeholders::_1表示
		auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1); 
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
		splitRequestLine(start, end, NULL, versionFunc);
		// 更新buffer读指针位置，+2 用于跳过请求行的\r\n
		readBuf->readPosIncrease(lineSize + 2);
		// 修改状态，解析请求头
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
		// 基于":"搜索字符串
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));
		if (middle != NULL) {
			size_t keyLen = middle - start;
			size_t valueLen = end - middle - 2;
			if (keyLen > 0 && valueLen > 0) {
				string key(start, keyLen);
				string value(middle + 2, valueLen);
				addHeader(key, value);
			}
			// 更新buffer读指针位置，+2 用于跳过请求行的\r\n
			readBuf->readPosIncrease(lineSize + 2);
		}
		else {
			// 请求头解析结束，空行只有\r\n，跳过空行
			readBuf->readPosIncrease(2);
			// 修改解析状态为ParseReqDone，只解析get请求，忽略post请求
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
			// get请求无body
			break;
		default:
			break;
		}
		if (!flag) {
			// flag为false，直接结束
			return flag;
		}
		// 判断是否解析完毕，若完毕，则准备恢复的数据
		if (m_curState == PrecessState::ParseReqDone) {
			// 1. 根据解析出的原始数据，来处理客户端请求
			processHttpRequest(response);
			// 2. 组织响应数据并发送给客户端
			response->prepareMsg(sendBuf, socket);
		}
	}
	m_curState = PrecessState::ParseReqLine; // 还原初始状态，方便处理后续请求 
	return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	// 解析请求行 get /xxx/xx.xxx http/1.1
	if (strcasecmp(m_method.data(), "get") != 0) { // strcasecmp不区分大小写
		return -1; // 非get请求，返回-1
	}
	// 对浏览器发送的特殊字符格式进行解码
	m_url = decodeMsg(m_url);

	// 处理客户端请求的静态资源（目录或文件）
	const char* file = NULL;
	if (0 == strcmp(m_url.data(), "/")) {
		file = "./"; // 绝对路径转换成相对路径
	}
	else {
		file = m_url.data() + 1;
	}

	// 获取文件属性（是目录或是文件）
	struct stat st; // 使用stat结构体接收文件属性
	int ret = stat(file, &st);
	// 文件不存在 -- 向客户端回复404
	if (-1 == ret) {
		response->setFileName("404.html"); // 回复状态码
		response->setStatusCode(statusCode::NotFound); // 回复响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile; // 函数地址保存到可调用对象中
		return 0;
	}
	// 文件存在，判断文件类型
	// 组织状态码
	response->setFileName(file);
	response->setStatusCode(statusCode::OK);

	if (S_ISDIR(st.st_mode)) {
		// 是目录，则将目录的内容发送给客户端
		response->addHeader("Content-type", getFileType(".html")); 
		response->sendDataFunc = sendDir; // 函数地址保存到可调用对象中
	}
	else {
		// 是文件，则将文件发送
		char tmp[128];
		sprintf(tmp, "%ld", st.st_size);
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile; // 函数地址保存到可调用对象中
	}
	return false;
}


void HttpRequest::sendDir(const string dirName, Buffer* sendBuf, int cfd)
{
	// 打开index.html文件
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
	// 从<table>标签后插入文件目录表格数据
	size_t pos = buf.find("</table>");
	if (buf.length() <= 0 || pos == string::npos) {
		exit(-1);
	}
	struct dirent** namelist; // 二级指针，指向一个指针数组 struct dirent* tmp[]
	int num = scandir(dirName.data(), &namelist, NULL, alphasort); // 第二个参数是二级指针namelist的地址
	// 获取该目录下文件的个数，所有文件的文件名保存在namelist指向的指针数组当中
	for (int i = 0; i < num; ++i) {
		// 取出文件名
		string name = namelist[i]->d_name;
		// 判断是文件还是目录
		struct stat st;
		string subPath;
		subPath.append(dirName);
		subPath.append(name); // 目录名与文件名拼接成相对地址
		// char subPath[1024] = { 0 };
		// sprintf(subPath, "%s/%s", dirName.data(), name); 
		stat(subPath.data(), &st);
		string table;
		string size = to_string(st.st_size);
		// 拼接html文件的body来显示dirName目录下所有文件的文件名及文件大小
		if (S_ISDIR(st.st_mode)) {
			// name是一个目录
			// a标签 <a href="(跳转对象)">name</a>
			table = "<tr><td><a href = " + name + "/>" + name + "</a></td><td>" + size + "</td></tr>";
			buf.insert(pos, table);
			pos += table.length();
		}
		else {
			// name是一个文件
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
	// 1. 打开文件
	FILE* fp = fopen(fileName.data(), "r");
	if (fp == NULL) {
		perror("fopen");
		return;
	} // 通过断言函数严格判断打开文件是否出错
	size_t bufSize = 40960;
	char fileBuf[bufSize];
	// 2. 发送文件内容
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
			break; //读完数据退出
		}
		else {
			fclose(fp);
			perror("fread");
		}
	}
#else
	off_t offset = 0; // 每次发送文件sendfile的偏移量
	ssize_t size = lseek(fd, 0, SEEK_END); // 从头到尾计算文件大小
	lseek(fd, 0, SEEK_SET); // 重新将lseek指针从尾部移动到开头
	while (offset < size) {
		int ret = sendfile(cfd, fd, &offset, size - offset);
		// 因为cfd设置成非阻塞模式，因fd向写缓冲区写数据的速度较慢，cfd会出现没有数据可读的情况
		if (-1 == ret) {
			perror("sendfile");
		}
	}
#endif
	fclose(fp);
}


char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)
{
	// start--字符串起始地址；end--字符串结尾地址；sub--标记的字符（空格或者NULL）；callback--可调用对象包装器，传入回调函数
	char* space = const_cast<char*>(end);
	if (sub != nullptr) { // 解析请求方式和静态资源
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub))); // 返回空格位置的地址
		assert(space != nullptr);
	}
	size_t length = space - start; // 获取标记前字符的长度
	callback(string(start, length)); 
	// 将string(start, length)作为参数，即method/url/version传给对应的set函数，保存到对应类成员变量中
	return space + 1;
}


string HttpRequest::decodeMsg(string msg)
{
	string str = string();
	const char* from = msg.data();
	while (*from != '\0') {
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char, 如 B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
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
	// 例如a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name.data(), '.'); //将传入的文件名从右往左查找"."
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
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

	return "text/plain; charset=utf-8"; // 特殊文件返回纯文本
}