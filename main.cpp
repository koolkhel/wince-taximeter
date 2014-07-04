#include "indigotaxi.h"
#include <QtGui/QApplication>

#include <QtNetwork/QTcpSocket>

#include <QtCore>
#include <QtGui>

#include "hello.pb.h"

#include "InputDevice/inputdevice.h"

#include "backend.h"
#include "logger.h"

IndigoTaxi *mainWindow = NULL;

void myMessageOutput(QtMsgType type, const char *msg)
 {
	 //in this function, you can write the message to any stream!
	 switch (type) {
	 case QtDebugMsg:
		 //fprintf(stderr, "Debug: %s\n", msg);
		 if (strchr(msg, '\n')) {
			fprintf(stderr, "%s", msg);
		 } else {
			 fprintf(stderr, "%s\n", msg);
		 }

         break;
     case QtWarningMsg:
         fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
//         abort();
     }
 }

Logger *logger = NULL;

void LoadFont(QString name)
{
	QByteArray segoe;
	QResource r(name);
	if (r.isValid()) {
		QByteArray b( reinterpret_cast< const char* >( r.data() ), r.size() );
		qDebug() << "font" << name;
		QFontDatabase::addApplicationFontFromData(b);
	}
}

int main(int argc, char *argv[])
{	
	int retcode = 0;
	bool result = 
#ifdef UNDER_CE	
	QResource::registerResource("\\ResidentFlash\\IndigoTaxi\\sound.rcc");	
	QResource::registerResource("\\ResidentFlash\\IndigoTaxi\\UI.rcc");	
#else
	QResource::registerResource("C:\\sound.rcc");
	QResource::registerResource("C:\\UI.rcc");
#endif
	qDebug() << "sounds loaded:" << result;
	qDebug() << "font status:" << QResource(":/IndigoTaxi/segoeui.ttf").isValid();
	qDebug() << "font bold status:" << QResource(":/IndigoTaxi/segoeuib.ttf").isValid();
	
		
	qInstallMsgHandler(myMessageOutput);
	
	QApplication a(argc, argv);
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("CP1251"));

		
	LoadFont(":/IndigoTaxi/segoeui.ttf");
	LoadFont(":/IndigoTaxi/segoeuib.ttf");

	// for virtual keyboard
	InputDevice inputdevice;
	
	/*
	Debug: "Windows" 
Debug: "WindowsCE" 
Debug: "WindowsMobile" 
*/
#ifdef UNDER_CE
		QApplication::setStyle("WindowsCE");
#else
		QApplication::setStyle("Windows");
#endif
	
		IndigoTaxi w;
		mainWindow = &w;

		
#ifdef UNDER_CE
		QRect rect = QApplication::desktop()->geometry();
		int width = rect.width();
		int height = rect.height();
		int dpi = (int) sqrt(width*width + height*height) / 4.5; // average screen size
		qDebug() << "calculated DPI:" << dpi;
		w.setProperty("_q_customDpiX", QVariant(dpi));
		w.setProperty("_q_customDpiY", QVariant(dpi));
		w.show();
		w.showFullScreen();
#else
		w.setProperty("_q_customDpiX", QVariant(122));
		w.setProperty("_q_customDpiY", QVariant(122));
		w.show();
#endif

		qDebug() << "screen height:" << QApplication::desktop()->heightMM() << "width:" << QApplication::desktop()->widthMM();
		qDebug() << "physical screen dpi height:" << w.physicalDpiY() << "width:" << w.physicalDpiX();
		//qDebug() << "logical screen dpi height:" << w.logicalDpiY() << "width:" << w.logicalDpiX();

		retcode = a.exec();	

		return retcode;

	
#if 0
	QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
	QStringList results;
	const QString blankString = QObject::tr("N/A");
	QString description;
    QString manufacturer;
    QString serialNumber;
	qDebug() << "Hello, world!";

	foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList) {
        description = serialPortInfo.description();
        manufacturer = serialPortInfo.manufacturer();
        serialNumber = serialPortInfo.serialNumber();
		
        qDebug() << endl
            << QObject::tr("Port: ") << serialPortInfo.portName() << endl
            << QObject::tr("Location: ") << serialPortInfo.systemLocation() << endl
            << QObject::tr("Description: ") << (!description.isEmpty() ? description : blankString) << endl
            << QObject::tr("Manufacturer: ") << (!manufacturer.isEmpty() ? manufacturer : blankString) << endl
            << QObject::tr("Serial number: ") << (!serialNumber.isEmpty() ? serialNumber : blankString) << endl
            << QObject::tr("Vendor Identifier: ") << (serialPortInfo.hasVendorIdentifier() ? QByteArray::number(serialPortInfo.vendorIdentifier(), 16) : blankString) << endl
            << QObject::tr("Product Identifier: ") << (serialPortInfo.hasProductIdentifier() ? QByteArray::number(serialPortInfo.productIdentifier(), 16) : blankString) << endl
            << QObject::tr("Busy: ") << (serialPortInfo.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) << endl;

	
	}
#endif
	
	
}
