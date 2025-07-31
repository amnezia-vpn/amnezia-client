#ifndef XRAYPROTOCOL_H
#define XRAYPROTOCOL_H

#include "QProcess"

#include "protocols/vpnprotocol.h"
#include "rep_ipc_process_tun2socks_replica.h"
#include "settings.h"
#include <cstdint>

class XrayProtocol : public VpnProtocol
{
public:
    XrayProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~XrayProtocol() override;

    ErrorCode start() override;
    ErrorCode startTun2Sock();
    void stop() override;

protected:
    void readXrayConfiguration(const QJsonObject &configuration);

protected:
    QJsonObject m_xrayConfig;

private:
    static void ctxSockCallback(uintptr_t fd, void* ctx) {
        reinterpret_cast<XrayProtocol*>(ctx)->sockCallback(fd);
    }
    static void ctxLogHandler(char* str, void* ctx) {
        reinterpret_cast<XrayProtocol*>(ctx)->logHandler(str);
    }

    void sockCallback(uintptr_t fd);
    void logHandler(char* str);

    int m_localPort;
    QString m_remoteHost;
    QString m_remoteAddress;
    Settings::RouteMode m_routeMode;
    QJsonObject m_configData;
    QString m_primaryDNS;
    QString m_secondaryDNS;
#ifndef Q_OS_IOS
    QProcess m_xrayProcess;
    QSharedPointer<IpcProcessTun2SocksReplica> m_t2sProcess;
#endif
    QTemporaryFile m_xrayCfgFile;
};

#endif // XRAYPROTOCOL_H
