#include "backend.h"
#include "logger.h"

#define GPS_SEND_INTERVAL (10 * 1000)

Backend::Backend(QObject *parent)
: QObject(parent),
receive_state(NOTHING),
message_length(0)
{
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(readyRead()), SLOT(readyRead()), Qt::QueuedConnection);
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(disconnected()));
	connect(socket, SIGNAL(connected()), SLOT(connected()));
	connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));

	positionSource = new QNmeaPositionInfoSource(QNmeaPositionInfoSource::RealTimeMode, this);
	connect(positionSource, SIGNAL(positionUpdated(const QGeoPositionInfo &)), SLOT(positionUpdated(const QGeoPositionInfo &)));

// FIXME port number aquire
#ifdef UNDER_CE
	QString portName = "COM7";
#else
	QString portName = "COM7";
#endif

	gpsSerialPort = new QSerialPort(this);
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
	gpsTimer->setInterval(GPS_SEND_INTERVAL);
	connect(gpsTimer, SIGNAL(timeout()), SLOT(sendLocationData()));
	gpsTimer->start();

	reconnect();

	driverName = 14;
}

Backend::~Backend()
{

}

void Backend::setDriverName(int _driverName) {
	driverName = _driverName;
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
	QTimer::singleShot(1000, this, SLOT(reconnect()));
}

void Backend::reconnect()
{
	socket->connectToHost("192.168.91.1", 9090);
	//socket->connectToHost("indigosystem.ru", 9090);
}

void Backend::sendEvent(hello_TaxiEvent event)
{
	hello var;
	var.set_drivername(driverName);
	var.set_event(event);

	send_message(var);
}

void Backend::send_message(hello &var)
{
	char buffer[1024];
	google::protobuf::io::ArrayOutputStream arr(buffer, sizeof(buffer));
	google::protobuf::io::CodedOutputStream output(&arr);

	var.set_drivername(driverName);
	var.set_taxiid(1); // FIXME

	output.WriteVarint32(var.ByteSize());
	var.SerializeToCodedStream(&output);

	socket->write(buffer, output.ByteCount());
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

			emit protobuf_message(var);

			receive_state = NOTHING;
			break;
		}
	}
}

void Backend::positionUpdated(const QGeoPositionInfo &update) 
{
	// надо будет фильтровать данные, чтобы скорость сохран€лась. ¬ разных сообщени€х еЄ может не быть, так что -- это проблема, что ли?
}

void Backend::sendLocationData()
{
	static hello var;
	
	QGeoPositionInfo info = positionSource->lastKnownPosition();
	if (info.isValid()) {
		var.set_longitude(info.coordinate().longitude());
		var.set_latitude(info.coordinate().latitude());
		if (info.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
			var.set_speed_kmh((int) (info.attribute(QGeoPositionInfo::GroundSpeed) * 3.6));
		}
	}

	send_message(var);
}