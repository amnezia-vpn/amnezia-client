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

    ErrorCode prepare() override;
    static QString defaultConfigFileName();
    static QString defaultConfigPath();

    void waitForDisconected(int msecs) override;

protected slots:
    void onReadyReadDataFromManagementServer();

private:
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

    QSharedPointer<PrivilegedProcess> m_openVpnProcess;
};

#endif // OPENVPNPROTOCOL_H
