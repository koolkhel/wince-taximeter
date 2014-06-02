#include "indigoqueue.h"
#include <QtCore>

IndigoQueue::~IndigoQueue(void)
{
}

void IndigoQueue::pushAll(QByteArray &array) 
{
	foreach (char a, array)
	{
		push(a);
	}
}

void IndigoQueue::push(char data)
{
    queue.enqueue(data);
}

char IndigoQueue::pop()
{
    if(queue.size() <= 0)
    {
        // You should handle Queue underflow the way you want here.
		qDebug() << "queue underflow!!!";
        return -1;
    }

	return queue.dequeue();
}

char IndigoQueue::peek()
{
    if(queue.size() <= 0)
    {
        // You should handle Queue underflow the way you want here.
		qDebug() << "queue underflow!!!";
        return -1;
    }

    return queue.head();
}

bool IndigoQueue::isEmpty()
{
    return queue.size() == 0;
}

int IndigoQueue::size() 
{
	return queue.size();
}