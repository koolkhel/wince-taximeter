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
	void setOutOfCity(bool _outOfCity) {
		outOfCity = _outOfCity;
	}

private:
	TaxiRatePeriod taxiRate;
	
	// IMPORTANT ORDER VARIABLES (USED FOR MONEY COMPUATION)
	// meters in da city
	float mileage_city;

	// meters out of city
	float mileage_out_of_city;
	
	// seconds, общее время поездки
	int seconds_travelled;

	// наш id для базы
	int order_id;
	// куда едем, id
	int destination_region_id;
	
	// true -- идёт счёт, false -- не идёт
	bool started;

	// END IMPORTANT ORDER VARIABLES
	

	QString addressText;
	
	QTimer *paymentTimer;
	
	QGeoCoordinate currentPosition;
	bool gotPosition;

	int seconds_moving;
	int seconds_stops;

	int current_stop;

	bool outOfCity;

	bool movementStarted;
};

#endif // TAXIORDER_H
