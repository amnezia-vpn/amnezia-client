#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                  const QSharedPointer<ContainersModel> &containersModel,
                                  QObject *parent = nullptr);

public slots:
    bool onConnectionButtonClicked();

signals:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void disconnectFromVpn();

private:
    bool openVpnConnection();
    bool closeVpnConnection();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    bool isConnected = false;
};

#endif // CONNECTIONCONTROLLER_H
