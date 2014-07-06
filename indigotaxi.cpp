#include "indigotaxi.h"
#include <QtGui>
#include <QtCore>

#include "windows.h"
#ifdef UNDER_CE
// ��� ����������
#include "Pm.h"
#endif

#include "backend.h"
#include "voicelady.h"

/* main version string! */
static const char *version = "0.0.9";
int const IndigoTaxi::EXIT_CODE_REBOOT = -123456789;

IndigoTaxi::IndigoTaxi(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), iTaxiOrder(NULL), satellitesUsed(0), movementStarted(false),
	newDirection(false)
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
	connect(backend, SIGNAL(newSpeed(int)), SLOT(newSpeed(int)));
	connect(backend, SIGNAL(newSatellitesUsed(int)), SLOT(newSatellitesUsed(int)));
	connect(backend, SIGNAL(movementStart(int)), SLOT(movementStart(int)));
	//settingsForm->setBackend(backend);

	settingsIniFile = new QSettings("indigotaxi.ini", QSettings::IniFormat, this);
	settingsIniFile->beginGroup("main");
	int driverName = settingsIniFile->value("driverName", QVariant(500)).toInt();
	settingsIniFile->endGroup();
	backend->setDriverName(driverName);
	

	//ui.versionLabel->setText(version);

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
    QString text = time.toString("hh:mm");
    ui.timeLabel->setText(text);

	updateTaxiRates();
}

void IndigoTaxi::settingsButtonClick()
{
	//emit reboot_application();
	//rebootApp();
	ui.stackedWidget->setCurrentWidget(ui.settingsPage4);
}

void IndigoTaxi::moveToClient() 
{
	backend->sendOrderEvent(hello_TaxiEvent_MOVE_TO_CLIENT, iTaxiOrder);
}

void IndigoTaxi::inPlace()
{
	backend->sendOrderEvent(hello_TaxiEvent_IN_PLACE, iTaxiOrder);
}

void IndigoTaxi::startClientMove()
{
	// ���� ����� ���������� �� �����
	if (iTaxiOrder == NULL) {
		// ����� ����������� ���������
		iTaxiOrder = createTaxiOrder(NO_ORDER_ID);
	}

	// ������� ������
	iTaxiOrder->setRegionId(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_id());
	// �������� �������
	if (newDirection) {
		backend->sendOrderEvent(hello_TaxiEvent_MOVED, iTaxiOrder);
	} else {
		backend->sendOrderEvent(hello_TaxiEvent_START_CLIENT_MOVE, iTaxiOrder);
		
		// ����� ����
		iTaxiOrder->startOrder();
	}
	
	// ��������� �����
	iTaxiOrder->recalcSum();
	// �� ������ ������ ����� �����, ���� ����
	ui.directionValueButton->setText("�����������\n" +
		QString::fromUtf8(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_name().c_str()));
	// ����� ������
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);

	if (!newDirection) {
		voiceLady->sayPhrase("GREETING");
	}

	newDirection = false;
}

// � ������� ��� ���-�� ������, ���� �����������
void IndigoTaxi::protobuf_message(hello message)
{
	// ������ ������ �������� ������
	if (message.text_string().length() > 0)
		ui.serverMessage->setPlainText(QString::fromUtf8(message.text_string().c_str()));

	// ����� ������ �������� ������
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


/**
 * ���������� �� ������ �������� ������ �� �����
 *
 */
void IndigoTaxi::updateTaxiRates()
{
	for (int i = 0; i < taxiRates.rates_size(); i++) {
		//qDebug() << "Car In:" << taxiRates.rates().Get(i).car_in() <<
		//	"KM_G:" << taxiRates.rates().Get(i).km_g();
	}
	TaxiRatePeriod period = getCurrentTaxiRatePeriod();
	ui.car_in_label->setText(QString("%1 ���.").arg(period.car_in(), 0, 'f', 1));
	ui.km_g_label->setText(QString("%1 ���.").arg(period.km_g(), 0, 'f', 1));
}

/**
 * ������ ����� ������, ��������� ��� ������� � ������� 
 *
 */
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
		voiceLady->sayPhrase("CONNECTIONOK");
		ui.connectionLabel->setPixmap(QPixmap(":/UI/images/connection-ok.png"));
		//ui.connectionLabel->setText("��������");
	} else {
		voiceLady->sayPhrase("NOCONNECTION");		
		ui.connectionLabel->setPixmap(QPixmap(":/UI/images/connection-bad.png"));
		//ui.connectionLabel->setText("��� ����������");
	}
}

void IndigoTaxi::rebootApp()
{
	// �������� �����
	restartMutex.lock();
	//delete backend;
	QString filePath = QApplication::applicationFilePath();
	QStringList args = QApplication::arguments();
	QString workingDir = QDir::currentPath();
	bool result = QProcess::startDetached(filePath, args, workingDir);

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
	// TODO ��������� ����������� ����������
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





// ������
void IndigoTaxi::paytimeClick() 
{
	// stop accounting
	if (iTaxiOrder != NULL) {
		iTaxiOrder->stopOrder();
		int payment = iTaxiOrder->calculateSum();

		ui.finalPaymentAmountLabel->setText(QString("%1 ���.").arg(payment));
		voiceLady->speakMoney(payment);
		ui.stackedWidget->setCurrentWidget(ui.paytimePage3);
	}
}

// �������� -- ����� ������������
void IndigoTaxi::freeButtonClick()
{
	if (iTaxiOrder != NULL) {
		voiceLady->sayPhrase("BYE");
		backend->sendOrderEvent(hello_TaxiEvent_END_CLIENT_MOVE, iTaxiOrder);
		destroyCurrentOrder();
	}
	ui.serverMessage->setPlainText("");
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}

// ���������� �������
void IndigoTaxi::resumeVoyageClick()
{
	iTaxiOrder->startOrder();
	ui.stackedWidget->setCurrentWidget(ui.orderPage2);
}

// ��������
void IndigoTaxi::clearMessageClick()
{	
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

// ���������
void IndigoTaxi::backToStandByClick()
{
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}


/*!
 * \brief
 * �� �����, �� �����
 * 
 * \param pressed
 * ������ -- �� �����, ������ -- �� �����.
 * 
 * Write detailed description for dutyButtonClicked here.
 * 
 * \remarks
 * Write remarks for dutyButtonClicked here.
 * 
 * \see
 * Separate items with the '|' character.
 */
void IndigoTaxi::dutyButtonClicked(bool pressed)
{
	if (pressed) {
		backend->sendEvent(hello_TaxiEvent_ARRIVED);
		ui.dutyStart->setText("����� �����");
	} else {
		backend->sendEvent(hello_TaxiEvent_DAY_END);
		ui.dutyStart->setText("������ �����");
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



/*!
 * \brief
 * ����� �� ������
 * 
 * \throws <exception class>
 * Description of criteria for throwing this exception.
 * 
 * Write detailed description for notToMeButtonClicked here.
 * 
 * \remarks
 * Write remarks for notToMeButtonClicked here.
 * 
 * \see
 * Separate items with the '|' character.
 */
void IndigoTaxi::notToMeButtonClicked()
{
	backend->sendOrderEvent(hello_TaxiEvent_NOT_TO_ME, iTaxiOrder);		
	ui.serverMessage->setPlainText("");
}

// ������� �� �������
void IndigoTaxi::selectRegionClicked() 
{
	if (!newDirection && satellitesUsed < 5) {
		voiceLady->sayPhrase("NOGPS");
		QMessageBox::critical(this, "���������� ������ �������", "����� ��������� ������ ���� ������ 5");
		return;
	}

	ui.stackedWidget->setCurrentWidget(ui.regionListPage6);
}

// ����� ������ �����
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

// ����� �����, � ������� ��� ���
ITaxiOrder *IndigoTaxi::createTaxiOrder(int order_id) 
{
	ITaxiOrder *iTaxiOrder = new ITaxiOrder(order_id, getCurrentTaxiRatePeriod(), this);
	connect(backend, SIGNAL(newPosition(QGeoCoordinate)), iTaxiOrder, SLOT(newPosition(QGeoCoordinate)));
	connect(iTaxiOrder, SIGNAL(newMileage(float)), SLOT(newMileage(float)));
	connect(iTaxiOrder, SIGNAL(paymentChanged(int)), SLOT(newPaymentCalculated(int)));

	iTaxiOrder->recalcSum();

	return iTaxiOrder;
}

void IndigoTaxi::destroyCurrentOrder()
{
	if (iTaxiOrder != NULL) {
		disconnect(iTaxiOrder, 0, 0, 0);
		disconnect(backend, 0, iTaxiOrder, 0);
		delete iTaxiOrder;
		iTaxiOrder = NULL;
	}
}

// ������ ����� � �������, ���� �����������
void IndigoTaxi::handleNewOrder(TaxiOrder taxiOrder)
{
	qDebug() << "Order ID:" << taxiOrder.order_id();
	// ����� � �����
	if (iTaxiOrder != NULL && iTaxiOrder->getOrderId() == NO_ORDER_ID) {
		iTaxiOrder->setOrderId(taxiOrder.order_id());
		// ���-�� ��� ���������
	} else if (iTaxiOrder != NULL) {
		destroyCurrentOrder();

		iTaxiOrder = createTaxiOrder(taxiOrder.order_id());

		if (taxiOrder.has_address()) {
			ui.serverMessage->setPlainText(QString::fromUtf8(taxiOrder.address().c_str()));
		}
	}
}

void IndigoTaxi::newPaymentCalculated(int payment)
{
	ui.paymentAmountLabel->setText(QString("%1 ���.").arg(payment));
}

void IndigoTaxi::newSpeed(int speed_kmh)
{
	qDebug() << "newSpeed" << speed_kmh;
	ui.speedValueLabel->setText(QString("%1 ��/�").arg(speed_kmh));
}

void IndigoTaxi::newMileage(float mileage)
{
	qDebug() << "newMileage" << mileage;
	ui.distanceValueLabel->setText(QString("%1 ��").arg(mileage, 0, 'f', 1));
}
void IndigoTaxi::newSatellitesUsed(int _satellitesUsed)
{
	satellitesUsed = _satellitesUsed;
}

void IndigoTaxi::movementStart(int start)
{
	qDebug() << (start ? "movement START" : "movement STOP");
	movementStarted = start == 1;

	if (movementStarted) {
		// FIXME ��������� ����� ��������
		ui.trainCrossButton->setEnabled(false);
		ui.trainCrossButton->setChecked(false);
	} else {
		ui.trainCrossButton->setEnabled(true);
	}
}


// ����� �������, ���������� �� ���
void IndigoTaxi::trainCrossButtonClicked()
{
	voiceLady->sayPhrase("TRAINCROSS");
	ui.trainCrossButton->setEnabled(false);
}

void IndigoTaxi::overloadButtonClicked()
{
	voiceLady->sayPhrase("OVERLOAD");
	// fixme some messages, maybe
}

void IndigoTaxi::newDirectionClicked()
{
	newDirection = true;

	selectRegionClicked();
}

void IndigoTaxi::cancelRegionSelectClicked()
{
	// ���� �������� ����� ����� ����������, ������������ � ������. �����, � ������ ��������
	if (!newDirection) {
		ui.stackedWidget->setCurrentWidget(ui.standByPage1);
	} else {
		ui.stackedWidget->setCurrentWidget(ui.orderPage2);
		newDirection = false;
	}
}

void IndigoTaxi::rebootSystem()
{
	// reboot
#ifdef UNDER_CE
	SetSystemPowerState(NULL, POWER_STATE_RESET, 0);
#endif
}