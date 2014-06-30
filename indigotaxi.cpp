#include "indigotaxi.h"
#include <QtGui>
#include <QtCore>

#include "windows.h"
#ifdef UNDER_CE
#include "Pm.h"
#endif

#include "backend.h"
#include "voicelady.h"

/* main version string! */
static const char *version = "0.0.8";
int const IndigoTaxi::EXIT_CODE_REBOOT = -123456789;

IndigoTaxi::IndigoTaxi(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), iTaxiOrder(NULL)
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

	connect(backend, SIGNAL(newSpeed(int)), SLOT(newSpeed(int)));

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

	voiceLady = new VoiceLady(this);
	iSoundPlayer = new ISoundPlayer();
	soundThread = new QThread(this);
	soundThread->start();

	iSoundPlayer->moveToThread(soundThread);

	connect(voiceLady, SIGNAL(playSound(QString)), iSoundPlayer, SLOT(playResourceSound(QString)));


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

	updateTaxiRates();
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
	//qDebug() << waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, MAKELONG( 0xFFFFFFFFF, 0xFFFFFFFF));
//	qDebug() << QFile::exists("alarm.wav");
	// QSound::play("alarm.wav");
	//int flags = SND_FILENAME|SND_ASYNC; 
//	int flags = SND_FILENAME; 
	//qDebug() << PlaySoundW(L"alarm.wav", 0, flags);
	
//	MessageBeep(MB_OK);
	backend->sendOrderEvent(hello_TaxiEvent_MOVE_TO_CLIENT, iTaxiOrder);
}

void IndigoTaxi::inPlace()
{
	backend->sendOrderEvent(hello_TaxiEvent_IN_PLACE, iTaxiOrder);
}

void IndigoTaxi::startClientMove()
{
	if (iTaxiOrder == NULL) {
		// заказ инициирован водителем
		iTaxiOrder = new ITaxiOrder(NO_ORDER_ID, getCurrentTaxiRatePeriod(), this);
	}
	iTaxiOrder->setRegionId(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_id());
	backend->sendOrderEvent(hello_TaxiEvent_START_CLIENT_MOVE, iTaxiOrder);
	iTaxiOrder->startOrder();
	iTaxiOrder->recalcSum();
	ui.directionValueLabel->setText(
		QString::fromUtf8(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_name().c_str()));
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);
}

void IndigoTaxi::protobuf_message(hello message)
{
	// old
	if (message.text_string().length() > 0)
		ui.serverMessage->setPlainText(QString::fromUtf8(message.text_string().c_str()));

	// new
	if (message.has_taxiorder()) {
		handleNewOrder(message.taxiorder());		
	}

	if (message.has_taxirate()) {
		taxiRates = message.taxirate();
		updateTaxiRates();
	}

	if (message.has_taxiregionlist()) {
		taxiRegionList = message.taxiregionlist();
		updateTaxiRegionList();
	}
}

void IndigoTaxi::updateTaxiRates()
{
	for (int i = 0; i < taxiRates.rates_size(); i++) {
		qDebug() << "Car In:" << taxiRates.rates().Get(i).car_in() <<
			"KM_G:" << taxiRates.rates().Get(i).km_g();
	}
	TaxiRatePeriod period = getCurrentTaxiRatePeriod();
	ui.car_in_label->setText(QString("%1 руб.").arg(period.car_in(), 0, 'f', 1));
	ui.km_g_label->setText(QString("%1 руб.").arg(period.km_g(), 0, 'f', 1));
}

void IndigoTaxi::updateTaxiRegionList()
{
	ui.regionList->clear();
	for (int i = 0; i < taxiRegionList.regions_size(); i++) {
		ui.regionList->addItem(QString::fromUtf8(taxiRegionList.regions().Get(i).region_name().c_str()));
	}
	ui.regionList->setCurrentRow(0);
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
	if (iTaxiOrder != NULL) {
		iTaxiOrder->stopOrder();
		ui.finalPaymentAmountLabel->setText(QString("%1 руб.").arg(iTaxiOrder->calculateSum(), 0, 'f', 1));
	}
	ui.stackedWidget->setCurrentWidget(ui.paytimePage3);
}

void IndigoTaxi::freeButtonClick()
{
	if (iTaxiOrder != NULL) {
		backend->sendOrderEvent(hello_TaxiEvent_END_CLIENT_MOVE, iTaxiOrder);
		delete iTaxiOrder;
	}
	ui.serverMessage->setPlainText("");
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}

void IndigoTaxi::resumeVoyageClick()
{
	iTaxiOrder->startOrder();
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);
}

void IndigoTaxi::clearMessageClick()
{
	
	voiceLady->speakMoney(1117);
	voiceLady->speakMoney(2222);
	
	voiceLady->speakMoney(1012);
	voiceLady->speakMoney(512);
	voiceLady->speakMoney(102);
	voiceLady->speakMoney(150);	
	voiceLady->speakMoney(117);
	voiceLady->speakMoney(572);
	voiceLady->speakMoney(331);	
	voiceLady->speakMoney(30);
	
	//SetSystemPowerState(NULL, POWER_STATE_RESET, 0);
	//QSound::play("click.wav");
	//QSound::play(qApp->applicationDirPath() + QDir::separator() + "stop.wav");
	if (iTaxiOrder != NULL) {
		delete iTaxiOrder;
		iTaxiOrder = NULL;
	}
	
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
	if (iTaxiOrder != NULL) {
		backend->sendOrderEvent(hello_TaxiEvent_NOT_TO_ME, iTaxiOrder);		
		ui.serverMessage->setPlainText("");
	}
}

void IndigoTaxi::selectRegionClicked() 
{
	ui.stackedWidget->setCurrentWidget(ui.regionListPage6);
}

TaxiRatePeriod IndigoTaxi::getCurrentTaxiRatePeriod() {
	int i = 0;
	QTime currentTime = QTime::currentTime();
	int hour = currentTime.hour();
	int minute = currentTime.minute();
	int minutes = hour * 60 + minute;
	for (i = 0; i < taxiRates.rates_size(); i++) {
		TaxiRatePeriod period = taxiRates.rates().Get(i);
		int begin_minutes = period.begin_hour() * 60 + period.begin_minute();
		int end_minutes = period.end_hour() * 60 + period.end_minute();

		if (minutes >= begin_minutes && minutes < end_minutes) 
			return period;
	}

	qDebug() << "rate period not found!!!";
	return TaxiRatePeriod::default_instance();
}

void IndigoTaxi::handleNewOrder(TaxiOrder taxiOrder)
{
	qDebug() << "Order ID:" << taxiOrder.order_id();
	if (iTaxiOrder != NULL)
		delete iTaxiOrder;

	iTaxiOrder = new ITaxiOrder(taxiOrder.order_id(), getCurrentTaxiRatePeriod(), this);
	connect(backend, SIGNAL(newPosition(QGeoCoordinate)), iTaxiOrder, SLOT(newPosition(QGeoCoordinate)));
	connect(iTaxiOrder, SIGNAL(newMileage(float)), SLOT(newMileage(float)));
	connect(iTaxiOrder, SIGNAL(paymentChanged(float)), SLOT(newPaymentCalculated(float)));

	iTaxiOrder->recalcSum();

	if (taxiOrder.has_address()) {
		ui.serverMessage->setPlainText(QString::fromUtf8(taxiOrder.address().c_str()));
	}
}

void IndigoTaxi::newPaymentCalculated(float payment)
{
	int value = (int) floor(payment + 0.5);
	ui.paymentAmountLabel->setText(QString("%1 руб.").arg(value));
}

void IndigoTaxi::newSpeed(int speed_kmh)
{
	qDebug() << "newSpeed" << speed_kmh;
	ui.speedValueLabel->setText(QString("%1 км/ч").arg(speed_kmh));
}

void IndigoTaxi::newMileage(float mileage)
{
	qDebug() << "newMileage" << mileage;
	ui.distanceValueLabel->setText(QString("%1 км").arg(mileage, 0, 'f', 1));
}
