#include "clientInfo.h"

#include <QJsonObject>

namespace
{
    namespace configKey
    {
        constexpr char clientId[] = "clientId";
        constexpr char clientName[] = "clientName";
        constexpr char container[] = "container";
        constexpr char userData[] = "userData";
        constexpr char creationDate[] = "creationDate";
        constexpr char latestHandshake[] = "latestHandshake";
        constexpr char dataReceived[] = "dataReceived";
        constexpr char dataSent[] = "dataSent";
        constexpr char allowedIps[] = "allowedIps";
    }
}

ClientInfo::ClientInfo()
    : container(DockerContainer::None)
{
}

ClientInfo::ClientInfo(const QString &clientId, const QString &clientName)
    : clientId(clientId), clientName(clientName), creationDate(QDateTime::currentDateTime()), container(DockerContainer::None)
{
}

ClientInfo::ClientInfo(const QJsonObject &jsonObject)
{
    clientId = jsonObject.value(configKey::clientId).toString();
    container = ContainerProps::containerFromString(jsonObject.value(configKey::container).toString());
    
    QJsonObject userData = jsonObject.value(configKey::userData).toObject();
    clientName = userData.value(configKey::clientName).toString();
    creationDate = QDateTime::fromString(userData.value(configKey::creationDate).toString());
    
    latestHandshake = jsonObject.value(configKey::latestHandshake).toString();
    dataReceived = jsonObject.value(configKey::dataReceived).toString();
    dataSent = jsonObject.value(configKey::dataSent).toString();
    allowedIps = jsonObject.value(configKey::allowedIps).toString();
}

QJsonObject ClientInfo::toJson() const
{
    QJsonObject jsonObject;
    jsonObject[configKey::clientId] = clientId;
    jsonObject[configKey::container] = ContainerProps::containerToString(container);
    
    QJsonObject userData;
    userData[configKey::clientName] = clientName;
    userData[configKey::creationDate] = creationDate.toString();
    jsonObject[configKey::userData] = userData;
    
    if (!latestHandshake.isEmpty()) {
        jsonObject[configKey::latestHandshake] = latestHandshake;
    }
    if (!dataReceived.isEmpty()) {
        jsonObject[configKey::dataReceived] = dataReceived;
    }
    if (!dataSent.isEmpty()) {
        jsonObject[configKey::dataSent] = dataSent;
    }
    if (!allowedIps.isEmpty()) {
        jsonObject[configKey::allowedIps] = allowedIps;
    }
    
    return jsonObject;
}

ClientInfo ClientInfo::fromJson(const QJsonObject &jsonObject)
{
    return ClientInfo(jsonObject);
}
