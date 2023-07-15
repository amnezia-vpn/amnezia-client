#ifndef WIREGUARDPROTOCOL_H
#define WIREGUARDPROTOCOL_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

#include "mozilla/controllerimpl.h"

class WireguardProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit WireguardProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~WireguardProtocol() override;

    ErrorCode start() override;
    void stop() override;

#ifdef Q_OS_MAC
    ErrorCode startMzImpl();
    ErrorCode stopMzImpl();
#endif

private:
    QString configPath() const;
    void writeWireguardConfiguration(const QJsonObject &configuration);

    void updateRouteGateway(QString line);
    void updateVpnGateway(const QString &line);
    QString serviceName() const;
    QStringList stopArgs();
    QStringList startArgs();

private:
    QString m_configFileName;
    QFile m_configFile;

    QSharedPointer<PrivilegedProcess> m_wireguardStartProcess;
    QSharedPointer<PrivilegedProcess> m_wireguardStopProcess;

    bool m_isConfigLoaded = false;

#ifdef Q_OS_MAC
    QScopedPointer<ControllerImpl> m_impl;
#endif
};

#endif // WIREGUARDPROTOCOL_H
