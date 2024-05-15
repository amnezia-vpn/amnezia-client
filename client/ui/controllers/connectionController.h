#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include "protocols/vpnprotocol.h"
#include "ui/models/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"
#include "vpnconnection.h"

class ConnectionController : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isConnectionInProgress READ isConnectionInProgress NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
    Q_PROPERTY(quint64 rxBytes READ rxBytes NOTIFY bytesChanged)
    Q_PROPERTY(quint64 txBytes READ txBytes NOTIFY bytesChanged)

    explicit ConnectionController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                  const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                  const QSharedPointer<VpnConnection> &vpnConnection, const std::shared_ptr<Settings> &settings,
                                  QObject *parent = nullptr);

    ~ConnectionController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;
    quint64 rxBytes() const;
    quint64 txBytes() const;

    Q_INVOKABLE QVector<quint64> getRxView() const;
    Q_INVOKABLE QVector<quint64> getTxView() const;
    Q_INVOKABLE QVector<quint64> getTimes() const;

public slots:
    void toggleConnection();

    void openConnection();
    void closeConnection();

    QString getLastConnectionError();
    void onConnectionStateChanged(Vpn::ConnectionState state);

    void onCurrentContainerUpdated();

    void onTranslationsUpdated();

    ErrorCode updateProtocolConfig(const DockerContainer container, const ServerCredentials &credentials, QJsonObject &containerConfig,
                                   QSharedPointer<ServerController> serverController = nullptr);

signals:
    void connectToVpn(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &vpnConfiguration);
    void disconnectFromVpn();
    void connectionStateChanged();

    void connectionErrorOccurred(const QString &errorMessage);
    void reconnectWithUpdatedContainer(const QString &message);
    void bytesChanged();

    void noInstalledContainers();

    void connectButtonClicked();
    void preparingConfig();

private:
    Vpn::ConnectionState getCurrentConnectionState();
    bool isProtocolConfigExists(const QJsonObject &containerConfig, const DockerContainer container);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;

    QSharedPointer<VpnConnection> m_vpnConnection;

    std::shared_ptr<Settings> m_settings;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
    QVector<quint64> m_rxView{};
    QVector<quint64> m_txView{};
    QVector<quint64> m_times{};

    QTimer m_tick{};

    Vpn::ConnectionState m_state;

    const static quint8 viewSize{60};
};

#endif // CONNECTIONCONTROLLER_H
