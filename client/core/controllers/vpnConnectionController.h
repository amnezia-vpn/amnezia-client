#ifndef VPNCONNECTIONCONTROLLER_H
#define VPNCONNECTIONCONTROLLER_H

#include <QObject>
#include <QFuture>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/servers/serverConfig.h"
#include "protocols/vpnprotocol.h"

class Settings;
class ServerController;
class VpnConfigurationsController;

using namespace amnezia;

class VpnConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit VpnConnectionController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QFuture<QJsonObject> prepareVpnConfiguration(const QSharedPointer<ServerConfig> &serverConfig,
                                                DockerContainer container,
                                                const QJsonObject &containerConfig);

    QFuture<bool> validateConnection(const QSharedPointer<ServerConfig> &serverConfig,
                                   DockerContainer container);

    QFuture<ErrorCode> establishConnection(const QSharedPointer<ServerConfig> &serverConfig,
                                         DockerContainer container,
                                         const QJsonObject &vpnConfiguration);

    QFuture<ErrorCode> terminateConnection();

    bool isServerSupported(const QSharedPointer<ServerConfig> &serverConfig, DockerContainer container) const;

signals:
    void configurationPrepared(const QJsonObject &vpnConfiguration);
    void connectionEstablished();
    void connectionTerminated();
    void connectionError(ErrorCode errorCode);
    void connectionProgress(const QString &message);

private:
    QJsonObject createVpnConfiguration(const QPair<QString, QString> &dns,
                                      const QJsonObject &serverConfig,
                                      const QJsonObject &containerConfig,
                                      DockerContainer container);

    std::shared_ptr<Settings> m_settings;
};

#endif // VPNCONNECTIONCONTROLLER_H 
