#ifndef IKEV2_VPN_PROTOCOL_MACOS_H
#define IKEV2_VPN_PROTOCOL_MACOS_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include "vpnProtocol.h"

#if defined(__OBJC__)
    #include <NetworkExtension/NetworkExtension.h>
#endif

class Ikev2ProtocolMacos : public VpnProtocol
{
    Q_OBJECT

public:
    explicit Ikev2ProtocolMacos(const QJsonObject &configuration, QObject *parent = nullptr);
    ~Ikev2ProtocolMacos() override;

    ErrorCode start() override;
    void stop() override;

    static QString tunnelName() { return "AmneziaVPN IKEv2"; }

private:
    void readIkev2Configuration(const QJsonObject &configuration);

    bool storeClientIdentity();

    void handleStatusChange(int rawStatus);

    void startHandshakeTimeoutTimer();
    void stopHandshakeTimeoutTimer();

    void removeStatusObserver();

private:
    QJsonObject m_config;

    QString m_hostName;
    QString m_clientId;
    QString m_clientCertBase64;
    QString m_clientCertPassword;

    QTimer *m_handshakeTimeoutTimer { nullptr };
    bool m_handshakeTimedOut { false };

    void *m_statusObserver { nullptr };

    int m_lastVpnStatus { 0 };

    static constexpr int HANDSHAKE_TIMEOUT_SEC = 20;
};

#endif // IKEV2_VPN_PROTOCOL_MACOS_H
