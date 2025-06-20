#include "selfHostedServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

using namespace amnezia;

SelfHostedServerConfig::SelfHostedServerConfig(const QJsonObject &serverConfigObject) : ServerConfig(serverConfigObject)
{   
    name = serverConfigObject.value(config_key::description).toString();
    if (name.isEmpty()) {
        name = serverConfigObject.value(config_key::hostName).toString();
    }

    serverCredentials.hostName = serverConfigObject.value(config_key::hostName).toString();
    serverCredentials.userName = serverConfigObject.value(config_key::userName).toString();
    serverCredentials.secretData = serverConfigObject.value(config_key::password).toString();
    serverCredentials.port = serverConfigObject.value(config_key::port).toInt(22);
}

QJsonObject SelfHostedServerConfig::toJson() const
{
    // Сначала вызываем родительскую функцию для сериализации базовых полей
    QJsonObject json = ServerConfig::toJson();
    
    // Добавляем имя только если оно не пустое
    if (!name.isEmpty()) {
        json[config_key::description] = name; // Используем description как в конструкторе
    }
    
    // Добавляем credentials только если они не пустые
    if (!serverCredentials.hostName.isEmpty()) {
        json[config_key::hostName] = serverCredentials.hostName;
    }
    if (!serverCredentials.userName.isEmpty()) {
        json[config_key::userName] = serverCredentials.userName;
    }
    if (!serverCredentials.secretData.isEmpty()) {
        json[config_key::password] = serverCredentials.secretData;
    }
    if (serverCredentials.port != 22) { // Добавляем порт только если не дефолтный
        json[config_key::port] = serverCredentials.port;
    }
    
    return json;
}
