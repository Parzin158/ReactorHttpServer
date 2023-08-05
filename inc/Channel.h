#pragma once
#include <functional>
using namespace std;


// ���庯��ָ��
// typedef int (*handleFunc)(void* arg);
// using handleFunc = int(*)(void*);

// ����fd�Ķ�д�¼�events
enum class FDEvent {
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};


class Channel {
public:
	// ʹ�ÿɵ��ö����װ���������1.����ָ�� 2. �ɵ��ö��󣨿�����һ��ʹ�ã������յõ��˵�ַ������δ����
	using handleFunc = std::function<int(void*)>;
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg);
	/* 	����1�����ڼ�����lfd  ����2��ָ�����lfd���¼�����  ����3���������¼�������  
		����4������д�¼�������  ����5��ɾ���¼�������  ����6���ص���������	 */
	// �ص�������Ϊ�ɵ��ö����װ������
	handleFunc readCallback;
	handleFunc writeCallback;
	handleFunc destroyCallback;
	/* �޸�fd��д�¼������or����⣩��flagΪtrue������Ϊд�¼���flagΪfalse������Ϊ���¼� */
	void writeEventEnable(bool flag);
	/* �ж��Ƿ���Ҫ���fd��д�¼� */
	bool isWriteEventEnable();
	/* ȡ��˽�г�Ա */
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
	// �ļ�������
	int m_fd;
	// �¼�
	int m_events;
	// �ص���������
	void* m_arg;
};

