#include "indigotaxi.h"
#include <QtGui>
#include <QtCore>

#include "backend.h"

IndigoTaxi::IndigoTaxi(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
#ifdef UNDER_CE
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#endif

	settingsForm = new SettingsForm(this);
	
	settingsForm->hide();

	ui.logText->hide();
	backend = new Backend(this);
	connect(backend, SIGNAL(protobuf_message(hello)), SLOT(protobuf_message(hello)));
	connect(backend, SIGNAL(connectedToServer(bool)), SLOT(connectionStatus(bool)));
	settingsForm->setBackend(backend);
}

IndigoTaxi::~IndigoTaxi()
{

}

void IndigoTaxi::settingsButtonClick()
{
	settingsForm->show();
}

void IndigoTaxi::showHideLog() 
{
	if (ui.logText->isVisible()) {
		ui.logText->hide();
	} else {
		ui.logText->show();
	}
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
