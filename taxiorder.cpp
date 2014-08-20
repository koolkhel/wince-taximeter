#include "taxiorder.h"

ITaxiOrder::ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, float _parkingCost, int _parkingId, QObject *parent)
	: QObject(parent), 
	
	_order_id(_order_id), 
	
	_mileage_city(0), 
	
	_total_travel_time_seconds(0), 
	seconds_stops(0), seconds_moving(0), seconds_client_stops(0), seconds_traincross_stops(0),
	
	_out_of_city_rate(0),
	
	_destination_region_id(0),
	taxiRate(_taxiRate), 
	
	gotPosition(false), 
	
	started(false), movementStarted(false), _totalTimeStarted(false),
	
	parkingCost(_parkingCost), parkingId(_parkingId),
	current_stop_seconds(0), 
	
	outOfCity(false),

	_clientStop(false),_overload(false),_trainCross(false),
	
	_mileage_out_of_city(0), _mileage_city_overload(0), _mileage_out_of_city_overload(0)
{
	qDebug() << "newOrder id:" << _order_id << "rate:" << taxiRate.car_in() << " " << taxiRate.km_g();
	paymentTimer = new QTimer(this);
	
	connect(paymentTimer, SIGNAL(timeout()), SLOT(measureTimes()));
	connect(paymentTimer, SIGNAL(timeout()), SLOT(recalcSum()));
	paymentTimer->setInterval(1000);
	paymentTimer->start();
}

ITaxiOrder::~ITaxiOrder()
{

}

void ITaxiOrder::measureTimes()
{
	// ���� �� �� �������� � �.�. ��� �� ����� ����� ����� �������,
	// �� ������ ����� ����, ��� ������ �������� �������
	if (_totalTimeStarted) {
		_total_travel_time_seconds++;
		emit newTimeTotal(_total_travel_time_seconds);
	}

	// ����� ���� �� �����, ���� �� �������
	if (!started)
		return;

	

	if (!movementStarted && !_clientStop) {
		// ��������� ���������, ���� ������ 30 ������
		if (current_stop_seconds < 30) {
			current_stop_seconds++;
		} else {
			seconds_stops++;
		}
	} else {
		current_stop_seconds = 0;
		seconds_moving++;
	}

	if (_clientStop)
	{
		seconds_client_stops++;
	}

	emit newTimeMovement(seconds_moving);
	emit newTimeStops(seconds_stops);
	emit newTimeClientStops(seconds_client_stops);
}

void ITaxiOrder::setRegionId(int _region_id)
{
	_destination_region_id = _region_id;
	emit regionChanged(_destination_region_id);
}

int ITaxiOrder::getRegionId()
{
	return _destination_region_id;
}

double ITaxiOrder::totalMileage()
{
	return cityMileage() + outOfCityMileage();
}

double ITaxiOrder::moneyCity()
{	
	double mileage_city_cost = cityMileage() * taxiRate.km_g();	
	double mileage_city_overload_cost = cityMileageOverload() * taxiRate.km_g() * 1.5;

	return mileage_city_cost + mileage_city_overload_cost;
}

double ITaxiOrder::moneyMg()
{
	double mileage_out_of_city_cost = outOfCityMileage() * mgRate();
	double mileage_out_of_city_overload_cost = outOfCityMileageOverload() * mgRate() * 1.5;

	return mileage_out_of_city_cost + mileage_out_of_city_overload_cost;
}

/* ============================================================= */
int ITaxiOrder::calculateSum()
{
	// ������ ������ -- � ������� �������
	double car_in = getCarIn();;
		
	// ��������� �� ������� �������
	double client_stops = minutesClientStops() * taxiRate.client_stop();

	// ������
	double stops = taxiRate.car_min() * 0.5 * minutesStops();

	double value = car_in 
		+ moneyCity() + moneyMg()
		+ stops + client_stops;
	
	// ��������� ����� � ����������
	return ((int)((value + 0.5) * 10.0)) / 10;
}
/* ============================================================= */

void ITaxiOrder::recalcSum()
{
	float payment = calculateSum();
	emit paymentChanged(payment);
}

void ITaxiOrder::movementStart(int startStop)
{
	if (startStop) {
		// ������ �����
		movementStarted = true;
	} else {
		// ���������
		movementStarted = false;
	}
}

void ITaxiOrder::setTrainCross(bool on)
{
	_trainCross = on;
	if (on) {
		stopOrder();
	} else {
		startOrder();
	}
}

void ITaxiOrder::newPosition(QGeoCoordinate newPosition)
{
	if (!started)
		return;
	
	if (gotPosition) {
		if (outOfCity) {
			if (_overload) {
				_mileage_out_of_city_overload += newPosition.distanceTo(currentPosition);
			} else {
				_mileage_out_of_city += newPosition.distanceTo(currentPosition);
			}
		} else {
			if (_overload) {
				_mileage_city_overload += newPosition.distanceTo(currentPosition);
			} else {
				_mileage_city += newPosition.distanceTo(currentPosition);
			}
		}
		//emit newMileage((_mileage_out_of_city + _mileage_city + _mileage_city_overload + _mileage_out_of_city_overload) / 1000.0);
		// ��� ��������� �� 100 ������ � �������
		emit newMileage(cityMileage() + outOfCityMileage() + cityMileageOverload() + outOfCityMileageOverload());
		recalcSum();
	}
	
	currentPosition = newPosition;
	gotPosition = true;

}

void ITaxiOrder::startOrder()
{
	started = true;
}

void ITaxiOrder::stopOrder()
{
	started = false;
}

int ITaxiOrder::getOrderId()
{
	return _order_id;
}

void ITaxiOrder::setOrderId(int order_id)
{
	_order_id = order_id;
}