#ifndef WIREGUARDPROTOCOL_H
#define WIREGUARDPROTOCOL_H

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnProtocol.h"
#include "core/utils/endpointWatchdog.h"

#include "mozilla/controllerimpl.h"

class WireguardProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit WireguardProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~WireguardProtocol() override;

    ErrorCode start() override;
    void stop() override;

    ErrorCode startMzImpl();
    ErrorCode stopMzImpl();

private:
    QString resolveHostName(const QString &hostName);
    void onPeerStatus(qint64 lastHandshakeMsecs, quint64 txBytes);

    QScopedPointer<ControllerImpl> m_impl;

    // Host names exactly as configured. They are resolved again on every
    // start, so a reconnect follows a change of the address behind the name.
    QString m_hostName;
    QString m_configHostName;
    QHash<QString, QString> m_lastAddresses;

    // Polls the peer while the tunnel is up and asks for a reconnect when the
    // server stopped answering (only if the endpoint is a host name).
    bool m_watchEndpoint = false;
    QTimer m_statusTimer;
    EndpointWatchdog m_endpointWatchdog;
};

#endif // WIREGUARDPROTOCOL_H
