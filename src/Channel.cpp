#include "Channel.h"
#include <stdlib.h>


/* 初始化一个Channel，返回一个Channel类型的地址，给对应函数进行操作 */
Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	m_arg = arg;
	m_fd = fd;
	m_events = (int)events;
	readCallback = readFunc;
	writeCallback = writeFunc;
	destroyCallback = destroyFunc;
}

/* 修改fd的写事件（检测or不检测），flag为true则设置为写事件，flag为false则设置为读事件 */
void Channel::writeEventEnable(bool flag)
{
	if (flag) {
		// flag为1，设置为写事件
		m_events |= static_cast<int>(FDEvent::WriteEvent); // 异或，将events中第三个标志位置为1
	}
	else {
		// flag为0，是读事件，对写事件标记清零
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent); // 取反1111 1011，再按位与
	}
}

/* 判断是否需要检测fd的写事件 */
bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent);
}
