#ifndef CONNECTIONUICONTROLLER_H
#define CONNECTIONUICONTROLLER_H

#include "protocols/vpnprotocol.h"
#include "ui/models/selfhosted/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"
#include "core/controllers/connectionController.h"

class ConnectionUIController : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isConnectionInProgress READ isConnectionInProgress NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)

    explicit ConnectionUIController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                    const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                    const QSharedPointer<ConnectionController> &connectionController, const std::shared_ptr<Settings> &settings,
                                    QObject *parent = nullptr);

    ~ConnectionUIController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;

public slots:
    void toggleConnection();

    void openConnection();
    void closeConnection();

    ErrorCode getLastConnectionError();
    void onConnectionStateChanged(Vpn::ConnectionState state);

    void onCurrentContainerUpdated();

    void onTranslationsUpdated();

signals:
    void connectionStateChanged();

    void connectionErrorOccurred(ErrorCode errorCode);
    void reconnectWithUpdatedContainer(const QString &message);

    void connectButtonClicked();
    void preparingConfig();
    void prepareConfig();

private:
    Vpn::ConnectionState getCurrentConnectionState();

    void continueConnection();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;

    QSharedPointer<ConnectionController> m_connectionController;

    std::shared_ptr<Settings> m_settings;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");

    Vpn::ConnectionState m_state;
};

#endif // CONNECTIONUICONTROLLER_H
