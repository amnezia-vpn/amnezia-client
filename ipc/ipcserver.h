#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QLocalServer>
#include <QObject>
#include <QRemoteObjectNode>
#include <QJsonObject>
#include "../client/daemon/interfaceconfig.h"

#include "ipc.h"
#include "ipcserverprocess.h"

#include "rep_ipc_interface_source.h"

class IpcServer : public IpcInterfaceSource
{
public:
    explicit IpcServer(QObject *parent = nullptr);
    virtual int createPrivilegedProcess() override;

    virtual int routeAddList(const QString &gw, const QStringList &ips) override;
    virtual bool clearSavedRoutes() override;
    virtual bool routeDeleteList(const QString &gw, const QStringList &ips) override;
    virtual void flushDns() override;
    virtual void resetIpStack() override;
    virtual bool checkAndInstallDriver() override;
    virtual QStringList getTapList() override;
    virtual void cleanUp() override;
    virtual void clearLogs() override;
    virtual void setLogsEnabled(bool enabled) override;
    virtual bool createTun(const QString &dev, const QString &subnet) override;
    virtual bool deleteTun(const QString &dev) override;
    virtual void StartRoutingIpv6() override;
    virtual void StopRoutingIpv6() override;
    virtual bool enablePeerTraffic(const QJsonObject &configStr) override;
    virtual bool enableKillSwitch(const QJsonObject &excludeAddr, int vpnAdapterIndex) override;
    virtual bool disableKillSwitch() override;
    virtual bool updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers) override;
    virtual bool writeIPsecCaCert(QString cacert, QString uuid) override;
    virtual bool writeIPsecPrivate(QString privKey, QString uuid) override;
    virtual bool writeIPsecConfig(QString config) override;
    virtual bool writeIPsecUserCert(QString usercert, QString uuid) override;
    virtual bool writeIPsecPrivatePass(QString pass, QString host, QString uuid) override;
    virtual bool stopIPsec(QString tunnelName) override;
    virtual bool startIPsec(QString tunnelName) override;
    virtual QString getTunnelStatus(QString tunnelName) override;

private:
    int m_localpid = 0;

    struct ProcessDescriptor {
        ProcessDescriptor (QObject *parent = nullptr) {
            serverNode = QSharedPointer<QRemoteObjectHost>(new QRemoteObjectHost(parent));
            ipcProcess = QSharedPointer<IpcServerProcess>(new IpcServerProcess(parent));
            localServer = QSharedPointer<QLocalServer>(new QLocalServer(parent));
        }

        QSharedPointer<IpcServerProcess> ipcProcess;
        QSharedPointer<QRemoteObjectHost> serverNode;
        QSharedPointer<QLocalServer> localServer;
    };

    QMap<int, ProcessDescriptor> m_processes;
};

#endif // IPCSERVER_H
