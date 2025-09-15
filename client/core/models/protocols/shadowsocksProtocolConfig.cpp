#include "shadowsocksProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

ShadowsocksProtocolConfig::ShadowsocksProtocolConfig(const QString &protocolName, int port) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = QString::number(port);
    serverProtocolConfig.cipher = protocols::shadowsocks::defaultCipher;
}

ShadowsocksProtocolConfig::ShadowsocksProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString();
    serverProtocolConfig.cipher = protocolConfigObject.value(config_key::cipher).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

ShadowsocksProtocolConfig::ShadowsocksProtocolConfig(const ShadowsocksProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
}

QJsonObject ShadowsocksProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.port.isEmpty()) {
        json[config_key::port] = serverProtocolConfig.port;
    }
    if (!serverProtocolConfig.cipher.isEmpty()) {
        json[config_key::cipher] = serverProtocolConfig.cipher;
    }

    if (!clientProtocolConfig.isEmpty) {
        QJsonObject clientConfigJson;
        json[config_key::last_config] = QString(QJsonDocument(clientConfigJson).toJson());
    }

    return json;
}

bool ShadowsocksProtocolConfig::hasEqualServerSettings(const ShadowsocksProtocolConfig &other) const
{
    if (serverProtocolConfig.port != other.serverProtocolConfig.port || 
        serverProtocolConfig.cipher != other.serverProtocolConfig.cipher) {
        return false;
    }
    return true;
}

ScriptVars ShadowsocksProtocolConfig::getScriptVars() const
{
    ScriptVars vars;

    if (!serverProtocolConfig.port.isEmpty()) {
        vars.append({ "$SHADOWSOCKS_SERVER_PORT", serverProtocolConfig.port });
    }
    if (!serverProtocolConfig.cipher.isEmpty()) {
        vars.append({ "$SHADOWSOCKS_CIPHER", serverProtocolConfig.cipher });
    }

    return vars;
}

void ShadowsocksProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = shadowsocks::ClientProtocolConfig();
} 
