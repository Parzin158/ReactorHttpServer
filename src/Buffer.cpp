#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>

// #define _GNU_SOURCE

Buffer::Buffer(size_t size):m_capacity(size)
{
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr) {
		free(m_data);
	}
}

void Buffer::extendRoom(size_t size)
{	
	// 1. 内存够用 - 无需扩容
	if (writeableSize() >= size) {
		return;
	}
	// 2. 内存需合并 - 无需扩容 （剩余的可写内存 + 已读的内存) > size
	else if (m_readPos + writeableSize() >= size) {
		// 未读内存前移
		size_t readable = readableSize(); // 得到未读内存大小
		memcpy(m_data, m_data + m_readPos, readable); // 移动内存
		m_readPos = 0;
		m_writePos = readable; // 更新位置
	}
	// 3. 内存不够用 - 扩容
	else {
		void* temp = realloc(m_data, m_capacity + size);
		if (temp == NULL) {
			return; // 扩容失败
		}
		memset((char*)temp + m_capacity, 0, size); // 新容量置0
		m_data = (char*)temp; 
		m_capacity += size; // 更新数据
	}
}

int Buffer::appendString(const char* data)
{
	size_t size = strlen(data);
	int ret = appendString(data, size);
	return ret;
}

int Buffer::appendString(const char* data, size_t size)
{
	if (data == nullptr || size <= 0) {
		return -1;
	}
	// 扩容
	extendRoom(size);
	// data中的数据拷贝到buffer中
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::appendString(const string data)
{
	int ret = appendString(data.data());
	return ret;
}


ssize_t Buffer::socketRead(int fd)
{
	// read, recv, readv
	struct iovec vec[2]; // 初始化readv函数的数组元素
	size_t writeable = writeableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	ssize_t result = readv(fd, vec, 2); // 返回接收字节数
	if (result < 0) {
		return -1;
	}
	else if ((size_t)result <= writeable) {
		m_writePos += result; // 存储成功，修改writePos位置
	}
	else {
		// vec[0]指向的data空间不够用，多出的数据被存储到vec[1]的，
		m_writePos = m_capacity; // 更新buffer的可写位置
		appendString(tmpbuf, result - writeable);  // 需要增加的长度 = 已接收长度result - 已写入buffer的长度writeable
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr() --> 大字符串匹配子字符串（遇到\0）结束
	// memmen() --> 大数据块匹配子数据块（需指定数据块大小）
	char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

ssize_t Buffer::sendData(int socket)
{
	// 判断是否有数据
	size_t readable = readableSize();
	if (readable > 0) {
		ssize_t count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count > 0) {
			m_readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
