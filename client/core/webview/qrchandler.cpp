#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include "qrchandler.h"
#include "mimecache.h"

QString QrcHandler::scheme()
{
	return QString("qrc");
}

QByteArray QrcHandler::dataForUrl(const QUrl &url) const
{
	QString requestUrl(QString(":/%1").arg(url.path()));
	QFile resource(requestUrl);
	QByteArray buffer;
	if (resource.exists() && resource.open(QIODevice::ReadOnly)) {

		buffer = resource.readAll();
	    resource.close();
	}
	return buffer;
}

bool QrcHandler::canHandleUrl(const QUrl &url) const
{
	if (scheme() == url.scheme())
		return true;
	return false;
}

QString QrcHandler::mimeTypeForUrl(const QUrl &url) const
{
	return mimeTypeForExtension(url.path().section('.', -1));
}
