#include "taxiorder.h"

ITaxiOrder::ITaxiOrder(int _order_id, TaxiRatePeriod _taxiRate, QObject *parent)
	: QObject(parent), order_id(_order_id), distance_travelled(0), seconds_travelled(0), region_id(0),
	taxiRate(_taxiRate), gotPosition(false), started(false)
{
	qDebug() << "newOrder id:" << order_id << "rate:" << taxiRate.car_in() << " " << taxiRate.km_g();
	paymentTimer = new QTimer(this);
	connect(paymentTimer, SIGNAL(timeout()), SLOT(recalcSum()));
	paymentTimer->setInterval(1000);
	//paymentTimer->start();
}

ITaxiOrder::~ITaxiOrder()
{

}

void ITaxiOrder::setRegionId(int _region_id)
{
	region_id = _region_id;
	emit regionChanged(region_id);
}

int ITaxiOrder::getRegionId()
{
	return region_id;
}

float ITaxiOrder::calculateSum()
{
	// стоимость поездки -- это стоимость подачи
	double value = taxiRate.car_in() + distance_travelled / 1000.0 * taxiRate.km_g();
	return QString("%1").arg(value, 0, 'f', 1).toFloat();
}

void ITaxiOrder::recalcSum()
{
	float payment = calculateSum();
	emit paymentChanged(payment);
}

void ITaxiOrder::newPosition(QGeoCoordinate newPosition)
{
	qDebug() << "ITaxiOrder::newPosition";
	if (started) {
		if (gotPosition) {
			distance_travelled += newPosition.distanceTo(currentPosition);
			emit newMileage(distance_travelled / 1000.0);
			recalcSum();
		}
		currentPosition = newPosition;
		gotPosition = true;
	}
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
	return order_id;
}