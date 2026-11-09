#ifndef LOGSCONTROLLER_H
#define LOGSCONTROLLER_H

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class LogsController
{
public:
    enum class Stream { App, Tunnel };

    static QStringList files(Stream stream);
    static qint64 size(Stream stream);
    static qint64 fileSize(const QString &path);
    static QString readTail(Stream stream, qint64 *newestFileEnd = nullptr, qint64 maxBytes = 512 * 1024);
    static QByteArray readAll(Stream stream);
    static void clear(Stream stream);
    static QByteArray zipAll();
    static QByteArray zipFiles(const QList<QPair<QString, QByteArray>> &entries);

    static QByteArray tailOfFiles(const QStringList &paths, qint64 maxBytes, qint64 *newestFileEnd = nullptr);

private:
    static QByteArray read(Stream stream, qint64 maxBytes, qint64 *newestFileEnd = nullptr);
};

#endif // LOGSCONTROLLER_H
