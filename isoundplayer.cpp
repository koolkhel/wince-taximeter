#include <QtCore>
#include <QDebug>

#include "isoundplayer.h"
#include "windows.h"


ISoundPlayer::ISoundPlayer(QObject *parent)
	: QObject(parent)
{

}

ISoundPlayer::~ISoundPlayer()
{

}

void ISoundPlayer::playResourceSound(QString url)
{
	PlaySoundW((LPCWSTR)QResource(url).data(), 0, SND_MEMORY | SND_SYNC);
}