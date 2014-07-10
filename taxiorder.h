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
	double totalMileage();
	// километражи округл€ютс€ до сотен метров
	float cityMileage() { return (((int)_mileage_city + 50) / 100) / 10.0; }
	float outOfCityMileage() { return (((int)_mileage_out_of_city + 50) / 100) / 10.0; }
	float mgRate() { return out_of_city_rate;}
	int minutesStops() { return (seconds_stops + 30) / 60; }
	int minutesTotal() { return (_total_travel_time_seconds + 30) / 60; }
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
	void setMg(float _out_of_city_rate)
	{
		out_of_city_rate = _out_of_city_rate;
	}

private:	
	// IMPORTANT ORDER VARIABLES (USED FOR MONEY COMPUATION)	
	TaxiRatePeriod taxiRate;
	
	// стоимость километра в межгороде
	float out_of_city_rate;
	// meters in da city
	float _mileage_city;

	// meters out of city
	float _mileage_out_of_city;
	
	// seconds, общее врем€ поездки
	int _total_travel_time_seconds;

	// наш id дл€ базы
	int _order_id;
	// куда едем, id
	int _destination_region_id;
	
	// true -- идЄт счЄт, false -- не идЄт
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
