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
#endif

#ifdef Q_OS_WIN
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

#ifdef Q_OS_MAC
    const auto gatewayAndIface = NetworkUtilities::getGatewayAndIface();
    w.uplinkGateway = gatewayAndIface.first;
    w.uplinkIface = gatewayAndIface.second.name();
    if (!w.uplinkIface.isEmpty() && !w.uplinkGateway.isEmpty()) {
        if (!RouterMac::Instance().routeAddXray(w.uplinkIface, w.uplinkGateway)) {
            qWarning() << "[xray] failed to install xray routes on" << w.uplinkIface;
        }
    }
#endif

    const QJsonObject startCmd{{QStringLiteral("op"), QStringLiteral("start")},
                               {QStringLiteral("config"), cfg}};
    w.process->write(QJsonDocument(startCmd).toJson(QJsonDocument::Compact) + '\n');

    QEventLoop loop;
    w.startLoop = &loop;
    w.startResult = false;
    loop.exec();

    // Re-fetch: the worker entry may have been removed during the loop (e.g. process finished).
    auto it = m_xrayWorkers.find(ifname);
    if (it == m_xrayWorkers.end()) {
        return false;
    }
    it->startLoop.clear();
    const bool ok = it->startResult;

    if (!ok) {
#ifdef Q_OS_MAC
        if (!it->uplinkIface.isEmpty()) {
            RouterMac::Instance().routeDeleteXray(it->uplinkIface, it->uplinkGateway);
        }
#endif
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

        if (!it->process->waitForFinished(3000)) {
            qWarning().noquote() << QStringLiteral("[xray-worker:%1] did not exit after stop, killing").arg(ifname);
            it->process->kill();
            it->process->waitForFinished(1000);
        }
    }

#ifdef Q_OS_MAC
    if (!it->uplinkIface.isEmpty()) {
        RouterMac::Instance().routeDeleteXray(it->uplinkIface, it->uplinkGateway);
    }
#endif

    m_xrayWorkers.remove(ifname);
    return true;
}
