#include "shadowsocksProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

ShadowsocksProtocolConfig::ShadowsocksProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(protocols::shadowsocks::defaultPort);
    serverProtocolConfig.cipher = protocolConfigObject.value(config_key::cipher).toString(protocols::shadowsocks::defaultCipher);

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;

        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
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

void ShadowsocksProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = shadowsocks::ClientProtocolConfig();
} 
