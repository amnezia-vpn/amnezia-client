#ifndef PAYLOADSENDER_H
#define PAYLOADSENDER_H

#include <QByteArray>
#include <QJsonArray>
#include <QString>

class PayloadSender
{
public:
    static void sendAll(const QJsonArray &sendPayload);
    static QByteArray buildPayload(const QString &tagString);

private:
    static void sendEntry(const QJsonObject &entry, int index);
};

#endif // PAYLOADSENDER_H
