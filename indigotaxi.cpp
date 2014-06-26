#include "indigotaxi.h"
#include <QtGui>
#include <QtCore>

#include "windows.h"

#include "backend.h"

/* main version string! */
static const char *version = "0.0.7";
int const IndigoTaxi::EXIT_CODE_REBOOT = -123456789;

IndigoTaxi::IndigoTaxi(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
#ifdef UNDER_CE
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#endif
	setAttribute(Qt::WA_QuitOnClose);

	//settingsForm = new SettingsForm(this);
	//settingsForm->hide();

	backend = new Backend(this);
	
	connect(backend, SIGNAL(protobuf_message(hello)), SLOT(protobuf_message(hello)));
	connect(backend, SIGNAL(connectedToServer(bool)), SLOT(connectionStatus(bool)));
	connect(backend, SIGNAL(driverNameChanged(int)), SLOT(driverNameChanged(int)));
	//settingsForm->setBackend(backend);

	settingsIniFile = new QSettings("indigotaxi.ini", QSettings::IniFormat, this);
	settingsIniFile->beginGroup("main");
	int driverName = settingsIniFile->value("driverName", QVariant(500)).toInt();
	settingsIniFile->endGroup();

	backend->setDriverName(driverName);

	ui.versionLabel->setText(version);

	connect(this, SIGNAL(reboot_application()), SLOT(rebootApp()));

	QString versionUrl = "http://indigosystem.ru/taxi-version.txt";
	QUrl version(versionUrl);
	downloader = new FileDownloader(version, this);
	connect(downloader, SIGNAL(downloaded()), SLOT(checkVersion()));

	ui.driverNameLineEdit->setProperty("keyboard",true); //enable the keyboard (this is a custom property)
	QValidator *validator = new QIntValidator(0, 500, this);
	ui.driverNameLineEdit->setValidator(validator); //add a int validator min value 0 max value 500. This will force the numpad to show, you can also use a QDoubleValidator

	timeTimer = new QTimer(this);
	connect(timeTimer, SIGNAL(timeout()), SLOT(updateTime()));
	timeTimer->setInterval(1000);
	timeTimer->setSingleShot(false);
	timeTimer->start();

	//ui.driverNameLineEdit->setProperty("keyboard",true); // enable the keyboard. when there is no validator set the keyboard will show
	//aTextLineEdit->setProperty("maxLength",25); //this can be used to limit the length of the string
}
		
IndigoTaxi::~IndigoTaxi()
{

}

void IndigoTaxi::updateTime()
{
	QTime time = QTime::currentTime();    
    QString text = time.toString("hh:mm:ss");
    ui.timeLabel->setText(text);
}

void IndigoTaxi::settingsButtonClick()
{
	//emit reboot_application();
	//rebootApp();
	ui.stackedWidget->setCurrentWidget(ui.settingsPage4);
}

void IndigoTaxi::showHideLog() 
{
}

void IndigoTaxi::moveToClient() 
{
	backend->sendEvent(hello_TaxiEvent_MOVE_TO_CLIENT);
}

void IndigoTaxi::inPlace()
{
	backend->sendEvent(hello_TaxiEvent_IN_PLACE);
}

void IndigoTaxi::startClientMove()
{
	backend->sendEvent(hello_TaxiEvent_START_CLIENT_MOVE);
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);
}

void IndigoTaxi::protobuf_message(hello message)
{
	if (message.text_string().length() > 0)
		ui.serverMessage->setPlainText(QString::fromUtf8(message.text_string().c_str()));
}

void IndigoTaxi::connectionStatus(bool status)
{
	if (status) {
		ui.connectionLabel->setText("Соединён");
	} else {
		ui.connectionLabel->setText("Нет соединения");
	}
}

void IndigoTaxi::rebootApp()
{
	restartMutex.lock();
	delete backend;
	QString filePath = QApplication::applicationFilePath();
	QStringList args = QApplication::arguments();
	QString workingDir = QDir::currentPath();
	bool result = QProcess::startDetached(filePath, args, workingDir);
	qDebug() << "starting" << filePath << "workingDir:" << workingDir << "result:" << result;

	QApplication::exit(0);
}

void IndigoTaxi::checkVersion()
{
	QByteArray data = downloader->downloadedData();
	QString newVersionString = data.data();
	newVersionString = newVersionString.trimmed();

	QString oldVersionString = version;
	if (newVersionString.trimmed() > oldVersionString.trimmed()) {
		qDebug() << "new version:" << newVersionString << "old version:" << oldVersionString;
		QString newVersionUrlPath = "http://indigosystem.ru/IndigoTaxi.exe";
		QUrl url(newVersionUrlPath);
		downloader->deleteLater();
		downloader = new FileDownloader(url, this);
		connect(downloader, SIGNAL(downloaded()), SLOT(newVersionDownloaded()));
		//ui.versionLabel->setText(newVersionString.trimmed());
	}
}

void IndigoTaxi::newVersionDownloaded()
{
	QByteArray data = downloader->downloadedData();
	QFile currentExePath(QApplication::instance()->applicationFilePath());
	QFile downloadedFilePath(QApplication::instance()->applicationDirPath() + "/new_exe.exe");
	if (downloadedFilePath.open(QIODevice::ReadWrite)) {;
		qint64 writtenLen = downloadedFilePath.write(data);
		downloadedFilePath.close();

		if (writtenLen == data.length()) {
			QString oldFilePath = QApplication::instance()->applicationDirPath() + "/old_exe.exe";
			QFile::remove(oldFilePath);
			bool result = !currentExePath.rename(oldFilePath);	
			result |= !downloadedFilePath.rename(QApplication::instance()->applicationFilePath());

			emit reboot_application();
		}
	}
}

void IndigoTaxi::paytimeClick() 
{
	// stop accounting
	ui.stackedWidget->setCurrentWidget(ui.paytimePage3);
}

void IndigoTaxi::freeButtonClick()
{
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}

void IndigoTaxi::resumeVoyageClick()
{
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);
}

void IndigoTaxi::clearMessageClick()
{
	ui.serverMessage->setPlainText("");
}

void IndigoTaxi::exitButtonClick()
{
	qApp->quit();
}

void IndigoTaxi::backToStandByClick()
{
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}

void IndigoTaxi::dutyButtonClicked(bool pressed)
{
	if (pressed) {
		backend->sendEvent(hello_TaxiEvent_ARRIVED);
		ui.dutyStart->setText("Конец смены");
	} else {
		backend->sendEvent(hello_TaxiEvent_DAY_END);
		ui.dutyStart->setText("Начало смены");
	}
}

void IndigoTaxi::dinnerStartClicked()
{
	backend->sendEvent(hello_TaxiEvent_GO_DINNER);
	ui.settingsStackedWidget->setCurrentWidget(ui.dinnerPage2);
}

void IndigoTaxi::dinnerStopClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_DINNER);
	ui.settingsStackedWidget->setCurrentWidget(ui.mainSettingsPage1);
}

void IndigoTaxi::driverNameChanged(int driverName)
{
	ui.driverNumberButton->setText(QString::number(driverName));
	ui.driverNameLineEdit->setText(QString::number(driverName));
	settingsIniFile->beginGroup("main");
	settingsIniFile->setValue("driverName", QVariant(driverName));
	settingsIniFile->endGroup();
	settingsIniFile->sync();
}

void IndigoTaxi::driverNameEdited(QString newValue)
{
	backend->setDriverName(newValue.toInt());
}

void IndigoTaxi::awayButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_MOVE_OUT);
	ui.settingsStackedWidget->setCurrentWidget(ui.awayPage3);
}

void IndigoTaxi::awayEndButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_MOVE_OUT);
	ui.settingsStackedWidget->setCurrentWidget(ui.mainSettingsPage1);
}

void IndigoTaxi::fromcarButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_GO_FROM_CAR);
	ui.settingsStackedWidget->setCurrentWidget(ui.fromcarPage4);
}

void IndigoTaxi::fromcarEndButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_TO_CAR);
	ui.settingsStackedWidget->setCurrentWidget(ui.mainSettingsPage1);
}

void IndigoTaxi::notToMeButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_NOT_TO_ME);
	ui.serverMessage->setPlainText("");
}