#include "apiV1ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

using namespace amnezia;

ApiV1ServerConfig::ApiV1ServerConfig(const QJsonObject &serverConfigObject) : ServerConfig(serverConfigObject)
{
    name = serverConfigObject.value(config_key::name).toString();
    description = serverConfigObject.value(config_key::description).toString();
}

QJsonObject ApiV1ServerConfig::toJson() const
{
    // Сначала вызываем родительскую функцию для сериализации базовых полей
    QJsonObject json = ServerConfig::toJson();
    
    // Добавляем свои поля только если они не пустые
    if (!name.isEmpty()) {
        json[config_key::name] = name;
    }
    if (!description.isEmpty()) {
        json[config_key::description] = description;
    }
    
    return json;
}
