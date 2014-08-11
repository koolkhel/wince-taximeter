#ifndef INDIGOTAXI_H
#define INDIGOTAXI_H

#include <QVector>

#include <QtGui/QMainWindow>
#include <QtGui/QProgressDialog>

#include "ui_indigotaxi.h"

#include "qgeopositioninfosource.h"

#include "filedownloader.h"

#include "backend.h"
#include "settingsform.h"

#include "taxiorder.h"

#include "isoundplayer.h"
#include "voicelady.h"

#include "iconfirmationdialog.h"
#include "iinfodialog.h"


class IndigoTaxi : public QMainWindow
{
	Q_OBJECT

public:
	IndigoTaxi(QWidget *parent = 0, Qt::WFlags flags = 0);
	~IndigoTaxi();
	static int const EXIT_CODE_REBOOT;

signals:
	void reboot_application();
	void orderMovementStart(int startStop);
	
	
public slots:
	// 1 -- межгород, 0 -- нет
	void intercity(int intercity);

	void protobuf_message(hello message);
	void connectionStatus(bool status);
// order
	void destroyCurrentOrder();
	void newSatellitesUsed(int);

// updates
	void rebootApp();

	void checkVersion();
	void newVersionDownloaded();

	void updateTime();

// page1
	void moveToClient();
	void inPlace();
	void selectRegionClicked();
	void startClientMove();
	void startClientMoveClicked();
	void settingsButtonClick();
	void clearMessageClick();
	void emergencyButtonClicked();

	// page 2
	void newPaymentCalculated(int);
	void newSpeed(int speed_kmh);
	void newMileage(float mileage);
	void movementStart(int start);
	void paytimeClick();
	void trainCrossButtonClicked();
	void overloadButtonClicked(bool on);
	void newDirectionClicked();
	void clientStopClicked(bool on);

	void newTimeMovement(int _seconds);
	void newTimeStops(int _seconds);
	void newTimeTotal(int _seconds);
	void newTimeClientStops(int _seconds);

	// page 3
	void freeButtonClick();
	void resumeVoyageClick();

	// page 4
	void driverNameChanged(int driverName);
	void driverRegionSelectClicked();
	void privateClientButtonClicked();

	void exitButtonClick();
	void backToStandByClick();
	void dutyButtonClicked(bool pressed);
	void awayButtonClicked();
	void awayEndButtonClicked();
	void clientNotExit();
	void fromcarButtonClicked();
	void fromcarEndButtonClicked();
	void notToMeButtonClicked();
	void emptyTripClicked();
	void repairClicked();
	void repairEndClicked();
	void techhelpClicked();
	void techhelpBackClicked();
	void showOrderHistoryClicked();

	void playClick();

	void dinnerStartClicked();
	void dinnerStopClicked();

	void driverNameEdited(QString newValue);
	void rebootSystem();

	void notPayClicked();

	void taxiRateShowButtonClicked();
	void taxiRateReturnButtonClicked();

	// page 6
	void cancelRegionSelectClicked();
	void showRegionDetailsClicked();
	void changeDriverRegionStopClicked();
	void backToDriverCabinetSettingsClicked();

	// для статуса водителя, типа ремонт-обед и так далее
	QString getSettingsStatus();
	void setSettingsStatus(QString status);
	void enableDutyUI(bool enable);

	void cancelRegionUpdate();
	
	void orderReceiveTimerTimeout();

	void backToRegionsSettings();

	void stackedWidgetCurrentChanged(int);

private:
	TaxiRegionList taxiRegionList;
	TaxiRateAll taxiRates;
	double currentParkingCost;
	int currentParkingId;

	QThread *soundThread;
	ISoundPlayer *iSoundPlayer;
	VoiceLady *voiceLady;

	QTimer *timeTimer;
	Ui::IndigoTaxiClass ui;
	SettingsForm *settingsForm;
	Backend *backend;
	QSettings *settingsIniFile;

	QProgressDialog *progressDialog;

	FileDownloader *downloader;
	QMutex restartMutex;

	void updateTaxiRegionList();
	void updateTaxiRates();


	TaxiInfo taxiInfo;
	void updateTaxiInfo();

	int satellitesUsed;

	ITaxiOrder *iTaxiOrder, *lastTaxiOrder;
	TaxiRatePeriod getCurrentTaxiRatePeriod();
	void handleNewOrder(TaxiOrder order);	
	void handlePersonalAnswer(hello var);
	void handleTextMessage(hello var);
	ITaxiOrder *createTaxiOrder(int order_id, QString address = "");

	bool movementStarted;

	bool newDirection;

	bool changeRegion;

	bool online;

	void enableMainButtons(bool enable);
	void enableWidget(QWidget *widget, bool enable);

	void abortOrder(int order_id);

	void setCurrentScreenFromSettings();

	// обращать на себя внимание 30 секунд, потом всё закрывать
	QTimer *orderReceiveTimer;
	int orderReceiveCounter;

	QList<ITaxiOrder *> ordersHistory;

	IConfirmationDialog *confirmDialog;
	IInfoDialog *infoDialog;

	int asked_region_id;

	void processAskRegionReply(hello var);
	QVector<int> regions_stops_ids;
	QVector<QString> regions_stops_names;

	void addOrderHistory(QString address, QString status, int sum);
	void saveOrderHistory(ITaxiOrder *order, int status = 0);

	bool _taxiRateUpdated;
	bool _taxiRateReceived;
};

#endif // INDIGOTAXI_H
