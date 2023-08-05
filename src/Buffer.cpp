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
	// 1. �ڴ湻�� - ��������
	if (writeableSize() >= size) {
		return;
	}
	// 2. �ڴ���ϲ� - �������� ��ʣ��Ŀ�д�ڴ� + �Ѷ����ڴ�) > size
	else if (m_readPos + writeableSize() >= size) {
		// δ���ڴ�ǰ��
		size_t readable = readableSize(); // �õ�δ���ڴ��С
		memcpy(m_data, m_data + m_readPos, readable); // �ƶ��ڴ�
		m_readPos = 0;
		m_writePos = readable; // ����λ��
	}
	// 3. �ڴ治���� - ����
	else {
		void* temp = realloc(m_data, m_capacity + size);
		if (temp == NULL) {
			return; // ����ʧ��
		}
		memset((char*)temp + m_capacity, 0, size); // ��������0
		m_data = (char*)temp; 
		m_capacity += size; // ��������
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
	// ����
	extendRoom(size);
	// data�е����ݿ�����buffer��
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
	struct iovec vec[2]; // ��ʼ��readv����������Ԫ��
	size_t writeable = writeableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	ssize_t result = readv(fd, vec, 2); // ���ؽ����ֽ���
	if (result < 0) {
		return -1;
	}
	else if ((size_t)result <= writeable) {
		m_writePos += result; // �洢�ɹ����޸�writePosλ��
	}
	else {
		// vec[0]ָ���data�ռ䲻���ã���������ݱ��洢��vec[1]�ģ�
		m_writePos = m_capacity; // ����buffer�Ŀ�дλ��
		appendString(tmpbuf, result - writeable);  // ��Ҫ���ӵĳ��� = �ѽ��ճ���result - ��д��buffer�ĳ���writeable
	}
	free(tmpbuf);
	return result;
}

char* Buffer::findCRLF()
{
	// strstr() --> ���ַ���ƥ�����ַ���������\0������
	// memmen() --> �����ݿ�ƥ�������ݿ飨��ָ�����ݿ��С��
	char* ptr = (char*)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

ssize_t Buffer::sendData(int socket)
{
	// �ж��Ƿ�������
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
