#ifndef TAXIORDER_H
#define TAXIORDER_H

#include <QtCore>

#include "hello.pb.h"
#include "qgeocoordinate.h"

#define NO_ORDER_ID -1

class ITaxiOrder : public QObject
{
	Q_OBJECT

public:
	ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, QObject *parent);
	~ITaxiOrder();

	void setRegionId(int _region_id);
	int getRegionId();
	int getOrderId();
	void setOrderId(int _order_id);
	int calculateSum();
	TaxiRatePeriod orderTaxiRate() { return taxiRate; }
	double mileage();
	int minutesStops() { return (seconds_stops + 30) / 60; }
	int minutesTotal() { return (seconds_travelled + 30) / 60; }
	int minutesMoving() { return (seconds_moving + 30) / 60; }

signals:
	void paymentChanged(int);
	void regionChanged(int regionId);
	void newMileage(float mileage);
	void newTimeMovement(int seconds);
	void newTimeStops(int seconds);
	void newTimeTotal(int seconds);

public slots:
	void recalcSum();
	void newPosition(QGeoCoordinate);
	void startOrder();
	void stopOrder();
	void measureTimes();
	void movementStart(int startStop);

private:
	TaxiRatePeriod taxiRate;
	
	// meters
	float distance_travelled;
	// seconds, ����� ����� �������
	int seconds_travelled;
	// ��� id ��� ����
	int order_id;
	// ���� ����, id
	int region_id;
	bool started;

	QString addressText;
	
	QTimer *paymentTimer;
	
	QGeoCoordinate currentPosition;
	bool gotPosition;

	int seconds_moving;
	int seconds_stops;

	int current_stop;

	bool movementStarted;
};

#endif // TAXIORDER_H
