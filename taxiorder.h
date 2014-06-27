#ifndef TAXIORDER_H
#define TAXIORDER_H

#include <QtCore>

#include "hello.pb.h"

class ITaxiOrder : public QObject
{
	Q_OBJECT

public:
	ITaxiOrder(QObject *parent);
	~ITaxiOrder();

signals:
	void paymentChanged(float payment);
public slots:

private:
	float distance_travelled;
	float seconds_travelled;
	int order_id;
	int region_id;

	QString addressText;
	
};

#endif // TAXIORDER_H
