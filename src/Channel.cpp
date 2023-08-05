#include "Channel.h"
#include <stdlib.h>


/* ��ʼ��һ��Channel������һ��Channel���͵ĵ�ַ������Ӧ�������в��� */
Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	m_arg = arg;
	m_fd = fd;
	m_events = (int)events;
	readCallback = readFunc;
	writeCallback = writeFunc;
	destroyCallback = destroyFunc;
}

/* �޸�fd��д�¼������or����⣩��flagΪtrue������Ϊд�¼���flagΪfalse������Ϊ���¼� */
void Channel::writeEventEnable(bool flag)
{
	if (flag) {
		// flagΪ1������Ϊд�¼�
		m_events |= static_cast<int>(FDEvent::WriteEvent); // ��򣬽�events�е�������־λ��Ϊ1
	}
	else {
		// flagΪ0���Ƕ��¼�����д�¼��������
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent); // ȡ��1111 1011���ٰ�λ��
	}
}

/* �ж��Ƿ���Ҫ���fd��д�¼� */
bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent);
}
