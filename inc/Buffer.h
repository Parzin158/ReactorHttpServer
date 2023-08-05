#pragma once
#include <string>

using namespace std;

class Buffer {
public:
	Buffer(size_t size);
	~Buffer();

	// ����
	void extendRoom(size_t size);
	// ��ȡʣ���д�ڴ�����
	inline size_t writeableSize() {
		return m_capacity - m_writePos;
	}
	// ��ȡʣ��ɶ��ڴ�����
	inline size_t readableSize() {
		return m_writePos - m_readPos;
	}
	// д�ڴ棺ֱ��д
	int appendString(const char* data);
	int appendString(const char* data, size_t size);
	int appendString(const string data);
	// д�ڴ棺�����׽��ֵ�����
	ssize_t socketRead(int fd);
	// ����\r\nȡ��һ�����ݣ��ҵ��������ݿ��е�λ��
	char* findCRLF();
	// ��������
	ssize_t sendData(int socket);
	// ��ȡ�����ݵ���ʼλ��
	char* data() {
		return m_data + m_readPos;
	}
	// ���¶�ָ��λ��
	inline size_t readPosIncrease(size_t count) {
		m_readPos += count;
		return m_readPos;
	}

private:	
	char* m_data;  // ָ����ڴ��ָ�룬����/��������ʱ�Ƚ����ݱ��浽�ö��ڴ���
	size_t m_capacity; // ��¼data���ݿ������
	size_t m_readPos = 0; // ������λ�ã������dataָ�����ʼ��ַ
	size_t m_writePos = 0; // д����λ�ã������dataָ�����ʼ��ַ
};


