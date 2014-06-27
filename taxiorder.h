#ifndef TAXIORDER_H
#define TAXIORDER_H

#include <QObject>

class TaxiOrder : public QObject
{
	Q_OBJECT

public:
	TaxiOrder(QObject *parent);
	~TaxiOrder();

private:
	
};

#endif // TAXIORDER_H
