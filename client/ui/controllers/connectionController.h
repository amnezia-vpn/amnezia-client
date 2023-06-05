#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"
#include "protocols/vpnprotocol.h"
#include "vpnconnection.h"

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool isConnected READ isConnected WRITE setIsConnected NOTIFY isConnectedChanged)

    explicit ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                  const QSharedPointer<ContainersModel> &containersModel,
                                  const QSharedPointer<VpnConnection> &vpnConnection,
                                  QObject *parent = nullptr);

    bool isConnected();
    void setIsConnected(bool isConnected);

public slots:
    void openConnection();
    void closeConnection();

    QString getLastConnectionError();

signals:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void disconnectFromVpn();
    void connectionStateChanged(Vpn::ConnectionState state);
    void isConnectedChanged();

    void connectionErrorOccurred(QString errorMessage);

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    QSharedPointer<VpnConnection> m_vpnConnection;

    bool m_isConnected = false;
};

#endif // CONNECTIONCONTROLLER_H
