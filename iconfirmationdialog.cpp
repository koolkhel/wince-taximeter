#include "iconfirmationdialog.h"

#include <QSound>

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
	ui.pushButton->setText("�����");
	ui.pushButton_2->setText("��");
	ui.textBrowser->setFontPointSize(16);
	setText(text);
	return exec() == QDialog::Accepted;
}

bool IConfirmationDialog::askYesNo(QString text)
{
	ui.pushButton->setText("���");
	ui.pushButton_2->setText("��");
	ui.textBrowser->setFontPointSize(14);
	setText(text);
	return exec() == QDialog::Accepted;
}

void IConfirmationDialog::clickSound()
{
	QSound::play("click.wav");
}