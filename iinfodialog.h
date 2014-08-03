#ifndef IINFODIALOG_H
#define IINFODIALOG_H

#include <QDialog>
#include "ui_iinfodialog.h"

class IInfoDialog : public QDialog
{
	Q_OBJECT

public:
	IInfoDialog(QWidget *parent = 0);
	~IInfoDialog();

	void setText(QString text);
	void info(QString text);

private:
	Ui::IInfoDialog ui;
};

#endif // IINFODIALOG_H
