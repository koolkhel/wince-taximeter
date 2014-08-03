#include "iconfirmationdialog.h"

IConfirmationDialog::IConfirmationDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
}

IConfirmationDialog::~IConfirmationDialog()
{

}

void IConfirmationDialog::setText(QString text)
{
	ui.textBrowser->setText(text);	
}

bool IConfirmationDialog::ask(QString text)
{
	setText(text);
	return exec() == QDialog::Accepted;
}