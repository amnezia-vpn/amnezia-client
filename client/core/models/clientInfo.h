#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>

#include "core/models/containers/containers_defs.h"

using namespace amnezia;

class ClientInfo
{
public:
    ClientInfo();
    ClientInfo(const QString &clientId, const QString &clientName);
    ClientInfo(const QJsonObject &jsonObject);

    QJsonObject toJson() const;
    
    static ClientInfo fromJson(const QJsonObject &jsonObject);

    QString clientId;
    QString clientName;
    QDateTime creationDate;
    DockerContainer container;
    
    QString latestHandshake;
    QString dataReceived;
    QString dataSent;
    QString allowedIps;
};

#endif // CLIENTINFO_H
