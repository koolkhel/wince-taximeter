#include "backend.h"
#include "logger.h"

#define SERVER_ADDRESS "87.117.17.221"
//#define SERVER_ADDRESS "192.168.91.1"
// #define SERVER_ADDRESS "indigosystem.ru"
#define SERVER_PORT 9099
#define GPS_SEND_INTERVAL (5 * 1000)

Backend::Backend(QObject *parent)
: QObject(parent),
receive_state(NOTHING),
message_length(0),
taxiId(4)
{
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(readyRead()), SLOT(readyRead()), Qt::QueuedConnection);
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(disconnected()));
	connect(socket, SIGNAL(connected()), SLOT(connected()));
	connect(socket, SIGNAL(connected()), SLOT(flushOrderEvents()));
	connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));

	positionSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
	connect(positionSource, SIGNAL(positionUpdated(const QGeoPositionInfo &)), SLOT(positionUpdated(const QGeoPositionInfo &)));

// FIXME port number aquire
#ifdef UNDER_CE
	QString registryKey = "HKEY_LOCAL_MACHINE\\Drivers\\BuiltIn\\GPS";
	QSettings registry(registryKey, QSettings::NativeFormat);
	QString portIndex = registry.value("index").toString();
	QString portPrefix = registry.value("Prefix").toString();	
	QString portName = portPrefix + portIndex;
	// COM5 COM2
	portName = "COM1";
#else
	QString portName = "COM7";
#endif

	gpsSerialPort = new QSerialPort(this);
	// qint32 baudRate = QSerialPort::Baud38400;
	qint32 baudRate = QSerialPort::Baud38400;
	
	gpsSerialPort->setPortName(portName);
	gpsSerialPort->setBaudRate(baudRate, QSerialPort::Input);
	gpsSerialPort->setBaudRate(baudRate, QSerialPort::Output);

	if (!gpsSerialPort->open(QIODevice::ReadWrite)) {
		qDebug() << QObject::tr("Failed to open port %1, error: %2").arg(portName).arg(gpsSerialPort->errorString()) << endl;
		
	} else {
		positionSource->setDevice(gpsSerialPort);
		positionSource->startUpdates();

	}

	gpsTimer = new QTimer(this);
	gpsTimer->setSingleShot(false);
	gpsTimer->setInterval(GPS_SEND_INTERVAL);
	connect(gpsTimer, SIGNAL(timeout()), SLOT(sendLocationData()));
	gpsTimer->start();

	reconnect();
}

Backend::~Backend()
{

}

void Backend::setDriverName(int _driverName) {
	driverName = _driverName;
	emit driverNameChanged(driverName);
}

// Static.
using namespace google::protobuf::io;
// i know it's bad
#ifndef UNDER_CE
int CodedInputStream::default_recursion_limit_ = 100;
#endif

// remove if any problems with big protobuf
namespace google {
	namespace protobuf {
		namespace internal {
			const string kEmptyString;
		}
	}
}

inline const google::protobuf::uint8* ReadVarint32FromArray(const google::protobuf::uint8* buffer, google::protobuf::uint32* value) {
	static const int kMaxVarintBytes = 10;
	static const int kMaxVarint32Bytes = 5;

	// Fast path:  We have enough bytes left in the buffer to guarantee that
	// this read won't cross the end, so we can skip the checks.
	const google::protobuf::uint8* ptr = buffer;
	google::protobuf::uint32 b;
	google::protobuf::uint32 result;

	b = *(ptr++); result  = (b & 0x7F)      ; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) <<  7; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) << 14; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |= (b & 0x7F) << 21; if (!(b & 0x80)) goto done;
	b = *(ptr++); result |=  b         << 28; if (!(b & 0x80)) goto done;

	// If the input is larger than 32 bits, we still need to read it all
	// and discard the high-order bits.
	for (int i = 0; i < kMaxVarintBytes - kMaxVarint32Bytes; i++) {
		b = *(ptr++); if (!(b & 0x80)) goto done;
	}

	// We have overrun the maximum size of a varint (10 bytes).  Assume
	// the data is corrupt.
	return NULL;

done:
	*value = result;
	return ptr;
}

void Backend::readyRead()
{
	qDebug() << "ready read" << socket->bytesAvailable() << "bytes";

	QByteArray data = socket->readAll();
	receiveBuffer.pushAll(data);
	consumeSocketData();
}

void Backend::connected()
{
	qDebug() << "connected";
	emit connectedToServer(true);
}

void Backend::disconnected()
{
	qDebug() << "disconnected";
	emit connectedToServer(false);
	QTimer::singleShot(5000, this, SLOT(reconnect()));
}

void Backend::error(QAbstractSocket::SocketError &error)
{
	qDebug() << "error" << socket->errorString();
}

void Backend::reconnect()
{
	socket->connectToHost(SERVER_ADDRESS, SERVER_PORT);
}

void Backend::sendEvent(hello_TaxiEvent event)
{
	hello var;
	var.set_drivername(driverName);
	var.set_taxiid(taxiId);
	var.set_event(event);

	orderEventsQueue.push(var);
	flushOrderEvents();
}

void Backend::sendOrderEvent(hello_TaxiEvent event, ITaxiOrder *order)
{
	hello var;

	if (order == NULL)
		return;
	
	var.set_drivername(driverName);
	var.set_taxiid(taxiId);
	var.set_event(event);

	TaxiOrder *pbOrder = var.mutable_taxiorder();

	pbOrder->set_order_id(order->getOrderId());

	switch (event) {
		case hello_TaxiEvent_END_CLIENT_MOVE:
			pbOrder->set_money((int) floor(order->calculateSum() + 0.5));
			break;
		case hello_TaxiEvent_START_CLIENT_MOVE:
		case hello_TaxiEvent_MOVED:
			pbOrder->set_destination_region_id(order->getRegionId());
			break;
		default:
			break;
	}	

	orderEventsQueue.push(var);
	flushOrderEvents();
}

// "��������" ��������
void Backend::flushOrderEvents()
{		
	while (!orderEventsQueue.isEmpty()) {
		if (socket->state() == QTcpSocket::ConnectedState) {
			hello var = orderEventsQueue.peek();
			
			char buffer[1024];
			google::protobuf::io::ArrayOutputStream arr(buffer, sizeof(buffer));
			google::protobuf::io::CodedOutputStream output(&arr);

			output.WriteVarint32(var.ByteSize());
			var.SerializeToCodedStream(&output);

			socketMutex.lock();
			qint64 result = socket->write(buffer, output.ByteCount());
			socket->flush();
			socketMutex.unlock();
			if (result != -1) {
				orderEventsQueue.pop();
			} else {
				break;
			}

		} else {
			break;
		}
	}
}


// ��������� �� ����� ����������, ���� ��� �����
void Backend::send_message(hello &var)
{
	char buffer[1024];
	google::protobuf::io::ArrayOutputStream arr(buffer, sizeof(buffer));
	google::protobuf::io::CodedOutputStream output(&arr);

	var.set_drivername(driverName);
	var.set_taxiid(taxiId);

	output.WriteVarint32(var.ByteSize());
	var.SerializeToCodedStream(&output);

	if (socket->state() == QTcpSocket::ConnectedState) {
		socketMutex.lock();
		socket->write(buffer, output.ByteCount());
		socket->flush();
		socketMutex.unlock();
	}
}

#if 0
void Backend::protobuf_message_slot(const google::protobuf::uint8* begin, google::protobuf::uint32 length)
{
//	ui.serverMessage->setPlainText(QString::fromUtf8(var.text_string().c_str()));
	logger->addLine(QString::fromUtf8(var.text_string().c_str()));
}
#endif

#define PROBABLE_VARINT32_SIZE 8
void Backend::consumeSocketData()
{

	int varint32_byte_count = 0;


	forever {
		switch (receive_state) {
		case NOTHING:
			if (receiveBuffer.size() < PROBABLE_VARINT32_SIZE)
				return;

			index = 0;
			message_length = 0;
			memset(message_buffer, 0, sizeof(message_buffer));
			// pop succeeds every time
			for (int i = 0; i < PROBABLE_VARINT32_SIZE; i++) {
				message_buffer[index++] = receiveBuffer.pop();
			}

			message_start = ReadVarint32FromArray(message_buffer, &message_length);
			varint32_byte_count = message_start - &message_buffer[0];
			remainder = message_length - (PROBABLE_VARINT32_SIZE - varint32_byte_count);
			receive_state = MESSAGE_RECEIVING;

			break;
		case MESSAGE_RECEIVING:
			int count = receiveBuffer.size();
			while (remainder > 0) {
				if (count == 0)
					return;
				message_buffer[index++] = receiveBuffer.pop();
				remainder--;
				count--;
			}

			hello var;

			google::protobuf::io::ArrayInputStream arrayIn(message_start, message_length);
			google::protobuf::io::CodedInputStream codedIn(&arrayIn);	
			google::protobuf::io::CodedInputStream::Limit msgLimit = codedIn.PushLimit(message_length);
			var.ParseFromCodedStream(&codedIn);
			codedIn.PopLimit(msgLimit);

			// ��������� �� ��������
			emit protobuf_message(var);

			receive_state = NOTHING;
			break;
		}
	}
}

void Backend::positionUpdated(const QGeoPositionInfo &update) 
{
	// ���� ����� ����������� ������, ����� �������� �����������. � ������ ���������� � ����� �� ����, ��� ��� -- ��� ��������, ��� ��?
	
	if (update.hasAttribute(QGeoPositionInfo::SatellitesUsed)) {
		qDebug() << "satellites used:" << update.attribute(QGeoPositionInfo::SatellitesUsed);

		emit newSatellitesUsed((int) update.attribute(QGeoPositionInfo::SatellitesUsed));
	}

	if (update.isValid()) {
		qDebug() << "longitude" << update.coordinate().longitude() << "latitude" << update.coordinate().latitude();
		positionMessage.set_longitude(update.coordinate().longitude());
		positionMessage.set_latitude(update.coordinate().latitude());
		
		if (update.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
			int speed = (int) (update.attribute(QGeoPositionInfo::GroundSpeed) * 3.6);
			positionMessage.set_speed_kmh(speed);
			emit newSpeed(speed);
			detectStartStop(speed);
		}

		emit newPosition(update.coordinate());
	}
}

void Backend::sendLocationData()
{
	// ��� ����� -- �� ����������
	send_message(positionMessage);	
}

void Backend::detectStartStop(int speed)
{
	// FIXME ����� ���� ������ �� �����������, ����� �������

	static int startThreshold = 0;
	static int stopThreshold = 0;
	
	if (speed > 5) {
		if (startThreshold >= 3) {			
			emit movementStart(1);
		} else {
			startThreshold++;
		}
		stopThreshold = 0;
	} else {
		if (stopThreshold >= 3) {
			emit movementStart(0);
		} else {			
			stopThreshold++;
		}
		startThreshold = 0;
		
	}
}