#pragma once
#include <string>

using namespace std;

class Buffer {
public:
	Buffer(size_t size);
	~Buffer();

	// 扩容
	void extendRoom(size_t size);
	// 获取剩余可写内存容量
	inline size_t writeableSize() {
		return m_capacity - m_writePos;
	}
	// 获取剩余可读内存容量
	inline size_t readableSize() {
		return m_writePos - m_readPos;
	}
	// 写内存：直接写
	int appendString(const char* data);
	int appendString(const char* data, size_t size);
	int appendString(const string data);
	// 写内存：接收套接字的数据
	ssize_t socketRead(int fd);
	// 根据\r\n取出一行数据，找到其在数据块中的位置
	char* findCRLF();
	// 发送数据
	ssize_t sendData(int socket);
	// 获取读数据的起始位置
	char* data() {
		return m_data + m_readPos;
	}
	// 更新读指针位置
	inline size_t readPosIncrease(size_t count) {
		m_readPos += count;
		return m_readPos;
	}

private:	
	char* m_data;  // 指向堆内存的指针，发送/接收数据时先将数据保存到该堆内存中
	size_t m_capacity; // 记录data数据块的容量
	size_t m_readPos = 0; // 读数据位置，相对于data指针的起始地址
	size_t m_writePos = 0; // 写数据位置，相对于data指针的起始地址
};


