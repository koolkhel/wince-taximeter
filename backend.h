#ifndef BACKEND_H
#define BACKEND_H

#include <QtCore>
#include <QtNetwork/QTcpSocket>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <google/protobuf/text_format.h>

#include "hello.pb.h"
#include "indigoqueue.h"

#include "qnmeapositioninfosource.h"

#include <QObject>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class Backend : public QObject
{
	Q_OBJECT

public:
	Backend(QObject *parent);
	~Backend();
	void Backend::setDriverName(int _driverName);

signals:
	void protobuf_message(hello message);
	void connectedToServer(bool status);
	
public slots:
	// from gps
	void positionUpdated(const QGeoPositionInfo &update);

	void sendEvent(hello_TaxiEvent event);

	// from timer
	void sendLocationData();
	
	void readyRead();
	void connected();
	void disconnected();
	void reconnect();

private:
	void consumeSocketData();

	QTcpSocket *socket;
	IndigoQueue receiveBuffer;

	enum receive_state_t
	{
		NOTHING = 0,
		MESSAGE_RECEIVING, // -> NOTHING
	};

	receive_state_t receive_state;
	int index;
	google::protobuf::uint8 message_buffer[2048];
	google::protobuf::uint32 message_length;
	const google::protobuf::uint8 *message_start;
	int remainder;

	void send_message(hello &var);

	int driverName;

	QTimer *gpsTimer;
	QNmeaPositionInfoSource *positionSource;

	QSerialPort *gpsSerialPort;
};

#endif // BACKEND_H