#include "ipcserver.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QRemoteObjectHost>
#include <QRemoteObjectNode>
#include <QString>
#include <QStringList>

#include "logger.h"
#include "router.h"
#include "killswitch.h"

#include "../client/daemon/daemon.h"

#ifdef Q_OS_MAC
#include "router_mac.h"
#include "core/utils/networkUtilities.h"
#include <QNetworkInterface>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_utun.h>
#include <unistd.h>
#include <cstring>
#endif

#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    #include "tapcontroller_win.h"
#endif


IpcServer::IpcServer(QObject *parent) : IpcInterfaceSource(parent)
{
    connect(&m_pingHelper, &PingHelper::connectionLose, this, &IpcServer::connectionLose);
}

int IpcServer::createPrivilegedProcess()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::createPrivilegedProcess";
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

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::error, this,
                     [pd](QRemoteObjectNode::ErrorCode errorCode) { qDebug() << "QRemoteObjectHost::error" << errorCode; });

    QObject::connect(pd.serverNode.data(), &QRemoteObjectHost::destroyed, this, [pd]() { qDebug() << "QRemoteObjectHost::destroyed"; });

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

int IpcServer::routeAddListVia(const QString &ifname, const QString &gw, const QStringList &ips)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::routeAddListVia" << ifname;
#endif

    return Router::routeAddListVia(ifname, gw, ips);
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

    return Router::routeDeleteList(gw, ips);
}

bool IpcServer::addExclusionRoute(const QString &ifname, const QString &addr)
{
    return Daemon::instance() && Daemon::instance()->addExclusionRoute(ifname, addr);
}

bool IpcServer::delExclusionRoute(const QString &ifname, const QString &addr)
{
    return Daemon::instance() && Daemon::instance()->delExclusionRoute(ifname, addr);
}

bool IpcServer::addAllowedIp(const QString &ifname, const QString &prefix)
{
    return Daemon::instance() && Daemon::instance()->addAllowedIp(ifname, prefix);
}

bool IpcServer::delAllowedIp(const QString &ifname, const QString &prefix)
{
    return Daemon::instance() && Daemon::instance()->delAllowedIp(ifname, prefix);
}

bool IpcServer::setTunnelResolvers(const QString &ifname, const QStringList &resolvers)
{
    return Daemon::instance() && Daemon::instance()->setTunnelResolvers(ifname, resolvers);
}

bool IpcServer::restoreTunnelResolvers()
{
    return Daemon::instance() && Daemon::instance()->restoreTunnelResolvers();
}

bool IpcServer::flushDns()
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

    Logger::deInit();
    Logger::cleanUp();
}

void IpcServer::clearLogs()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::clearLogs";
#endif

    Logger::clearLogs(true);
}

bool IpcServer::createTun(const QString &dev, const QString &subnet)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::createTun";
#endif

    return Router::createTun(dev, subnet);
}

bool IpcServer::deleteTun(const QString &dev)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::deleteTun";
#endif

    return Router::deleteTun(dev);
}

QString IpcServer::reserveUtunName()
{
#ifdef Q_OS_MAC
    int fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        qWarning() << "reserveUtunName: socket() failed:" << strerror(errno);
        return QString();
    }

    struct ctl_info info;
    std::memset(&info, 0, sizeof(info));
    std::memcpy(info.ctl_name, UTUN_CONTROL_NAME, sizeof(UTUN_CONTROL_NAME));
    if (ioctl(fd, CTLIOCGINFO, &info) < 0) {
        qWarning() << "reserveUtunName: CTLIOCGINFO failed:" << strerror(errno);
        ::close(fd);
        return QString();
    }

    struct sockaddr_ctl addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYSTEM;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_id = info.ctl_id;
    addr.sc_unit = 0;

    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        qWarning() << "reserveUtunName: connect() failed:" << strerror(errno);
        ::close(fd);
        return QString();
    }

    char ifname[IFNAMSIZ] = {0};
    socklen_t len = sizeof(ifname);
    if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &len) < 0) {
        qWarning() << "reserveUtunName: getsockopt UTUN_OPT_IFNAME failed:" << strerror(errno);
        ::close(fd);
        return QString();
    }

    ::close(fd);
    return QString::fromUtf8(ifname);
#else
    return QString();
#endif
}

bool IpcServer::applyAdapterAddress(const QString &ifname, const QString &ipv4, const QString &ipv6)
{
#ifdef Q_OS_WIN
    bool ok = true;
    if (!ipv4.isEmpty()) {
        ok &= Router::createTun(ifname, ipv4);
    }
    if (!ipv6.isEmpty()) {
        NET_LUID luid;
        if (ConvertInterfaceAliasToLuid(reinterpret_cast<const wchar_t*>(ifname.utf16()), &luid) != NO_ERROR) {
            qWarning() << "IpcServer::applyAdapterAddress: cannot resolve" << ifname;
            return false;
        }
        const QByteArray ip = ipv6.section('/', 0, 0).toUtf8();
        MIB_UNICASTIPADDRESS_ROW row;
        InitializeUnicastIpAddressEntry(&row);
        row.InterfaceLuid.Value = luid.Value;
        row.Address.si_family = AF_INET6;
        row.OnLinkPrefixLength = 128;
        row.DadState = IpDadStatePreferred;
        if (InetPtonA(AF_INET6, ip.toStdString().c_str(), &row.Address.Ipv6.sin6_addr) != 1) {
            qWarning() << "IpcServer::applyAdapterAddress: cannot parse" << ipv6;
            return false;
        }
        DWORD r = CreateUnicastIpAddressEntry(&row);
        ok &= (r == NO_ERROR || r == ERROR_OBJECT_ALREADY_EXISTS);
    }
    return ok;
#else
    Q_UNUSED(ifname)
    Q_UNUSED(ipv4)
    Q_UNUSED(ipv6)
    return true;
#endif
}

bool IpcServer::removeAdapterAddress(const QString &ifname, const QString &ipv4, const QString &ipv6)
{
#ifdef Q_OS_WIN
    NET_LUID luid;
    if (ConvertInterfaceAliasToLuid(reinterpret_cast<const wchar_t*>(ifname.utf16()), &luid) != NO_ERROR) {
        qWarning() << "IpcServer::removeAdapterAddress: cannot resolve" << ifname;
        return false;
    }

    auto removeOne = [&](const QString& addr, int family) -> bool {
        if (addr.isEmpty()) return true;
        const QByteArray ip = addr.section('/', 0, 0).toUtf8();
        MIB_UNICASTIPADDRESS_ROW row;
        InitializeUnicastIpAddressEntry(&row);
        row.InterfaceLuid.Value = luid.Value;
        row.Address.si_family = static_cast<ADDRESS_FAMILY>(family);
        void* dst = (family == AF_INET)
                      ? static_cast<void*>(&row.Address.Ipv4.sin_addr)
                      : static_cast<void*>(&row.Address.Ipv6.sin6_addr);
        if (InetPtonA(family, ip.toStdString().c_str(), dst) != 1) return false;
        DWORD r = DeleteUnicastIpAddressEntry(&row);
        return r == NO_ERROR || r == ERROR_NOT_FOUND;
    };

    bool ok = removeOne(ipv4, AF_INET);
    ok &= removeOne(ipv6, AF_INET6);
    return ok;
#else
    Q_UNUSED(ifname)
    Q_UNUSED(ipv4)
    Q_UNUSED(ipv6)
    return true;
#endif
}

bool IpcServer::updateResolvers(const QString &ifname, const QList<QHostAddress> &resolvers)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::updateResolvers";
#endif

    return Router::updateResolvers(ifname, resolvers);
}

bool IpcServer::restoreResolvers()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::restoreResolvers";
#endif

    if (m_xrayWorkers.size() > 1) {
        return true;
    }
    return Router::restoreResolvers();
}

bool IpcServer::StartRoutingIpv6()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::StartRoutingIpv6";
#endif

    return Router::StartRoutingIpv6();
}

bool IpcServer::StopRoutingIpv6()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::StopRoutingIpv6";
#endif

    return Router::StopRoutingIpv6();
}

void IpcServer::setLogsEnabled(bool enabled)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::setLogsEnabled";
#endif

    if (enabled) {
        Logger::init(true);
    } else {
        Logger::deInit();
    }
}

bool IpcServer::startNetworkCheck(const QString& serverIpv4Gateway, const QString& deviceIpv4Address)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::startNetworkCheck";
#endif

    m_pingHelper.start(serverIpv4Gateway, deviceIpv4Address);
    return true;
}

bool IpcServer::stopNetworkCheck()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::stopNetworkCheck";
#endif

    m_pingHelper.stop();
    return true;
}

bool IpcServer::resetKillSwitchAllowedRange(QStringList ranges)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::resetKillSwitchAllowedRange";
#endif

    return KillSwitch::instance()->resetAllowedRange(ranges);
}

bool IpcServer::addKillSwitchAllowedRange(const QString &ifname, QStringList ranges)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::addKillSwitchAllowedRange" << ifname;
#endif

    return KillSwitch::instance()->addAllowedRange(ifname, ranges);
}

bool IpcServer::disableAllTraffic()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::disableAllTraffic";
#endif

    return KillSwitch::instance()->disableAllTraffic();
}

bool IpcServer::enableKillSwitch(const QJsonObject &configStr, int vpnAdapterIndex)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::enableKillSwitch";
#endif

    return KillSwitch::instance()->enableKillSwitch(configStr, vpnAdapterIndex);
}

bool IpcServer::disableKillSwitch()
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::disableKillSwitch";
#endif

    return KillSwitch::instance()->disableKillSwitch();
}

bool IpcServer::disableKillSwitchForTunnel(const QString &ifname, const QStringList &remainingRanges)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::disableKillSwitchForTunnel" << ifname;
#endif

    return KillSwitch::instance()->disableKillSwitchForTunnel(ifname, remainingRanges);
}

bool IpcServer::enablePeerTraffic(const QJsonObject &configStr)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::enablePeerTraffic";
#endif

    return KillSwitch::instance()->enablePeerTraffic(configStr);
}

bool IpcServer::refreshKillSwitch(bool enabled)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::refreshKillSwitch";
#endif

    return KillSwitch::instance()->refresh(enabled);
}

void IpcServer::onXrayWorkerLine(const QString& ifname, const QByteArray& line)
{
    const QJsonObject ev = QJsonDocument::fromJson(line).object();
    const QString name = ev.value("ev").toString();
    if (name == "log") {
        const QString prefix = QStringLiteral("[xray-worker:%1]").arg(ifname);
        const QString level = ev.value("level").toString();
        const QString msg   = ev.value("msg").toString();
        if (level == QLatin1String("warn")) {
            qWarning().noquote() << prefix << msg;
        } else if (level == QLatin1String("error") || level == QLatin1String("fatal")) {
            qCritical().noquote() << prefix << msg;
        } else if (level == QLatin1String("info")) {
            qInfo().noquote() << prefix << msg;
        } else {
            qDebug().noquote() << prefix << msg;
        }
    } else if (name == "ready" || name == "failed") {
        auto it = m_xrayWorkers.find(ifname);
        if (it != m_xrayWorkers.end() && it->startLoop) {
            it->startResult = (name == "ready");
            it->startLoop->quit();
        }
    }
}

bool IpcServer::xrayStart(const QString& ifname, const QString& cfg)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::xrayStart" << ifname;
#endif

    XrayWorker& w = m_xrayWorkers[ifname];

    if (!w.process || w.process->state() == QProcess::NotRunning) {
        w.process = QSharedPointer<QProcess>::create();
        w.stdoutBuf.clear();

        QObject::connect(w.process.data(), &QProcess::readyReadStandardOutput, this, [this, ifname]() {
            auto it = m_xrayWorkers.find(ifname);
            if (it == m_xrayWorkers.end() || !it->process) return;
            it->stdoutBuf.append(it->process->readAllStandardOutput());
            int nl;
            while ((nl = it->stdoutBuf.indexOf('\n')) >= 0) {
                const QByteArray line = it->stdoutBuf.left(nl);
                it->stdoutBuf.remove(0, nl + 1);
                onXrayWorkerLine(ifname, line);
            }
        });

        QObject::connect(w.process.data(), &QProcess::errorOccurred, this,
                         [this, ifname](QProcess::ProcessError err) {
            const QString prefix = QStringLiteral("[xray-worker:%1]").arg(ifname);
            qCritical().noquote().nospace() << prefix << " process error: " << err;
            auto it = m_xrayWorkers.find(ifname);
            if (it != m_xrayWorkers.end() && it->startLoop) {
                it->startResult = false;
                it->startLoop->quit();
            }
        });

        QObject::connect(w.process.data(),
                         QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         this, [this, ifname](int code, QProcess::ExitStatus status) {
            const QString prefix = QStringLiteral("[xray-worker:%1]").arg(ifname);
            qDebug().noquote().nospace() << prefix << " finished, code=" << code << " status=" << status;
            auto it = m_xrayWorkers.find(ifname);
            if (it != m_xrayWorkers.end() && it->startLoop) {
                it->startResult = false;
                it->startLoop->quit();
            }
        });

        w.process->setProgram(QCoreApplication::applicationFilePath());
        w.process->setArguments({QStringLiteral("--xray-worker")});
        w.process->start();

        if (!w.process->waitForStarted(5000)) {
            qCritical().noquote() << QStringLiteral("[xray-worker:%1] failed to start").arg(ifname);
            m_xrayWorkers.remove(ifname);
            return false;
        }
    }

    const QJsonObject startCmd{{QStringLiteral("op"), QStringLiteral("start")},
                               {QStringLiteral("config"), cfg}};
    w.process->write(QJsonDocument(startCmd).toJson(QJsonDocument::Compact) + '\n');

    QEventLoop loop;
    w.startLoop = &loop;
    w.startResult = false;
    loop.exec();

    auto it = m_xrayWorkers.find(ifname);
    if (it == m_xrayWorkers.end()) {
        return false;
    }
    it->startLoop.clear();
    const bool ok = it->startResult;

    if (!ok) {
        m_xrayWorkers.remove(ifname);
    }

    return ok;
}

bool IpcServer::xrayStop(const QString& ifname)
{
#ifdef MZ_DEBUG
    qDebug() << "IpcServer::xrayStop" << ifname;
#endif

    auto it = m_xrayWorkers.find(ifname);
    if (it == m_xrayWorkers.end()) {
        return true;
    }

    if (it->process && it->process->state() != QProcess::NotRunning) {
        const QJsonObject stopCmd{{QStringLiteral("op"), QStringLiteral("stop")}};
        it->process->write(QJsonDocument(stopCmd).toJson(QJsonDocument::Compact) + '\n');

        if (!it->process->waitForFinished()) {
            qWarning().noquote() << QStringLiteral("[xray-worker:%1] did not exit after stop, killing").arg(ifname);
            it->process->kill();
            it->process->waitForFinished();
        }
    }

    m_xrayWorkers.remove(ifname);
    return true;
}

bool IpcServer::xrayAddUplinkRoutes(const QString& uplinkIface, const QString& uplinkGateway)
{
#ifdef Q_OS_MAC
    if (uplinkIface.isEmpty() || uplinkGateway.isEmpty()) {
        return false;
    }
    return RouterMac::Instance().routeAddXray(uplinkIface, uplinkGateway);
#else
    Q_UNUSED(uplinkIface)
    Q_UNUSED(uplinkGateway)
    return true;
#endif
}

bool IpcServer::xrayRemoveUplinkRoutes(const QString& uplinkIface, const QString& uplinkGateway)
{
#ifdef Q_OS_MAC
    if (uplinkIface.isEmpty()) {
        return false;
    }
    if (m_xrayWorkers.size() > 1) {
        return true;
    }
    return RouterMac::Instance().routeDeleteXray(uplinkIface, uplinkGateway);
#else
    Q_UNUSED(uplinkIface)
    Q_UNUSED(uplinkGateway)
    return true;
#endif
}
