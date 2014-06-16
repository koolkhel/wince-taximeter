#include <QDebug>

#include "filedownloader.h"
 
FileDownloader::FileDownloader(QUrl imageUrl, QObject *parent) :
    QObject(parent)
{
    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
                SLOT(fileDownloaded(QNetworkReply*)));
 
    QNetworkRequest request(imageUrl);
	qDebug() << "requesting url" << imageUrl;
    m_WebCtrl.get(request);
}
 
FileDownloader::~FileDownloader()
{
 
}
 
void FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
	if (pReply->error() == QNetworkReply::NoError) {
		qDebug() << "download successful";
		m_DownloadedData = pReply->readAll();
		emit downloaded();
	} else {
		qDebug() << "download error";
	}
    
	//emit a signal
    pReply->deleteLater();
    
}
 
QByteArray FileDownloader::downloadedData() const
{
    return m_DownloadedData;
}