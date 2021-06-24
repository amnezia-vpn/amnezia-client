#ifndef WIREGUARDPROTOCOL_H
#define WIREGUARDPROTOCOL_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

class WireguardProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit WireguardProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~WireguardProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    QString configPath() const;
    QString wireguardExecPath() const;
    //bool openVpnProcessIsRunning() const;
    void readWireguardConfiguration(const QJsonObject &configuration);

    void updateRouteGateway(QString line);
    void updateVpnGateway(const QString &line);
    QString serviceName() const;


private:
    QString m_configFileName;
    QFile m_configFile;

    QSharedPointer<IpcProcessInterfaceReplica> m_wireguardStartProcess;
    QSharedPointer<IpcProcessInterfaceReplica> m_wireguardStopProcess;

    bool m_isConfigLoaded = false;

};

#endif // WIREGUARDPROTOCOL_H
