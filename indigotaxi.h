#ifndef INDIGOTAXI_H
#define INDIGOTAXI_H

#include <QtGui/QMainWindow>

#include "ui_indigotaxi.h"

#include "qgeopositioninfosource.h"

#include "backend.h"
#include "settingsform.h"

class IndigoTaxi : public QMainWindow
{
	Q_OBJECT

public:
	IndigoTaxi(QWidget *parent = 0, Qt::WFlags flags = 0);
	~IndigoTaxi();

	QTextEdit *getLogWidget() { return ui.logText; }
	
public slots:

	void protobuf_message(hello message);
	void settingsButtonClick();
	void showHideLog();
	void connectionStatus(bool status);

	void moveToClient();
	void inPlace();
	void startClientMove();

private:
	Ui::IndigoTaxiClass ui;
	SettingsForm *settingsForm;
	Backend *backend;
};

#endif // INDIGOTAXI_H
