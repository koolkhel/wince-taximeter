#ifndef INDIGOTAXI_H
#define INDIGOTAXI_H

#include <QtGui/QMainWindow>

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

	void playClick();

	void dinnerStartClicked();
	void dinnerStopClicked();

	void driverNameEdited(QString newValue);
	void rebootSystem();

	void notPayClicked();

	// page 6
	void cancelRegionSelectClicked();

	// для статуса водителя, типа ремонт-обед и так далее
	QString getSettingsStatus();
	void setSettingsStatus(QString status);
	void enableDutyUI(bool enable);
	
	void orderReceiveTimerTimeout();

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
	ITaxiOrder *createTaxiOrder(int order_id, QString address = "");

	bool movementStarted;

	bool newDirection;

	bool changeRegion;

	bool online;

	void enableMainButtons(bool enable);
	void enableInPlaceButton(bool enable);
	void enableWidget(QWidget *widget, bool enable);

	void abortOrder(int order_id);

	void setCurrentScreenFromSettings();

	// обращать на себя внимание 30 секунд, потом всё закрывать
	QTimer *orderReceiveTimer;
	int orderReceiveCounter;

	QList<ITaxiOrder *> ordersHistory;

	IConfirmationDialog *confirmDialog;
	IInfoDialog *infoDialog;
};

#endif // INDIGOTAXI_H
