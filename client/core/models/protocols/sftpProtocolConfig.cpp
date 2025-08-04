#include "sftpProtocolConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include "protocols/protocols_defs.h"

using namespace amnezia;

SftpProtocolConfig::SftpProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp)));
    serverProtocolConfig.userName = protocolConfigObject.value(config_key::userName).toString(protocols::sftp::defaultUserName);
    serverProtocolConfig.password = protocolConfigObject.value(config_key::password).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;
    }
}

QJsonObject SftpProtocolConfig::toJson() const
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

bool SftpProtocolConfig::hasEqualServerSettings(const SftpProtocolConfig &other) const
{
    return serverProtocolConfig.port == other.serverProtocolConfig.port &&
           serverProtocolConfig.userName == other.serverProtocolConfig.userName &&
           serverProtocolConfig.password == other.serverProtocolConfig.password;
}

void SftpProtocolConfig::clearClientSettings()
{
    clientProtocolConfig.isEmpty = true;
}
