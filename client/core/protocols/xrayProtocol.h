#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"
#include <QtCore/qsharedpointer.h>
#include <QHostAddress>
#include <QList>

#include <functional>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/ipcClient.h"
#include "vpnProtocol.h"

class QTimer;

class XrayProtocol : public VpnProtocol
{
public:
    XrayProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~XrayProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    ErrorCode setupRouting();
    ErrorCode startTun2Socks();

    // Step 3: fast pre-connect TCP probe to the real VPN server endpoint.
    bool probeServerReachable();
    // Step 2/4: end-to-end probe through the local SOCKS5 proxy (xray -> VPS -> internet).
    void runConnectivityProbe(std::function<void(bool)> onResult);
    // Step 4: periodic liveness monitoring after the tunnel is established.
    void startLivenessMonitor();
    void stopLivenessMonitor();
    int extractServerPort() const;

    QJsonObject m_xrayConfig;
    amnezia::RouteMode m_routeMode;
    QList<QHostAddress> m_dnsServers;
    QString m_remoteAddress;
    int m_serverPort = 0;

    QString m_socksUser;
    QString m_socksPassword;
    int m_socksPort = 10808;

    QSharedPointer<IpcProcessInterfaceReplica> m_tun2socksProcess;
    int m_tun2socksRetryCount = 0;
    static constexpr int maxTun2SocksRetries = 5;
    static constexpr int tun2socksRetryDelayMs = 400;

    bool m_connectivityProbeStarted = false;
    bool m_tunResourceBusy = false;

    QTimer *m_livenessTimer = nullptr;
    int m_livenessFailures = 0;

    static constexpr int defaultServerProbeTimeoutMs = 5000;
    static constexpr int defaultConnectivityProbeTimeoutMs = 7000;
    static constexpr int defaultLivenessIntervalMs = 15000;
    static constexpr int defaultMaxLivenessFailures = 3;

    int m_serverProbeTimeoutMs = defaultServerProbeTimeoutMs;
    int m_connectivityProbeTimeoutMs = defaultConnectivityProbeTimeoutMs;
    int m_livenessIntervalMs = defaultLivenessIntervalMs;
    int m_maxLivenessFailures = defaultMaxLivenessFailures;
};

#endif // XRAYPROTOCOL_H
