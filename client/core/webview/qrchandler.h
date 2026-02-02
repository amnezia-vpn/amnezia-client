#ifndef QRCHANDLER_H
#define QRCHANDLER_H

class QString;
class QByteArray;
class QUrl;

class QrcHandler
{
public:
	explicit QrcHandler();
	static QString scheme();
        bool canHandleUrl(const QUrl &url) const;
	QByteArray dataForUrl(const QUrl &url) const;
	QString mimeTypeForUrl(const QUrl &url) const;
};

#endif
