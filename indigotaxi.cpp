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

#define DEBUG

IndigoTaxi::IndigoTaxi(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags), iTaxiOrder(NULL), lastTaxiOrder(NULL), 
	satellitesUsed(0), movementStarted(false), currentParkingCost(0), currentParkingId(0),
	newDirection(false), online(false), downloader(NULL), changeRegion(false)
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
	backend->sendEvent(hello_TaxiEvent_GET_INFO);
	

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
	connect(voiceLady, SIGNAL(playSoundFile(QString)), iSoundPlayer, SLOT(playFileSystemSound(QString)));

	infoDialog = new IInfoDialog(this);
	confirmDialog = new IConfirmationDialog(this);

	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);

	//ui.driverNameLineEdit->setProperty("keyboard",true); // enable the keyboard. when there is no validator set the keyboard will show
	//aTextLineEdit->setProperty("maxLength",25); //this can be used to limit the length of the string
	int dpi = 122;
#ifdef UNDER_CE
	QRect rect = QApplication::desktop()->geometry();
	int width = rect.width();
	int height = rect.height();
	dpi = (int) sqrt(width*width + height*height) / 4.5; // average screen size
	qDebug() << "calculated DPI:" << dpi;
#endif

	setProperty("_q_customDpiX", QVariant(dpi));
	setProperty("_q_customDpiY", QVariant(dpi));
	infoDialog->setProperty("_q_customDpiX", QVariant(dpi));
	infoDialog->setProperty("_q_customDpiY", QVariant(dpi));
	confirmDialog->setProperty("_q_customDpiX", QVariant(dpi));
	confirmDialog->setProperty("_q_customDpiY", QVariant(dpi));

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
	voiceLady->sayPhrase("ORDERACCEPT");
}

void IndigoTaxi::inPlace()
{
	backend->sendOrderEvent(hello_TaxiEvent_IN_PLACE, iTaxiOrder);
	voiceLady->sayPhrase("ORDERINPLACE");
}

void IndigoTaxi::driverRegionSelectClicked()
{
	changeRegion = true;

	ui.stackedWidget->setCurrentWidget(ui.regionListPage6);
}

void IndigoTaxi::startClientMoveClicked()
{
	voiceLady->sayPhrase("ORDERGO");
}

void IndigoTaxi::startClientMove()
{
#ifndef DEBUG
	if (movementStarted)
		return;
#endif
	// ����� ������ ��������
	if (iTaxiOrder == NULL && changeRegion)
	{
		hello var;
		TaxiOrder *pbOrder = var.mutable_taxiorder();
		pbOrder->set_order_id(NO_ORDER_ID);
		pbOrder->set_destination_region_id(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_id());
		var.set_event(hello_TaxiEvent_MOVED);
		backend->sendMessageQueued(var);
		changeRegion = false;
		ui.stackedWidget->setCurrentWidget(ui.standByPage1);
		return;
	}

	// ���� ����� ���������� �� �����
	if (iTaxiOrder == NULL) {
		// ����� ����������� ���������
		iTaxiOrder = createTaxiOrder(NO_ORDER_ID);
	}

	float stopsTaxiRate = (floor((iTaxiOrder->orderTaxiRate().car_min() / 2.0) * 10)) / 10.0;
	float clientStopsTaxiRate = (floor((iTaxiOrder->orderTaxiRate().client_stop()) * 10)) / 10.0;
	ui.stopsTaxiRateLabel->setText(QString("%1/%2")
		.arg(clientStopsTaxiRate, 0, 'f', 1)
		.arg(stopsTaxiRate, 0, 'f', 1));

	float kmTaxiRate = iTaxiOrder->orderTaxiRate().km_g();
	float kmgTaxiRate = iTaxiOrder->mgRate();
	ui.kmgTaxiRateLabel->setText(QString("%1/%2")
		.arg(kmTaxiRate, 0, 'f', 1)
		.arg(kmgTaxiRate, 0, 'f', 1));

	float carInRate = iTaxiOrder->orderTaxiRate().car_in() + currentParkingCost;
	ui.finalCarInLabel ->setText(QString("%1").arg(carInRate, 0, 'f', 1));

	// ������� ������
	iTaxiOrder->setRegionId(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_id());
	// �������� �������
	if (newDirection) {
		backend->sendOrderEvent(hello_TaxiEvent_MOVED, iTaxiOrder);
	} else {
		if (iTaxiOrder->getOrderId() == NO_ORDER_ID) {
			backend->sendOrderEvent(hello_TaxiEvent_CLIENT_IN_PLACE, iTaxiOrder);
		} else
		{
			backend->sendOrderEvent(hello_TaxiEvent_START_CLIENT_MOVE, iTaxiOrder);
		}
		
		// ����� ����
		iTaxiOrder->startOrder();
	}
	
	// ��������� �����
	iTaxiOrder->recalcSum();
	// �� ������ ������ ����� �����, ���� ����
	ui.directionValueButton->setText("�����������\n" +
		QString::fromUtf8(taxiRegionList.regions().Get(ui.regionList->currentRow()).region_name().c_str()).toUpper());
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
	if (message.event() == hello_TaxiEvent_ABORT_ORDER) {
		abortOrder(message.taxiorder().order_id());
	}
#if 0
	if (message.text_string().length() > 0)
		ui.serverMessage->setPlainText(QString::fromUtf8(message.text_string().c_str()));
#endif
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

	if (message.has_taxiinfo()) {
		taxiInfo = message.taxiinfo();
		updateTaxiInfo();
	}
}

void IndigoTaxi::abortOrder(int order_id)
{
	if (iTaxiOrder->getOrderId() == order_id) {
		voiceLady->sayPhrase("ORDERABORT");
		infoDialog->info("����� �� ����� " + iTaxiOrder->address() + " ����ͨ� �����������");
		destroyCurrentOrder();
		ui.stackedWidget->setCurrentWidget(ui.standByPage1);
		clearMessageClick();
	}
}
	
void IndigoTaxi::intercity(int intercity)
{
	if (intercity) {
		ui.intercityLabel->setText("��������");
	} else {
		ui.intercityLabel->setText("�����");
	}
}

void IndigoTaxi::updateTaxiInfo()
{
	if (taxiInfo.out_of_city()) {
		intercity(1);
	
		qDebug() << "out of town";
	} else {
		intercity(0);
	
		qDebug() << "inside town" << taxiInfo.city_name().c_str();
	}
	
	if (taxiInfo.inside_parking()) {
		currentParkingCost = taxiInfo.parking_price();
		currentParkingId = taxiInfo.parking_id();

		qDebug() << "inside parking" << currentParkingId << "cost" << currentParkingCost;
	} else {
		currentParkingCost = 0;
		currentParkingId = 0;

		qDebug() << "outside parking";
	}
	

	if (iTaxiOrder == NULL)
		return;

	iTaxiOrder->setOutOfCity(taxiInfo.out_of_city());

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
	//ui.car_in_label->setText(QString("%1 ���.").arg(period.car_in() + currentParkingCost, 0, 'f', 1));
	ui.car_in_label->setText(QString("%1 ���.").arg(period.car_in()));
	ui.km_g_label->setText(QString("%1 ���.").arg(period.km_g(), 0, 'f', 1));
	ui.km_mg_label->setText(QString("%1 ���.").arg(taxiRates.mg(), 0, 'f', 1));
	
	float clientStopsTaxiRate = (floor((period.client_stop()) * 10)) / 10.0;
	float stopsTaxiRate = (floor((period.car_min() * 0.5) * 10)) / 10.0;
	ui.client_stop_label->setText(QString("%1/%2 ���.")
		.arg(clientStopsTaxiRate, 0, 'f', 1)
		.arg(stopsTaxiRate, 0, 'f', 1));
}

/**
 * ������ ����� ������, ��������� ��� ������� � ������� 
 *
 */
void IndigoTaxi::updateTaxiRegionList()
{
	// ������ ����� �� �������, ��� �����, �������, ��������
	enableWidget(ui.startClientMoveButton, true);

	ui.regionList->clear();
	for (int i = 0; i < taxiRegionList.regions_size(); i++) {
		ui.regionList->addItem(QString::fromUtf8(taxiRegionList.regions().Get(i).region_name().c_str()));
	}
	ui.regionList->setCurrentRow(0);
}

void IndigoTaxi::connectionStatus(bool status)
{
	if (status && !online) {
		voiceLady->sayPhrase("CONNECTIONOK");
		ui.connectionLabel->setPixmap(QPixmap(":/UI/images/connection-ok.png"));
		online = true;
	} else if (!status && online) {
		voiceLady->sayPhrase("NOCONNECTION");		
		ui.connectionLabel->setPixmap(QPixmap(":/UI/images/connection-bad.png"));
		online = false;
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
	if (iTaxiOrder == NULL)
		return;

#ifndef DEBUG
	if (movementStarted)
		return;
#endif
		
	iTaxiOrder->stopOrder();
	int payment = iTaxiOrder->calculateSum();

	ui.finalPaymentAmountLabel->setText(QString("%1�.").arg(payment));
	
	// ���������
	ui.finalMileageLabel->setText(QString("%1/%2")
		.arg(iTaxiOrder->cityMileage(), 0, 'f', 1)
		.arg(iTaxiOrder->outOfCityMileage(), 0, 'f', 1));
	
	ui.finalStopsTimeLabel->setText(QString("%1/%2")
		.arg(iTaxiOrder->minutesClientStops())
		.arg(iTaxiOrder->minutesStops()));
	ui.finalTotalTimeTravelledLabel->setText(QString("%1").arg(iTaxiOrder->minutesTotal()));

	voiceLady->speakMoney(payment);
	ui.stackedWidget->setCurrentWidget(ui.paytimePage3);
	
	voiceLady->sayPhrase("BYE");
}

// �������� -- ����� ������������
void IndigoTaxi::freeButtonClick()
{
	if (iTaxiOrder != NULL) {
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
	//if (iTaxiOrder != NULL) {
	//	destroyCurrentOrder();
	//	iTaxiOrder = NULL;
	//}
	
	ui.serverMessage->setPlainText("");
}

void IndigoTaxi::exitButtonClick()
{
	if (confirmDialog->ask("�� ������������� ����� �� ���������?")) {
		qApp->quit();
	}
}

// ���������
void IndigoTaxi::backToStandByClick()
{
	ui.stackedWidget->setCurrentWidget(ui.standByPage1);
}

void IndigoTaxi::enableWidget(QWidget *widget, bool enable)
{
	widget->setEnabled(enable);
	ui.centralWidget->style()->unpolish(widget);
	ui.centralWidget->style()->polish(widget);
	ui.centralWidget->update();
}

void IndigoTaxi::enableMainButtons(bool enable)
{
	enableWidget(ui.moveToClientButton, enable);
	enableWidget(ui.startClientMoveButton, enable);
}

void IndigoTaxi::enableInPlaceButton(bool enable)
{
	enableWidget(ui.inPlaceButton, enable);
}

// ���� �� ��� ������
void IndigoTaxi::playClick()
{
	voiceLady->click();
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
	if (!ui.dutyStart->property("pressed").toBool()) {
		if (confirmDialog->ask("�� ������������� ������ �����?")) {
			backend->sendEvent(hello_TaxiEvent_ARRIVED);
			enableMainButtons(true);
			ui.dutyStart->setText("����� �����");		
			ui.dutyStart->setProperty("pressed", true);
			infoDialog->info("����� ������!");
		}
		
	} else {
		if (confirmDialog->ask("�� ������������� ����� �����?")) {
			backend->sendEvent(hello_TaxiEvent_DAY_END);
			ui.dutyStart->setText("������ �����");
			enableMainButtons(false);
			ui.dutyStart->setProperty("pressed", false);
			infoDialog->info("����� ��������!");
		}
	}
	
	ui.dutyStart->style()->unpolish(ui.dutyStart);
	ui.dutyStart->style()->polish(ui.dutyStart);
	ui.dutyStart->update();
}

void IndigoTaxi::notPayClicked()
{
	if (confirmDialog->ask("�� �������������, ��� ������ �� ������� �������?")) {
		backend->sendOrderEvent(hello_TaxiEvent_NOT_PAY, lastTaxiOrder);
		infoDialog->info("���������� ���������� ����������� � �������� �������");
	}
}

void IndigoTaxi::dinnerStartClicked()
{
	backend->sendEvent(hello_TaxiEvent_GO_DINNER);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPageDinner2);
}

void IndigoTaxi::dinnerStopClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_DINNER);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);
}


void IndigoTaxi::driverNameChanged(int driverName)
{
	ui.driverNumberButton->setText("��������\n" + QString::number(driverName));
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
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPageAway4);
}

void IndigoTaxi::awayEndButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_MOVE_OUT);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);
}

void IndigoTaxi::fromcarButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_GO_FROM_CAR);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPageFromcar3);
}

void IndigoTaxi::fromcarEndButtonClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_TO_CAR);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);
}

void IndigoTaxi::emptyTripClicked() 
{
	// ���������
	if (confirmDialog->ask("�� ������������� ��������� �� ������?")) {
		backend->sendOrderEvent(hello_TaxiEvent_EMPTY_TRIP, iTaxiOrder);
		ui.stackedWidget->setCurrentWidget(ui.standByPage1);
		// ���������� �����
		destroyCurrentOrder();
		infoDialog->info("���������� ���������� ����������� � ���������");
	}
}

void IndigoTaxi::repairClicked()
{
	backend->sendEvent(hello_TaxiEvent_GET_DAMAGE);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPageRepair5);
}

void IndigoTaxi::repairEndClicked()
{
	backend->sendEvent(hello_TaxiEvent_REPEAR_DAMAGE);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);
}

void IndigoTaxi::techhelpClicked()
{
	backend->sendEvent(hello_TaxiEvent_TECHHELP);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPageTechhelp6);
}

void IndigoTaxi::techhelpBackClicked()
{
	backend->sendEvent(hello_TaxiEvent_BACK_TECHHELP);
	ui.driverCabinetSettingsStackWidget->setCurrentWidget(ui.driverCabinetPage1);
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
	if (confirmDialog->ask("�� ������������� ����� �� �������� ������?")) {
		backend->sendOrderEvent(hello_TaxiEvent_NOT_TO_ME, iTaxiOrder);
		destroyCurrentOrder();
		ui.serverMessage->setPlainText("");
		infoDialog->info("���������� ���������� ����������� �� ������ �� ������");
	}
}

// ������� �� �������
void IndigoTaxi::selectRegionClicked() 
{
#ifndef DEBUG
	if (!newDirection && satellitesUsed < 5) {
		voiceLady->sayPhrase("NOGPS");
		infoDialog->info("���������� ������ �������. ����� ��������� ������ ���� ������ 5");
		return;
	}
#endif
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
ITaxiOrder *IndigoTaxi::createTaxiOrder(int order_id, QString address) 
{
	ITaxiOrder *iTaxiOrder = new ITaxiOrder(order_id, getCurrentTaxiRatePeriod(), 
		currentParkingCost, currentParkingId, this);

	iTaxiOrder->setMg(taxiRates.mg());

	connect(backend, SIGNAL(newPosition(QGeoCoordinate)), iTaxiOrder, SLOT(newPosition(QGeoCoordinate)));
	connect(iTaxiOrder, SIGNAL(newMileage(float)), SLOT(newMileage(float)));
	connect(iTaxiOrder, SIGNAL(paymentChanged(int)), SLOT(newPaymentCalculated(int)));

	connect(iTaxiOrder, SIGNAL(newTimeTotal(int)), SLOT(newTimeTotal(int)));
	connect(iTaxiOrder, SIGNAL(newTimeStops(int)), SLOT(newTimeStops(int)));
	connect(iTaxiOrder, SIGNAL(newTimeMovement(int)), SLOT(newTimeMovement(int)));
	connect(iTaxiOrder, SIGNAL(newTimeClientStops(int)), SLOT(newTimeClientStops(int)));

	connect(this, SIGNAL(orderMovementStart(int)), iTaxiOrder, SLOT(movementStart(int)));

	iTaxiOrder->recalcSum();
	iTaxiOrder->setAddress(address);

	enableWidget(ui.moveToClientButton, true);

	return iTaxiOrder;
}

void IndigoTaxi::destroyCurrentOrder()
{
	if (iTaxiOrder != NULL) {
		enableInPlaceButton(false);
		disconnect(iTaxiOrder, 0, 0, 0);
		disconnect(backend, 0, iTaxiOrder, 0);
		//delete iTaxiOrder;
		if (lastTaxiOrder != NULL)
		{
			delete lastTaxiOrder;
			lastTaxiOrder = NULL;
		}

		lastTaxiOrder = iTaxiOrder;
		iTaxiOrder = NULL;

		enableWidget(ui.moveToClientButton, false);
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
	} else {
		destroyCurrentOrder();

		iTaxiOrder = createTaxiOrder(taxiOrder.order_id(), QString::fromUtf8(taxiOrder.address().c_str()));
		voiceLady->alarm();

		if (taxiOrder.has_address()) {
			ui.serverMessage->setPlainText(QString::fromUtf8(taxiOrder.address().c_str()));
		}
		enableInPlaceButton(true);
		ui.inPlaceButton->setEnabled(true);

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
	ui.gpsSatelliteCountLabel->setText(QString::number(satellitesUsed));
}

void IndigoTaxi::newTimeMovement(int _seconds)
{
}

void IndigoTaxi::newTimeStops(int _seconds)
{
	int hours = _seconds / 3600;
	int minutes = (_seconds % 3600) / 60;
	int seconds = _seconds % 60;	
	
	ui.timeStopsLabel->setText(QString("%1:%2:%3")
		.arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
}

void IndigoTaxi::newTimeClientStops(int _seconds)
{
	int hours = _seconds / 3600;
	int minutes = (_seconds % 3600) / 60;
	int seconds = _seconds % 60;	
	
	ui.timeClientStopsLabel->setText(QString("%1:%2:%3")
		.arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
	
}

void IndigoTaxi::newTimeTotal(int _seconds)
{
	int hours = _seconds / 3600;
	int minutes = (_seconds % 3600) / 60;
	int seconds = _seconds % 60;	
	
	ui.timeTotalLabel->setText(QString("%1:%2:%3")
		.arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));

}

void IndigoTaxi::clientNotExit()
{
	if (confirmDialog->ask("�� �������������, ��� ������ �� �����?")) {
		backend->sendOrderEvent(hello_TaxiEvent_NOT_EXIT, iTaxiOrder);
		infoDialog->info("���������� ���������� �����������, ��� ������ �� �����");
	}
}

void IndigoTaxi::clientStopClicked(bool on)
{
	if (iTaxiOrder == NULL)
		return;

	if (!movementStarted) {
		iTaxiOrder->setClientStop(on);

		if (on)
		{
			// ����� ��� ����� ����� ���-�� ������������� ��� ��������?
			//if (!iTaxiOrder->isStarted()) {
			//	iTaxiOrder->startOrder();
			//}
			voiceLady->sayPhrase("CLIENTSTOP");
			ui.clientStopButton->setEnabled(false);
		}
	}
}

void IndigoTaxi::movementStart(int start)
{
	qDebug() << (start ? "movement START" : "movement STOP");
	
	emit orderMovementStart(start);
	movementStarted = start == 1;

	if (movementStarted) {
		if (iTaxiOrder != NULL) {
			// ���� ���������� ������, ���� �������
#if 0
			if (!iTaxiOrder->isStarted()) {
				iTaxiOrder->startOrder();
			}
#endif
			// ��������� �������
			if (iTaxiOrder->isTrainCross()) {
				iTaxiOrder->setTrainCross(false);

				voiceLady->sayPhrase("TRAINCROSSOFF");
				
				ui.trainCrossButton->setEnabled(true);
				ui.trainCrossButton->setChecked(false);
			}
			
			// ��������� ��������� �� ������� �������
			if (iTaxiOrder->isClientStop()) {
				iTaxiOrder->setClientStop(false);
				voiceLady->sayPhrase("CLIENTSTOPOFF");

				ui.clientStopButton->setEnabled(true);
				ui.clientStopButton->setChecked(false);
			}
		}
	}
}


// ����� �������, ���������� �� ���
void IndigoTaxi::trainCrossButtonClicked()
{
	if (!movementStarted) {
		voiceLady->sayPhrase("TRAINCROSS");
		ui.trainCrossButton->setEnabled(false);
		if (iTaxiOrder != NULL)
		{
			iTaxiOrder->setTrainCross(true);
		}
	}
}

void IndigoTaxi::overloadButtonClicked(bool on)
{
	if (on) {
		voiceLady->sayPhrase("OVERLOAD");
	} else {
		voiceLady->sayPhrase("OVERLOADOFF");
	}

	if (iTaxiOrder == NULL)
		return;

	iTaxiOrder->setOverload(on);
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
		
	}

	newDirection = false;
	changeRegion = false;
}

void IndigoTaxi::rebootSystem()
{
	// reboot
	if (confirmDialog->ask("������������� �������?")) {
#ifdef UNDER_CE
		SetSystemPowerState(NULL, POWER_STATE_RESET, 0);
#endif
	}
}