#pragma once
#include <functional>
using namespace std;


// 定义函数指针
// typedef int (*handleFunc)(void* arg);
// using handleFunc = int(*)(void*);

// 定义fd的读写事件events
enum class FDEvent {
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};


class Channel {
public:
	// 使用可调用对象包装器，打包：1.函数指针 2. 可调用对象（可像函数一样使用），最终得到了地址，但是未调用
	using handleFunc = std::function<int(void*)>;
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);
	/* 	参数1：用于监听的lfd  参数2：指定检测lfd的事件类型  参数3：触发读事件处理函数  
		参数4：触发写事件处理函数  参数5：删除事件处理函数  参数6：回调函数参数	 */
	// 回调函数，为可调用对象包装器类型
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	/* 修改fd的写事件（检测or不检测），flag为true则设置为写事件，flag为false则设置为读事件 */
	void writeEventEnable(bool flag);
	/* 判断是否需要检测fd的写事件 */
	bool isWriteEventEnable();
	/* 取出私有成员 */
	inline int getSocket() {
		return m_fd;
	}
	inline int getEvent() {
		return m_events;
	}
	inline const void* getArg() {
		return m_arg;
	}

private:
	// 文件描述符
	int m_fd;
	// 事件
	int m_events;
	// 回调函数参数
	void* m_arg;
};

