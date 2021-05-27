#include "ipcserver.h"

#include <QObject>
#include <QDateTime>
#include <QLocalSocket>

#include "router.h"
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

//    connect(m_server.data(), &QLocalServer::newConnection, this, &LocalServer::onNewConnection);

//    qDebug().noquote() << QString("Local server started on '%1'").arg(m_server->serverName());

//    m_serverNode.setHostUrl(QUrl(QStringLiteral(IPC_SERVICE_URL))); // create host node without Registry



    // Make sure any connections are handed to QtRO
    QObject::connect(pd.localServer.data(), &QLocalServer::newConnection, this, [pd]() {
        qDebug() << "LocalServer new connection";
        if (pd.serverNode) {
            pd.serverNode->addHostSideConnection(pd.localServer->nextPendingConnection());
            pd.serverNode->enableRemoting(pd.ipcProcess.data());
        }
    });

    m_processes.insert(m_localpid, pd);

    return m_localpid;
}

bool IpcServer::routeAdd(const QString &ip, const QString &gw)
{
    return Router::routeAdd(ip, gw);
}

int IpcServer::routeAddList(const QString &gw, const QStringList &ips)
{
    return Router::routeAddList(gw, ips);
}

bool IpcServer::clearSavedRoutes()
{
    return Router::clearSavedRoutes();
}

bool IpcServer::routeDelete(const QString &ip, const QString &gw)
{
    return Router::routeDelete(ip, gw);
}

void IpcServer::flushDns()
{
    return Router::flushDns();
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
