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
    QJsonObject json = ServerConfig::toJson();
    
    if (!name.isEmpty()) {
        json[config_key::name] = name;
    }
    if (!description.isEmpty()) {
        json[config_key::description] = description;
    }
    
    return json;
}
