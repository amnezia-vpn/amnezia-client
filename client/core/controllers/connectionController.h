#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QObject>
#include <QFuture>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/servers/serverConfig.h"
#include "protocols/vpnprotocol.h"
#include "vpnconnection.h"

class Settings;
class ServerController;
class VpnConfigurationsController;

using namespace amnezia;

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(const QSharedPointer<VpnConnection> &vpnConnection,
                                  std::shared_ptr<Settings> settings,
                                  QObject *parent = nullptr);

    QFuture<QJsonObject> prepareVpnConfiguration(const QSharedPointer<ServerConfig> &serverConfig,
                                                DockerContainer container,
                                                const QPair<QString, QString> &dns) const;

    QFuture<ErrorCode> openConnection(const int serverIndex,
                                      const QSharedPointer<ServerConfig> &serverConfig,
                                      const DockerContainer container,
                                      const ServerCredentials &credentials,
                                      const QPair<QString, QString> &dns);

    QFuture<ErrorCode> closeConnection();

    ErrorCode getLastConnectionError() const;

signals:
    void configurationPrepared(const QJsonObject &vpnConfiguration);
    void connectionEstablished();
    void connectionTerminated();
    void connectionError(ErrorCode errorCode);
    void connectionProgress(const QString &message);

private:
    bool isServerSupported(DockerContainer container) const;

    QSharedPointer<VpnConnection> m_vpnConnection;
    std::shared_ptr<Settings> m_settings;
};

#endif // CONNECTIONCONTROLLER_H 
