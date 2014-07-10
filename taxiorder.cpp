#include "taxiorder.h"

ITaxiOrder::ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, QObject *parent)
	: QObject(parent), _order_id(_order_id), _mileage_city(0), _total_travel_time_seconds(0), _destination_region_id(0),
	taxiRate(_taxiRate), gotPosition(false), started(false), movementStarted(false), 
	current_stop(0), seconds_stops(0), seconds_moving(0),
	outOfCity(false), _mileage_out_of_city(0)
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
	// заказ либо на паузе, либо не начался
	if (!started)
		return;

	_total_travel_time_seconds++;

	if (!movementStarted) {
		// остановка считается, если больше 5 секунд
		if (current_stop < 5) {
			current_stop++;
		} else {
			if (seconds_stops < 5)
				seconds_stops = 5;
			seconds_stops++;
		}
	} else {
		current_stop = 0;
		seconds_moving++;
	}

	emit newTimeMovement(seconds_moving);
	emit newTimeStops(seconds_stops);
	emit newTimeTotal(_total_travel_time_seconds);
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

/* ============================================================= */
int ITaxiOrder::calculateSum()
{
	// стоимость поездки -- это стоимость подачи
	double car_in = taxiRate.car_in();
	double mileage_city_cost = cityMileage() * taxiRate.km_g();
	double mileage_out_of_city_cost = outOfCityMileage() * mgRate();

	// округление вверх на полминуте
	double stops = taxiRate.car_min() * 0.5 * minutesStops();

	double value = car_in + mileage_city_cost + mileage_out_of_city_cost + stops;
	
	// округляем рубли к ближайшему
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
		// начали ехать
		movementStarted = true;
	} else {
		// остановка
		movementStarted = false;
	}
}

void ITaxiOrder::newPosition(QGeoCoordinate newPosition)
{
	if (!started)
		return;
	
	if (gotPosition) {
		if (outOfCity) {
			_mileage_out_of_city += newPosition.distanceTo(currentPosition);
		} else {
			_mileage_city += newPosition.distanceTo(currentPosition);
		}
		emit newMileage((_mileage_out_of_city + _mileage_city) / 1000.0);
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

void ITaxiOrder::setOrderId(int _order_id)
{
	_order_id = _order_id;
}