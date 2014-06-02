#pragma once

#include <QtCore/QQueue>
#include <QtCore/QByteArray>
/**
	* ����� ����� ��� ����, ����� ������������ ����� � ������ ������� ������� � ���������������
	* ������� TcpSocket�/QByteArray. ������� �� char
	*
	*
	*/

class IndigoQueue
{
private:
	QQueue<char> queue;

public:
	IndigoQueue(void)
	{}
	~IndigoQueue(void);

	void push(char data);
	void pushAll(QByteArray &array);
    char pop();
    char peek();
	int size();
    bool isEmpty();
};
