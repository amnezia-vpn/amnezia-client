#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"
#include "protocols/vpnprotocol.h"

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                  const QSharedPointer<ContainersModel> &containersModel,
                                  QObject *parent = nullptr);

public slots:
    bool onConnectionButtonClicked();

    bool isConnected();

signals:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void disconnectFromVpn();
    void connectionStateChanged(Vpn::ConnectionState state);

private:
    bool openVpnConnection();
    bool closeVpnConnection();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    bool m_isConnected = false;
};

#endif // CONNECTIONCONTROLLER_H
