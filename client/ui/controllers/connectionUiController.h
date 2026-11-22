#ifndef CONNECTIONUICONTROLLER_H
#define CONNECTIONUICONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
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
    Q_PROPERTY(int connectionDurationSecs READ connectionDurationSecs NOTIFY connectionStatsChanged)
    Q_PROPERTY(int connectionSetupMs READ connectionSetupMs NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 receivedBytes READ receivedBytes NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 sentBytes READ sentBytes NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 downloadSpeed READ downloadSpeed NOTIFY connectionStatsChanged)
    Q_PROPERTY(qint64 uploadSpeed READ uploadSpeed NOTIFY connectionStatsChanged)

    explicit ConnectionUiController(ConnectionController* connectionController,
                                    ServersController* serversController,
                                    QObject *parent = nullptr);

    ~ConnectionUiController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;
    int connectionDurationSecs() const;
    int connectionSetupMs() const;
    qint64 receivedBytes() const;
    qint64 sentBytes() const;
    qint64 downloadSpeed() const;
    qint64 uploadSpeed() const;

public slots:
    void toggleConnection();

    void openConnection();
    void closeConnection();

    bool isRevokeBlockedDuringActiveConnection(const QString &serverId, int containerIndex, const QString &clientId) const;

    ErrorCode getLastConnectionError();
    void onConnectionStateChanged(Vpn::ConnectionState state);

    void onTranslationsUpdated();

signals:
    void connectionStateChanged();
    void connectionStatsChanged();

    void connectionErrorOccurred(ErrorCode errorCode);

    void connectButtonClicked();
    void preparingConfig();
    void prepareConfig();
    void unsupportedConnectDrawerRequested();
    void noInstalledContainers();

private:
    Vpn::ConnectionState getCurrentConnectionState();
    void notifyConnectionBlocked(ErrorCode errorCode);
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);

    ConnectionController* m_connectionController;
    ServersController* m_serversController;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");

    Vpn::ConnectionState m_state;

    QTimer m_statsTimer;
    QElapsedTimer m_connectionAttemptTimer;
    QElapsedTimer m_connectedTimer;
    QElapsedTimer m_speedTimer;
    qint64 m_receivedBytes = 0;
    qint64 m_sentBytes = 0;
    qint64 m_downloadSpeed = 0;
    qint64 m_uploadSpeed = 0;
    int m_connectionSetupMs = 0;
    bool m_attemptInProgress = false;
    bool m_setupCaptured = false;
};

#endif
