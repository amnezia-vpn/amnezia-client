#ifndef FILEHANDLER_H
#define FILEHANDLER_H

class QString;
class QByteArray;
class QUrl;

class FileHandler
{
public:
	explicit FileHandler();
        static QList<QString> schemes();
        bool canHandleUrl(const QUrl &url) const;
        QByteArray dataForUrl(const QUrl &url) const;
        QString mimeTypeForUrl(const QUrl &url) const;
};

#endif
