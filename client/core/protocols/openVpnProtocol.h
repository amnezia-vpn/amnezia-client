#ifndef OPENVPNPROTOCOL_H
#define OPENVPNPROTOCOL_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "core/utils/managementServer.h"
#include "vpnProtocol.h"

#include "core/utils/ipcClient.h"

class OpenVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit OpenVpnProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~OpenVpnProtocol() override;

    ErrorCode start() override;
    void stop() override;

    ErrorCode prepare() override;
    static QString defaultConfigFileName();
    static QString defaultConfigPath();

protected slots:
    void onReadyReadDataFromManagementServer();

private:
    void cleanupResources();
    QString configPath() const;
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
    QJsonObject m_configData;
    QTemporaryFile m_configFile;

    uint selectMgmtPort();

private:
    void updateRouteGateway(QString line);
    void updateVpnGateway(const QString &line);

    QSharedPointer<IpcProcessInterfaceReplica> m_openVpnProcess;

#ifdef Q_OS_WIN
    // openvpn falls back to tap-windows6 on its own for configs the ovpn-dco
    // driver cannot handle (compression, --fragment, proxies, non-AEAD
    // ciphers, --dev tap). Only the DCO adapter is provisioned up front, so
    // that fallback dies with "There are no TAP-Windows or ovpn-dco adapters
    // on this system" — install the legacy driver on demand and retry once.
    bool retryWithLegacyDriver();

    bool m_legacyDriverRequested = false;
#endif
};

#endif // OPENVPNPROTOCOL_H
