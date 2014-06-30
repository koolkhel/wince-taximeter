#include "voicelady.h"
#include <QDebug>
#include <QSound>
#include <QtCore>

#include "windows.h"

VoiceLady::VoiceLady(QObject *parent)
	: QObject(parent)
{

}

void VoiceLady::speakMoney(int amount)
{
	/// 123 == 100 + 20 + 3
	int tmp = amount;
	// единицы, десятки, сотни
	int amounts[6] = {0};
	int max_order = 0;

	while (tmp > 0) {
		int a = tmp % 10;
		amounts[max_order] = a;
		tmp /= 10;
		max_order++;
	}

	max_order -= 1;

	if (amounts[3] != 0) {
		switch (amounts[3]) {
			case 1:
				emit playSound(QString(":/Sound/Resources/voice/1na.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000a.wav"));
				break;
			case 2:
				emit playSound(QString(":/Sound/Resources/voice/2e.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000i.wav"));
				break;
			case 3:
				emit playSound(QString(":/Sound/Resources/voice/3.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000i.wav"));
				break;
			case 4:
				emit playSound(QString(":/Sound/Resources/voice/4.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000i.wav"));
				break;
			case 5:
				emit playSound(QString(":/Sound/Resources/voice/5.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000.wav"));
				break;
			case 6:
				emit playSound(QString(":/Sound/Resources/voice/6.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000.wav"));
				break;
			case 7:
				emit playSound(QString(":/Sound/Resources/voice/7.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000.wav"));
				break;
			case 8:
				emit playSound(QString(":/Sound/Resources/voice/8.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000.wav"));
				break;
			case 9:
				emit playSound(QString(":/Sound/Resources/voice/9.wav"));
				emit playSound(QString(":/Sound/Resources/voice/1000.wav"));
				break;
		}
	}
	if (max_order >= 2) {
		emit playSound(QString(":/Sound/Resources/voice/") 
			+ QString::number(amounts[2] * 100) 
			+ QString(".wav"));
	}

	if (amounts[1] == 1) { // десятки
		emit playSound(QString(":/Sound/Resources/voice/") 
			+ QString::number(10 * amounts[1] + amounts[0]) 
			+ QString(".wav"));
	} else if (amounts[1] == 0) { // нули
		if (amounts[0] != 0) {
			emit playSound(QString(":/Sound/Resources/voice/") 
				+ QString::number(amounts[0]) 
				+ QString(".wav"));
		}
	} else {
		emit playSound(QString(":/Sound/Resources/voice/") 
			+ QString::number(amounts[1] * 10) 
			+ QString(".wav"));
		if (amounts[0] != 0) {
			emit playSound(QString(":/Sound/Resources/voice/") 
				+ QString::number(amounts[0]) 
				+ QString(".wav"));
		}
	}

	if (amounts[1] == 1) {
		emit playSound(QString(":/Sound/Resources/voice/roubles.wav"));
	} else {
		switch (amounts[0]) {
	case 0:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		emit playSound(QString(":/Sound/Resources/voice/roubles.wav"));
		break;
	case 2:
	case 3:
	case 4:
		emit playSound(QString(":/Sound/Resources/voice/roubla.wav"));
		break;
	case 1:
		emit playSound(QString(":/Sound/Resources/voice/roubl.wav"));
		break;
		}
	}

	//PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/stop.wav").data(), 0, SND_MEMORY | SND_SYNC);
	
#if 0
		emit playSound(":/Sound/Resources/voice/100.wav");
	emit playSound(":/Sound/Resources/voice/20.wav");
	emit playSound(":/Sound/Resources/voice/3.wav");
	emit playSound(":/Sound/Resources/voice/roubla.wav");

	PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/100.wav").data(), 0, SND_MEMORY | SND_SYNC);
	Sleep(50);
	PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/20.wav").data(), 0, SND_MEMORY | SND_SYNC);
	Sleep(50);	
	PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/3.wav").data(), 0, SND_MEMORY | SND_SYNC);
	Sleep(50);	
	PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/roubla.wav").data(), 0, SND_MEMORY | SND_SYNC);
#endif
	
	
	//int flags = SND_FILENAME|SND_ASYNC; 
//	int flags = SND_FILENAME; 
	//qDebug() << PlaySoundW(L"alarm.wav", 0, flags);
	
	//PlaySoundW((LPCWSTR)QResource(":/Sound/Resources/voice/stop.wav").data(), 0, SND_MEMORY | SND_SYNC);
	//qDebug() << "max order:" << max_order << "amounts:" << amounts[0] << amounts[1] << amounts[2] << amounts[3] << amounts[4] << amounts[5];
	//QSound::play(wavPath + QDir::separator() + "100.wav");
	//QSound::play(wavPath + QDir::separator() + "20.wav");
	//QSound::play(wavPath + QDir::separator() + "3.wav");
	//QSound::play(wavPath + QDir::separator() + "roubla.wav");

	qDebug() << "123123123";
}

VoiceLady::~VoiceLady()
{

}
