#include "iinfodialog.h"

IInfoDialog::IInfoDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

IInfoDialog::~IInfoDialog()
{

}

void IInfoDialog::setText(QString text)
{
	ui.textBrowser->setText(text);
}