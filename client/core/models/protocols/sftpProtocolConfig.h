#ifndef SFTPPROTOCOLCONFIG_H
#define SFTPPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

#include "protocolConfig.h"
#include "protocols/protocols_defs.h"

namespace sftp
{
    struct ServerProtocolConfig
    {
        QString port;
        QString userName;
        QString password;
    };

    struct ClientProtocolConfig
    {
        bool isEmpty = true;
    };
}

class SftpProtocolConfig : public ProtocolConfig
{
public:
    SftpProtocolConfig(const QString &protocolName, int port);
    SftpProtocolConfig(const QJsonObject &protocolConfigObject, const QString &protocolName);
    SftpProtocolConfig(const SftpProtocolConfig &other);

    QJsonObject toJson() const override;
    ScriptVars getScriptVars() const override;

    bool hasEqualServerSettings(const SftpProtocolConfig &other) const;
    void clearClientSettings();

    sftp::ServerProtocolConfig serverProtocolConfig;
    sftp::ClientProtocolConfig clientProtocolConfig;
};

#endif // SFTPPROTOCOLCONFIG_H
