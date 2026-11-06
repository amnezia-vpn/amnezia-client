#ifndef CONNECTIONUICONTROLLER_H
#define CONNECTIONUICONTROLLER_H

#include <QObject>

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
    Q_PROPERTY(bool hasTrafficSpeed READ hasTrafficSpeed NOTIFY trafficSpeedChanged)
    Q_PROPERTY(QString downloadSpeed READ downloadSpeed NOTIFY trafficSpeedChanged)
    Q_PROPERTY(QString uploadSpeed READ uploadSpeed NOTIFY trafficSpeedChanged)

    explicit ConnectionUiController(ConnectionController* connectionController,
                                    ServersController* serversController,
                                    QObject *parent = nullptr);

    ~ConnectionUiController() = default;

    bool isConnected() const;
    bool isConnectionInProgress() const;
    QString connectionStateText() const;
    bool hasTrafficSpeed() const;
    QString downloadSpeed() const;
    QString uploadSpeed() const;

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
    void trafficSpeedChanged();

    void connectionErrorOccurred(ErrorCode errorCode);

    void connectButtonClicked();
    void preparingConfig();
    void prepareConfig();
    void unsupportedConnectDrawerRequested();
    void noInstalledContainers();

private:
    Vpn::ConnectionState getCurrentConnectionState();
    void notifyConnectionBlocked(ErrorCode errorCode);
    void onTrafficRateChanged(quint64 receivedBytesPerSec, quint64 sentBytesPerSec);

    ConnectionController* m_connectionController;
    ServersController* m_serversController;

    bool m_isConnected = false;
    bool m_isConnectionInProgress = false;
    QString m_connectionStateText = tr("Connect");

    bool m_hasTrafficSpeed = false;
    quint64 m_downloadBytesPerSec = 0;
    quint64 m_uploadBytesPerSec = 0;

    Vpn::ConnectionState m_state;
};

#endif
