#ifndef VOICELADY_H
#define VOICELADY_H

#include <QObject>

class VoiceLady : public QObject
{
	Q_OBJECT

public:
	VoiceLady(QObject *parent = 0);
	~VoiceLady();
public slots:
	void speakMoney(int amount);
signals:
	void playSound(QString);
private:
	
};

#endif // VOICELADY_H
