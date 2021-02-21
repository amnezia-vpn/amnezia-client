#ifndef OPENVPNPROTOCOL_H
#define OPENVPNPROTOCOL_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "managementserver.h"
#include "vpnprotocol.h"

#include "core/ipcclient.h"

class OpenVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit OpenVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~OpenVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

    ErrorCode checkAndSetupTapDriver();

protected slots:
    void onReadyReadDataFromManagementServer();

private:
    QString configPath() const;
    QString openVpnExecPath() const;
    bool openVpnProcessIsRunning() const;
    bool sendTermSignal();
    void readOpenVpnConfiguration(const QJsonObject &configuration);
    void disconnectFromManagementServer();
    void killOpenVpnProcess();
    void sendByteCount();
    void sendInitialData();
    void sendManagementCommand(const QString& command);

    const QString m_managementHost = "127.0.0.1";
    const unsigned int m_managementPort = 57775;

    ManagementServer m_managementServer;
    QString m_configFileName;
    QTemporaryFile m_configFile;

private:
    void updateRouteGateway(QString line);
    void updateVpnGateway(const QString &line);

    QSharedPointer<IpcProcessInterfaceReplica> m_openVpnProcess;
};

#endif // OPENVPNPROTOCOL_H
