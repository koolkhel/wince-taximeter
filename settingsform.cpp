#include "settingsform.h"

SettingsForm::SettingsForm(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	ui.lineEdit->setProperty("keyboard",true); //enable the keyboard (this is a custom property)
	QValidator *validator = new QIntValidator(0, 500, this);
	ui.lineEdit->setValidator(validator); //add a int validator min value 0 max value 500. This will force the numpad to show, you can also use a QDoubleValidator

	ui.lineEdit->setProperty("keyboard",true); // enable the keyboard. when there is no validator set the keyboard will show
//aTextLineEdit->setProperty("maxLength",25); //this can be used to limit the length of the string

}

SettingsForm::~SettingsForm()
{

}

void SettingsForm::setBackend(Backend *backend)
{
	this->backend = backend;
}

void SettingsForm::newDriverName(QString name)
{
	backend->setDriverName(name.toInt());
}

void SettingsForm::dutyStart()
{
	backend->sendEvent(hello_TaxiEvent_ARRIVED);
}

void SettingsForm::dutyStop()
{
	backend->sendEvent(hello_TaxiEvent_DAY_END);
}

void SettingsForm::dinnerStart()
{
	backend->sendEvent(hello_TaxiEvent_GO_DINNER);
}

void SettingsForm::dinnerStop()
{
	backend->sendEvent(hello_TaxiEvent_BACK_DINNER);
}