#ifndef TAXIORDER_H
#define TAXIORDER_H

#include <QtCore>

#include "hello.pb.h"
#include "qgeocoordinate.h"

class ITaxiOrder : public QObject
{
	Q_OBJECT

public:
	ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, QObject *parent);
	~ITaxiOrder();

	void setRegionId(int _region_id);
	int getRegionId();
	int getOrderId();
	float calculateSum();

signals:
	void paymentChanged(float payment);
	void regionChanged(int regionId);

public slots:
	void recalcSum();
	void newGeoPosition(const QGeoCoordinate newPosition);
	void startOrder();
	void stopOrder();

private:
	TaxiRatePeriod taxiRate;
	
	// meters
	float distance_travelled;
	// seconds
	float seconds_travelled;
	// наш id для базы
	int order_id;
	// куда едем, id
	int region_id;
	bool started;

	QString addressText;
	
	QTimer *paymentTimer;
	
	QGeoCoordinate currentPosition;
	bool gotPosition;
};

#endif // TAXIORDER_H
