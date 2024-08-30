#include "ipcserver.h"

#include <QObject>
#include <QDateTime>
#include <QLocalSocket>
#include <QFileInfo>

#include "qjsonarray.h"
#include "router.h"
#include "logger.h"

#include "../client/protocols/protocols_defs.h"
#ifdef Q_OS_WIN
#include "tapcontroller_win.h"
#include "../client/platforms/windows/daemon/windowsfirewall.h"
#include "../client/platforms/windows/daemon/windowsdaemon.h"
#endif

#ifdef Q_OS_LINUX
#include "../client/platforms/linux/daemon/linuxfirewall.h"
#endif

#ifdef Q_OS_MACOS
#include "../client/platforms/macos/daemon/macosfirewall.h"
#endif

IpcServer::IpcServer(QObject *parent):
    IpcInterfaceSource(parent)

{}

int IpcServer::createPrivilegedProcess()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::createPrivilegedProcess";
#endif

#ifdef Q_OS_WIN
    WindowsFirewall::instance()->init();
#endif

    m_localpid++;

    ProcessDescriptor pd(this);

    pd.localServer->setSocketOptions(QLocalServer::WorldAccessOption);

    if (!pd.localServer->listen(amnezia::getIpcProcessUrl(m_localpid))) {
        qDebug() << QString("Unable to start the server: %1.").arg(pd.localServer->errorString());
        return -1;
    }

    // Make sure any connections are handed to QtRO
    QObject::connect(pd.localServer.data(), &QLocalServer::newConnection, this, [pd]() {
        qDebug() << "IpcServer new connection";
        if (pd.serverNode) {
            pd.serverNode->addHostSideConnection(pd.localServer->nextPendingConnection());
            pd.serverNode->enableRemoting(pd.ipcProcess.data());
        }
    });

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::error, this, [pd](QRemoteObjectNode::ErrorCode errorCode) {
        qDebug() << "QRemoteObjectHost::error" << errorCode;
    });

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::destroyed, this, [pd]() {
        qDebug() << "QRemoteObjectHost::destroyed";
    });

//    connect(pd.ipcProcess.data(), &IpcServerProcess::finished, this, [this, pid=m_localpid](int exitCode, QProcess::ExitStatus exitStatus){
//        qDebug() << "IpcServerProcess finished" << exitCode << exitStatus;
////        if (m_processes.contains(pid)) {
////            m_processes[pid].ipcProcess.reset();
////            m_processes[pid].serverNode.reset();
////            m_processes[pid].localServer.reset();
////            m_processes.remove(pid);
////        }
//    });

    m_processes.insert(m_localpid, pd);

    return m_localpid;
}

int IpcServer::routeAddList(const QString &gw, const QStringList &ips)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::routeAddList";
#endif

    return Router::routeAddList(gw, ips);
}

bool IpcServer::clearSavedRoutes()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::clearSavedRoutes";
#endif

    return Router::clearSavedRoutes();
}

bool IpcServer::routeDeleteList(const QString &gw, const QStringList &ips)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::routeDeleteList";
#endif

    return Router::routeDeleteList(gw ,ips);
}

void IpcServer::flushDns()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::flushDns";
#endif

    return Router::flushDns();
}

void IpcServer::resetIpStack()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::resetIpStack";
#endif

    Router::resetIpStack();
}

bool IpcServer::checkAndInstallDriver()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::checkAndInstallDriver";
#endif

#ifdef Q_OS_WIN
    return TapController::checkAndSetup();
#else
    return true;
#endif
}

QStringList IpcServer::getTapList()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::getTapList";
#endif

#ifdef Q_OS_WIN
    return TapController::getTapList();
#else
    return QStringList();
#endif
}

void IpcServer::cleanUp()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::cleanUp";
#endif

    Logger::deinit();
    Logger::cleanUp();
}

void IpcServer::clearLogs() {
    Logger::clearLogs();
}

bool IpcServer::createTun(const QString &dev, const QString &subnet)
{
    return Router::createTun(dev, subnet);
}

bool IpcServer::deleteTun(const QString &dev)
{
    return Router::deleteTun(dev);
}

bool IpcServer::updateResolvers(const QString& ifname, const QList<QHostAddress>& resolvers)
{
    return Router::updateResolvers(ifname, resolvers);
}

void IpcServer::StartRoutingIpv6()
{
    Router::StartRoutingIpv6();
}

void IpcServer::StopRoutingIpv6()
{
    Router::StopRoutingIpv6();
}

void IpcServer::setLogsEnabled(bool enabled)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::setLogsEnabled";
#endif

    if (enabled) {
        Logger::init();
    }
    else {
        Logger::deinit();
    }
}

bool IpcServer::enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex)
{
#ifdef Q_OS_WIN
    return WindowsFirewall::instance()->enableKillSwitch(vpnAdapterIndex);
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    int splitTunnelType = configStr.value("splitTunnelType").toInt();
    QJsonArray splitTunnelSites = configStr.value("splitTunnelSites").toArray();
    bool blockAll = 0;
    bool allowNets = 0;
    bool blockNets = 0;
    QStringList allownets;
    QStringList blocknets;

    if (splitTunnelType == 0)
    {
        blockAll = true;
        allowNets = true;
        allownets.append(configStr.value(amnezia::config_key::hostName).toString());
    } else if (splitTunnelType == 1)
    {
        blockNets = true;
        for (auto v : splitTunnelSites) {
            blocknets.append(v.toString());
        }
    } else if (splitTunnelType == 2) {
        blockAll = true;
        allowNets = true;
        allownets.append(configStr.value(amnezia::config_key::hostName).toString());
        for (auto v : splitTunnelSites) {
            allownets.append(v.toString());
        }
    }
#endif

#ifdef Q_OS_LINUX
    // double-check + ensure our firewall is installed and enabled
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), blockAll);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), allowNets);
    LinuxFirewall::updateAllowNets(allownets);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), blockAll);
    LinuxFirewall::updateBlockNets(blocknets);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), true);
    QStringList dnsServers;
    dnsServers.append(configStr.value(amnezia::config_key::dns1).toString());
    dnsServers.append(configStr.value(amnezia::config_key::dns2).toString());
    dnsServers.append("127.0.0.1");
    dnsServers.append("127.0.0.53");
    LinuxFirewall::updateDNSServers(dnsServers);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), true);
#endif

#ifdef Q_OS_MACOS

    // double-check + ensure our firewall is installed and enabled. This is necessary as
    // other software may disable pfctl before re-enabling with their own rules (e.g other VPNs)
    if (!MacOSFirewall::isInstalled()) MacOSFirewall::install();

    MacOSFirewall::ensureRootAnchorPriority();
    MacOSFirewall::setAnchorEnabled(QStringLiteral("000.allowLoopback"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("100.blockAll"), blockAll);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("110.allowNets"), allowNets);
    MacOSFirewall::setAnchorTable(QStringLiteral("110.allowNets"), allowNets,
                                  QStringLiteral("allownets"), allownets);

    MacOSFirewall::setAnchorEnabled(QStringLiteral("120.blockNets"), blockNets);
    MacOSFirewall::setAnchorTable(QStringLiteral("120.blockNets"), blockNets,
                                  QStringLiteral("blocknets"), blocknets);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("200.allowVPN"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("250.blockIPv6"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("290.allowDHCP"), true);
    MacOSFirewall::setAnchorEnabled(QStringLiteral("300.allowLAN"), true);

    QStringList dnsServers;
    dnsServers.append(configStr.value(amnezia::config_key::dns1).toString());
    dnsServers.append(configStr.value(amnezia::config_key::dns2).toString());
    MacOSFirewall::setAnchorEnabled(QStringLiteral("310.blockDNS"), true);
    MacOSFirewall::setAnchorTable(QStringLiteral("310.blockDNS"), true, QStringLiteral("dnsaddr"), dnsServers);
#endif
    return true;
}

bool IpcServer::disableKillSwitch()
{
#ifdef Q_OS_WIN
    return WindowsFirewall::instance()->disableKillSwitch();
#endif

#ifdef Q_OS_LINUX
    LinuxFirewall::uninstall();
#endif

#ifdef Q_OS_MACOS
    MacOSFirewall::uninstall();
#endif

    return true;
}

bool IpcServer::startIPsec(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess processSystemd;
    QStringList commandsSystemd;
    commandsSystemd << "systemctl" << "restart" << "ipsec";
    processSystemd.start("sudo", commandsSystemd);
    if (!processSystemd.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel!\n";
        return false;
    }
    else if (!processSystemd.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel\n";
        return false;
    }
    commandsSystemd.clear();

    QThread::msleep(5000);

    QProcess process;
    QStringList commands;
    commands << "ipsec" << "up" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel!\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not start ipsec tunnel\n";
        return false;
    }
    commands.clear();
#endif
    return true;
}

bool IpcServer::stopIPsec(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess process;
    QStringList commands;
    commands << "ipsec" << "down" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return false;
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return false;
    }
    commands.clear();
#endif
    return true;
}

bool IpcServer::writeIPsecConfig(QString config)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: IPSec config file";
    QString configFile = QString("/etc/ipsec.conf");
    QFile ipSecConfFile(configFile);
    if (ipSecConfFile.open(QIODevice::WriteOnly)) {
        ipSecConfFile.write(config.toUtf8());
        ipSecConfFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecUserCert(QString usercert, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: Write user cert " << uuid;
    QString certName = QString("/etc/ipsec.d/certs/%1.crt").arg(uuid);
    QFile userCertFile(certName);
    if (userCertFile.open(QIODevice::WriteOnly)) {
        userCertFile.write(usercert.toUtf8());
        userCertFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecCaCert(QString cacert, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: Write CA cert user " << uuid;
    QString certName = QString("/etc/ipsec.d/cacerts/%1.crt").arg(uuid);
    QFile caCertFile(certName);
    if (caCertFile.open(QIODevice::WriteOnly)) {
        caCertFile.write(cacert.toUtf8());
        caCertFile.close();
    }
#endif
    return true;
}

bool IpcServer::writeIPsecPrivate(QString privKey, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: User private key " << uuid;
    QString privateKey = QString("/etc/ipsec.d/private/%1.p12").arg(uuid);
    QFile pKeyFile(privateKey);
    if (pKeyFile.open(QIODevice::WriteOnly)) {
        pKeyFile.write(QByteArray::fromBase64(privKey.toUtf8()));
        pKeyFile.close();
    }
#endif
    return true;
}


bool IpcServer::writeIPsecPrivatePass(QString pass, QString host, QString uuid)
{
#ifdef Q_OS_LINUX
    qDebug() << "IPSEC: User private key " << uuid;
    const QString secretsFilename = "/etc/ipsec.secrets";
    QStringList lines;

    {
        QFile secretsFile(secretsFilename);
        if (secretsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream edit(&secretsFile);
            while (!edit.atEnd()) lines.push_back(edit.readLine());
        }
        secretsFile.close();
    }

    for (auto iter = lines.begin(); iter!=lines.end();)
    {
        if (iter->contains(host))
        {
            iter = lines.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    {
        QFile secretsFile(secretsFilename);
        if (secretsFile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream edit(&secretsFile);
            for (int i=0; i<lines.size(); i++) edit << lines[i] << Qt::endl;
        }
        QString P12 = QString("%any %1 : P12 %2.p12 \"%3\" \n").arg(host, uuid, pass);
        secretsFile.write(P12.toUtf8());
        secretsFile.close();
    }
#endif
    return true;
}

QString IpcServer::getTunnelStatus(QString tunnelName)
{
#ifdef Q_OS_LINUX
    QProcess process;
    QStringList commands;
    commands << "ipsec" << "status" << QString("%1").arg(tunnelName);
    process.start("sudo", commands);
    if (!process.waitForStarted(1000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return "";
    }
    else if (!process.waitForFinished(2000))
    {
        qDebug().noquote() << "Could not stop ipsec tunnel\n";
        return "";
    }
    commands.clear();


    QString status = process.readAll();
    return status;
#endif
    return QString();

}

bool IpcServer::enablePeerTraffic(const QJsonObject &configStr)
{
#ifdef Q_OS_WIN
    InterfaceConfig config;
    config.m_dnsServer = configStr.value(amnezia::config_key::dns1).toString();
    config.m_serverPublicKey = "openvpn";
    config.m_serverIpv4Gateway = configStr.value("vpnGateway").toString();
    config.m_serverIpv4AddrIn = configStr.value("vpnServer").toString();
    int vpnAdapterIndex = configStr.value("vpnAdapterIndex").toInt();
    int inetAdapterIndex = configStr.value("inetAdapterIndex").toInt();

    int splitTunnelType = configStr.value("splitTunnelType").toInt();
    QJsonArray splitTunnelSites = configStr.value("splitTunnelSites").toArray();

    QStringList AllowedIPAddesses;

    // Use APP split tunnel
    if (splitTunnelType == 0 || splitTunnelType == 2) {
            config.m_allowedIPAddressRanges.append(
                    IPAddress(QHostAddress("0.0.0.0"), 0));
            config.m_allowedIPAddressRanges.append(
                IPAddress(QHostAddress("::"), 0));
    }

    if (splitTunnelType == 1) {
        for (auto v : splitTunnelSites) {
            QString ipRange = v.toString();
            if (ipRange.split('/').size() > 1) {
                config.m_allowedIPAddressRanges.append(
                    IPAddress(QHostAddress(ipRange.split('/')[0]), atoi(ipRange.split('/')[1].toLocal8Bit())));
            } else {
                 config.m_allowedIPAddressRanges.append(
                    IPAddress(QHostAddress(ipRange), 32));
            }
        }
    }

    config.m_excludedAddresses.append(configStr.value(amnezia::config_key::hostName).toString());
    if (splitTunnelType == 2) {
        for (auto v : splitTunnelSites) {
            QString ipRange = v.toString();
            config.m_excludedAddresses.append(ipRange);
        }
    }

    for (const QJsonValue& i : configStr.value(amnezia::config_key::splitTunnelApps).toArray()) {
        if (!i.isString()) {
            break;
        }
        config.m_vpnDisabledApps.append(i.toString());
    }

    // killSwitch toggle
    if (QVariant(configStr.value(amnezia::config_key::killSwitchOption).toString()).toBool()) {
        WindowsFirewall::instance()->enablePeerTraffic(config);
    }

    WindowsDaemon::instance()->prepareActivation(config, inetAdapterIndex);
    WindowsDaemon::instance()->activateSplitTunnel(config, vpnAdapterIndex);
    return true;
#endif
    return true;
}
