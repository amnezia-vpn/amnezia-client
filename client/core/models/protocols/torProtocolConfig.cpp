#include "torProtocolConfig.h"

#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

TorProtocolConfig::TorProtocolConfig(const QString &protocolName) : ProtocolConfig(protocolName)
{
}

TorProtocolConfig::TorProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.site = protocolConfigObject.value(config_key::site).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

TorProtocolConfig::TorProtocolConfig(const TorProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
}

QJsonObject TorProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.site.isEmpty()) {
        json[config_key::site] = serverProtocolConfig.site;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;
        json[config_key::last_config] = QString(QJsonDocument(clientConfigJson).toJson());
    }

    return json;
}

bool TorProtocolConfig::hasEqualServerSettings(const TorProtocolConfig &other) const
{
    return serverProtocolConfig.site == other.serverProtocolConfig.site;
}

void TorProtocolConfig::clearClientSettings()
{
    clientProtocolConfig.isEmpty = true;
}

ScriptVars TorProtocolConfig::getScriptVars() const
{
    ScriptVars vars;
    
    if (!serverProtocolConfig.site.isEmpty()) {
        vars.append(qMakePair(QString("$TOR_SITE"), serverProtocolConfig.site));
    }
    
    return vars;
}
