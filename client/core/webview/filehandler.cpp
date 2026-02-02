#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include "filehandler.h"
#include "mimecache.h"

QList<QString> FileHandler::schemes()
{
    static QList<QString> list = QList<QString>() << "file" << "local";
    return list;
}

QByteArray FileHandler::dataForUrl(const QUrl &url) const
{
    QUrl fileUrl = url;
    if (fileUrl.scheme() != "file") {
        fileUrl.setScheme("file");
    }
    QString requestUrl(fileUrl.toLocalFile());
	QFile resource(requestUrl);
	QByteArray buffer;
	if (resource.exists() && resource.open(QIODevice::ReadOnly)) {

		buffer = resource.readAll();
	    resource.close();
	}
	return buffer;
}

bool FileHandler::canHandleUrl(const QUrl &url) const
{
    if (schemes().contains(url.scheme().toLower()))
		return true;
	return false;
}


QString FileHandler::mimeTypeForUrl(const QUrl &url) const
{
	return mimeTypeForExtension(url.path().section('.', -1));
}
