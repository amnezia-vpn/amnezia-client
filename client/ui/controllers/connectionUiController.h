#ifndef CONNECTIONUICONTROLLER_H
#define CONNECTIONUICONTROLLER_H

#include <QObject>
#include <QTimer>

#include "core/controllers/connectionController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/protocols/vpnProtocol.h"
#include "core/controllers/serversController.h"

class ConnectionUiController : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
    Q_PROPERTY(bool isConnectionInProgress READ isConnectionInProgress NOTIFY connectionStateChanged)
    Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)

    explicit ConnectionUiController(ConnectionController* connectionController,
                                    ServersController* serversController,
                                    QObject *parent = nullptr);

    ~ConnectionUiController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;

public slots:
    void toggleConnection();

    void openConnection();
    void closeConnection();

    ErrorCode getLastConnectionError();
    void onConnectionStateChanged(Vpn::ConnectionState state);

    void onTranslationsUpdated();

public slots:
    void checkAndStartAwgStateTimer();
    void onUpdateServiceFromGatewayCompleted(bool success, const QString &serverId);
    void onCurrentContainerUpdated();

private slots:
    void onAwgStateTimeout();

signals:
    void connectionStateChanged();

    void connectionErrorOccurred(ErrorCode errorCode);

    void connectButtonClicked();
    void preparingConfig();
    void prepareConfig();

    // serverId + protocol — both carried so the receiver doesn't need to re-read default server
    void requestSetCurrentProtocol(const QString &serverId, const QString &protocol);
    void requestUpdateServiceFromGateway(const QString &serverId, const QString &newCountryCode,
                                         const QString &newCountryName, bool reloadServiceConfig);
    void requestSetProcessedServer(const QString &serverId);

private:
    Vpn::ConnectionState getCurrentConnectionState();

    static constexpr int kAwgSwitchTimeoutMs = 10000;

    QTimer m_awgStateTimer;
    ConnectionController* m_connectionController;
    ServersController* m_serversController;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");

    Vpn::ConnectionState m_state;

    QString m_pendingApiServerId;
    bool m_apiSwitched = false;
    bool m_waitingForApiUpdate = false;
};

#endif
