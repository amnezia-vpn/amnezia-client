#include "sftpProtocolConfig.h"

#include <QJsonDocument>
#include "protocols/protocols_defs.h"
#include "utilities.h"

using namespace amnezia;

SftpProtocolConfig::SftpProtocolConfig(const QString &protocolName, int port) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = QString::number(port);
    serverProtocolConfig.userName = protocols::sftp::defaultUserName;
    serverProtocolConfig.password = Utils::getRandomString(16);
}

SftpProtocolConfig::SftpProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName) : ProtocolConfig(protocolName)
{
    serverProtocolConfig.port = protocolConfigObject.value(config_key::port).toString(QString::number(ProtocolProps::defaultPort(Proto::Sftp)));
    serverProtocolConfig.userName = protocolConfigObject.value(config_key::userName).toString();
    serverProtocolConfig.password = protocolConfigObject.value(config_key::password).toString();

    auto clientProtocolString = protocolConfigObject.value(config_key::last_config).toString();
    if (!clientProtocolString.isEmpty()) {
        clientProtocolConfig.isEmpty = false;
        QJsonObject clientProtocolConfigObject = QJsonDocument::fromJson(clientProtocolString.toUtf8()).object();
    }
}

SftpProtocolConfig::SftpProtocolConfig(const SftpProtocolConfig &other) : ProtocolConfig(other.protocolName)
{
    serverProtocolConfig = other.serverProtocolConfig;
    clientProtocolConfig = other.clientProtocolConfig;
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

ScriptVars SftpProtocolConfig::getScriptVars() const
{
    ScriptVars vars;

    if (!serverProtocolConfig.port.isEmpty()) {
        vars.append({ "$SFTP_PORT", serverProtocolConfig.port });
    }
    if (!serverProtocolConfig.userName.isEmpty()) {
        vars.append({ "$SFTP_USER", serverProtocolConfig.userName });
    }
    if (!serverProtocolConfig.password.isEmpty()) {
        vars.append({ "$SFTP_PASSWORD", serverProtocolConfig.password });
    }

    return vars;
}

bool SftpProtocolConfig::hasEqualServerSettings(const SftpProtocolConfig &other) const
{
    if (serverProtocolConfig.port != other.serverProtocolConfig.port
        || serverProtocolConfig.userName != other.serverProtocolConfig.userName
        || serverProtocolConfig.password != other.serverProtocolConfig.password) {
        return false;
    }
    return true;
}

void SftpProtocolConfig::clearClientSettings()
{
    clientProtocolConfig = sftp::ClientProtocolConfig();
}
