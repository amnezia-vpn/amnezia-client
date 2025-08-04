#include "socks5ProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

Socks5ProtocolConfig::Socks5ProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(protocols::socks5Proxy::defaultPort);
    serverProtocolConfig.userName = protocolConfigObject.value(config_key::userName).toString(protocols::socks5Proxy::defaultUserName);
    serverProtocolConfig.password = protocolConfigObject.value(config_key::password).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;
    }
}

QJsonObject Socks5ProtocolConfig::toJson() const
{
    QJsonObject json;

    if (!serverProtocolConfig.port.isEmpty()) {
        json[config_key::port] = serverProtocolConfig.port;
    }
    if (!serverProtocolConfig.userName.isEmpty()) {
        json[config_key::userName] = serverProtocolConfig.userName;
    }
    if (!serverProtocolConfig.password.isEmpty()) {
        json[config_key::password] = serverProtocolConfig.password;
    }

    return json;
}

bool Socks5ProtocolConfig::hasEqualServerSettings(const Socks5ProtocolConfig &other) const
{
    return serverProtocolConfig.port == other.serverProtocolConfig.port &&
           serverProtocolConfig.userName == other.serverProtocolConfig.userName &&
           serverProtocolConfig.password == other.serverProtocolConfig.password;
}

void Socks5ProtocolConfig::clearClientSettings()
{
    clientProtocolConfig.isEmpty = true;
}
