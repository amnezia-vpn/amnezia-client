#include "ipcserver.h"

#include <QObject>
#include <QDateTime>
#include <QLocalSocket>
#include <QFileInfo>

#include "router.h"
#include "logger.h"

#ifdef Q_OS_WIN
#include "tapcontroller_win.h"
#endif

IpcServer::IpcServer(QObject *parent):
    IpcInterfaceSource(parent)
{}

int IpcServer::createPrivilegedProcess()
{
    m_localpid++;

    ProcessDescriptor pd(this);
//    pd.serverNode->setHostUrl(QUrl(amnezia::getIpcProcessUrl(m_localpid)));
//    pd.serverNode->enableRemoting(pd.ipcProcess.data());



    //pd.localServer = QSharedPointer<QLocalServer>(new QLocalServer(this));
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
    return Router::routeAddList(gw, ips);
}

bool IpcServer::clearSavedRoutes()
{
    return Router::clearSavedRoutes();
}

bool IpcServer::routeDeleteList(const QString &gw, const QStringList &ips)
{
    return Router::routeDeleteList(gw ,ips);
}

void IpcServer::flushDns()
{
    return Router::flushDns();
}

void IpcServer::resetIpStack()
{
    Router::resetIpStack();
}

bool IpcServer::checkAndInstallDriver()
{
#ifdef Q_OS_WIN
    return TapController::checkAndSetup();
#else
    return true;
#endif
}

QStringList IpcServer::getTapList()
{
#ifdef Q_OS_WIN
    return TapController::getTapList();
#else
    return QStringList();
#endif
}

void IpcServer::cleanUp()
{
    qDebug() << "IpcServer::cleanUp";
    Logger::deinit();
    Logger::cleanUp();
}

void IpcServer::setLogsEnabled(bool enabled)
{
    if (enabled) {
        Logger::init();
    }
    else {
        Logger::deinit();
    }
}

bool IpcServer::copyWireguardConfig(const QString &sourcePath)
{
#ifdef Q_OS_LINUX
    const QString wireguardConfigPath = "/etc/wireguard/wg99.conf";
    if (QFile::exists(wireguardConfigPath))
    {
        QFile::remove(wireguardConfigPath);
    }

    if (!QFile::copy(sourcePath, wireguardConfigPath)) {
        qDebug() << "WireguardProtocol::WireguardProtocol error occurred while copying wireguard config:";
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool IpcServer::isWireguardRunning()
{
#ifdef Q_OS_LINUX
    QProcess checkWireguardStatusProcess;

    connect(&checkWireguardStatusProcess, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qDebug() << "WireguardProtocol::WireguardProtocol error occurred while checking wireguard status: " << error;
    });

    checkWireguardStatusProcess.setProgram("/bin/wg");
    checkWireguardStatusProcess.setArguments(QStringList{"show"});
    checkWireguardStatusProcess.start();
    checkWireguardStatusProcess.waitForFinished(10000);
    QString output = checkWireguardStatusProcess.readAllStandardOutput();
    if (!output.isEmpty()) {
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool IpcServer::isWireguardConfigExists(const QString &configPath)
{
    return QFileInfo::exists(configPath);
}
