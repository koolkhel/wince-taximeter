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


class IndigoTaxi : public QMainWindow
{
	Q_OBJECT

public:
	IndigoTaxi(QWidget *parent = 0, Qt::WFlags flags = 0);
	~IndigoTaxi();
	static int const EXIT_CODE_REBOOT;
signals:
		void reboot_application();
	
public slots:

	void protobuf_message(hello message);
	void connectionStatus(bool status);
// order
	void destroyCurrentOrder();

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
	void settingsButtonClick();
	void clearMessageClick();

	// page 2
	void newPaymentCalculated(int);
	void newSpeed(int speed_kmh);
	void newMileage(float mileage);
	void paytimeClick();

	// page 3
	void freeButtonClick();
	void resumeVoyageClick();

	// page 4
	void driverNameChanged(int driverName);

	void exitButtonClick();
	void backToStandByClick();
	void dutyButtonClicked(bool pressed);
	void awayButtonClicked();
	void awayEndButtonClicked();
	void fromcarButtonClicked();
	void fromcarEndButtonClicked();
	void notToMeButtonClicked();

	void dinnerStartClicked();
	void dinnerStopClicked();

	void driverNameEdited(QString newValue);

private:
	TaxiRegionList taxiRegionList;
	TaxiRateAll taxiRates;

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

	ITaxiOrder *iTaxiOrder;
	TaxiRatePeriod getCurrentTaxiRatePeriod();
	void handleNewOrder(TaxiOrder order);	
	ITaxiOrder *createTaxiOrder(int order_id);
};

#endif // INDIGOTAXI_H
