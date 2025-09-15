#include "socks5ProxyProtocolConfig.h"

#include <QJsonDocument>
#include "protocols/protocols_defs.h"
#include "utilities.h"

using namespace amnezia;

Socks5ProxyProtocolConfig::Socks5ProxyProtocolConfig(const QString &protocolName, int port) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = QString::number(port);
    serverProtocolConfig.userName = protocols::socks5Proxy::defaultUserName;
    serverProtocolConfig.password = Utils::getRandomString(16);
}

Socks5ProxyProtocolConfig::Socks5ProxyProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(protocols::socks5Proxy::defaultPort);
    serverProtocolConfig.userName = protocolConfigObject.value(config_key::userName).toString();
    serverProtocolConfig.password = protocolConfigObject.value(config_key::password).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;
        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

Socks5ProxyProtocolConfig::Socks5ProxyProtocolConfig(const Socks5ProxyProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
}

QJsonObject Socks5ProxyProtocolConfig::toJson() const
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

ScriptVars Socks5ProxyProtocolConfig::getScriptVars() const
{
    ScriptVars vars;

    if (!serverProtocolConfig.port.isEmpty()) {
        vars.append({ "$SOCKS5_PROXY_PORT", serverProtocolConfig.port });
    }

    QString socks5user = (!serverProtocolConfig.userName.isEmpty() && !serverProtocolConfig.password.isEmpty()) 
                        ? QString("users %1:CL:%2").arg(serverProtocolConfig.userName, serverProtocolConfig.password) : "";
    vars.append({ "$SOCKS5_USER", socks5user });
    vars.append({ "$SOCKS5_AUTH_TYPE", socks5user.isEmpty() ? QString("none") : QString("strong") });

    return vars;
}

bool Socks5ProxyProtocolConfig::hasEqualServerSettings(const Socks5ProxyProtocolConfig &other) const
{
    if (serverProtocolConfig.port != other.serverProtocolConfig.port
        || serverProtocolConfig.userName != other.serverProtocolConfig.userName
        || serverProtocolConfig.password != other.serverProtocolConfig.password) {
        return false;
    }
    return true;
}

void Socks5ProxyProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = socks5Proxy::ClientProtocolConfig();
}
