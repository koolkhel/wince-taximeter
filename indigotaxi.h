#ifndef INDIGOTAXI_H
#define INDIGOTAXI_H

#include <QtGui/QMainWindow>

#include "ui_indigotaxi.h"

#include "qgeopositioninfosource.h"

#include "filedownloader.h"

#include "backend.h"
#include "settingsform.h"

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
	void showHideLog();
	void connectionStatus(bool status);
	
	void rebootApp();

	void checkVersion();
	void newVersionDownloaded();

// page1
	void moveToClient();
	void inPlace();
	void startClientMove();
	void settingsButtonClick();
	void clearMessageClick();

	// page 2
	void paytimeClick();

	// page 3
	void freeButtonClick();
	void resumeVoyageClick();

	// page 4
	void exitButtonClick();
	void backToStandByClick();

private:
	Ui::IndigoTaxiClass ui;
	SettingsForm *settingsForm;
	Backend *backend;

	FileDownloader *downloader;
	QMutex restartMutex;
};

#endif // INDIGOTAXI_H
