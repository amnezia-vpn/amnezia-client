#ifndef WIREGUARDPROTOCOL_H
#define WIREGUARDPROTOCOL_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"

#include "mozilla/controllerimpl.h"

class WireguardProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit WireguardProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~WireguardProtocol() override;

    ErrorCode start() override;
    void stop() override;
    void statusUpdated(const QString& serverIpv4Gateway, const QString& deviceIpv4Address,
                       uint64_t txBytes, uint64_t rxBytes);
    ErrorCode startMzImpl();
    ErrorCode stopMzImpl();

private:

    QScopedPointer<ControllerImpl> m_impl;
};

#endif // WIREGUARDPROTOCOL_H
