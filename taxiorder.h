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
	ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, float _parkingCost, int _parkingId, QObject *parent);
	~ITaxiOrder();

	void setRegionId(int _region_id);
	int getRegionId();
	int getOrderId();
	void setOrderId(int _order_id);
	int calculateSum();
	TaxiRatePeriod orderTaxiRate() { return taxiRate; }
	double totalMileage();
	// ����������� ����������� �� ����� ������
	float cityMileage() { return (((int)_mileage_city + 50) / 100) / 10.0; }
	float outOfCityMileage() { return (((int)_mileage_out_of_city + 50) / 100) / 10.0; }
	float cityMileageOverload() { return (((int)_mileage_city_overload + 50) / 100) / 10.0; }
	float outOfCityMileageOverload() { return (((int)_mileage_out_of_city_overload + 50) / 100) / 10.0; }
	void setOverload(bool overload) { _overload = overload; }
	void setClientStop(bool clientStop) { _clientStop = clientStop; }
	
	
	float mgRate() { return _out_of_city_rate;}
	int getParkingId() { return parkingId; }
	float getParkingCost() { return parkingCost; }
	int minutesClientStops() {return (seconds_client_stops + 30) / 60;}
	int minutesStops() { return (seconds_stops + 30) / 60; }
	int minutesTotal() { return (_total_travel_time_seconds + 30) / 60; }
	int minutesMoving() { return (seconds_moving + 30) / 60; }

signals:
	void paymentChanged(int);
	void regionChanged(int regionId);
	void newMileage(float mileage);
	void newTimeMovement(int seconds);
	void newTimeStops(int seconds);
	void newTimeClientStops(int seconds);
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
		_out_of_city_rate = _out_of_city_rate;
	}

private:	
	// IMPORTANT ORDER VARIABLES (USED FOR MONEY COMPUATION)	
	TaxiRatePeriod taxiRate;
	
	// ��������� ��������� � ���������
	float _out_of_city_rate;
	// meters in da city
	float _mileage_city;
	// meters out of city
	float _mileage_out_of_city;

	float _mileage_city_overload;
	float _mileage_out_of_city_overload;
	
	// seconds, ����� ����� �������
	int _total_travel_time_seconds;

	// ��� id ��� ����
	int _order_id;
	// ���� ����, id
	int _destination_region_id;
	
	// true -- ��� ����, false -- �� ���
	bool started;

	// ����� ����� � ��������
	int seconds_moving;

	// ����� � �������
	int seconds_stops;

	int seconds_client_stops;
	
	float parkingCost; 
	
	int parkingId;

	// END IMPORTANT ORDER VARIABLES
	

	QString addressText;
	
	QTimer *paymentTimer;
	
	QGeoCoordinate currentPosition;
	bool gotPosition;


	int current_stop_seconds;

	bool outOfCity;

	bool movementStarted;

	bool _overload;

	bool _clientStop;
};

#endif // TAXIORDER_H
