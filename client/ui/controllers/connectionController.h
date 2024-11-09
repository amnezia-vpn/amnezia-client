#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include "protocols/vpnprotocol.h"
#include "ui/controllers/authController.h"
#include "ui/controllers/importController.h"
#include "ui/models/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/regionsModel.h"
#include "ui/models/servers_model.h"
#include "vpnconnection.h"

class ConnectionController : public QObject {
    Q_OBJECT

public:
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isConnectionInProgress READ isConnectionInProgress NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)

    explicit ConnectionController(const QSharedPointer<ImportController> importController,
                                  const QSharedPointer<AuthController> authController,
                                  const QSharedPointer<RegionsModel> regionsModel,
                                  const QSharedPointer<ServersModel> &serversModel,
                                  const QSharedPointer<ContainersModel> &containersModel,
                                  const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                  const QSharedPointer<VpnConnection> &vpnConnection,
                                  const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

    ~ConnectionController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;

public slots:
    void toggleConnection();

    void openConnection();
    void closeConnection();

    ErrorCode getLastConnectionError();
    void onConnectionStateChanged(Vpn::ConnectionState state, bool getLastError = true);

    void onCurrentContainerUpdated();

    void onTranslationsUpdated();

    ErrorCode updateProtocolConfig(const DockerContainer container, const ServerCredentials &credentials,
                                   QJsonObject &containerConfig,
                                   QSharedPointer<ServerController> serverController = nullptr);

signals:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container,
                      const QJsonObject &vpnConfiguration);
    void disconnectFromVpn();
    void connectionStateChanged();

    void connectionErrorOccurred(const QString &errorMessage);
    void connectionErrorOccurred(ErrorCode errorCode);
    void reconnectWithUpdatedContainer(const QString &message);

    void noInstalledContainers();

    void connectButtonClicked();
    void preparingConfig();

    void updateApiConfigFromGateway();
    void updateApiConfigFromTelegram();
    void configFromApiUpdated();

    void splitTunnelingFailed();

private:
    Vpn::ConnectionState getCurrentConnectionState();
    bool isProtocolConfigExists(const QJsonObject &containerConfig, const DockerContainer container);

    void continueConnection();

    QSharedPointer<ImportController> m_importController;
    QSharedPointer<AuthController> m_authController;
    QSharedPointer<RegionsModel> m_regionsModel;

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;

    QSharedPointer<VpnConnection> m_vpnConnection;

    std::shared_ptr<Settings> m_settings;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");

    Vpn::ConnectionState m_state;
};

#endif // CONNECTIONCONTROLLER_H
