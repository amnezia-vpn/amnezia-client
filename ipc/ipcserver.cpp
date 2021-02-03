#include "ipcserver.h"

#include <QDateTime>
#include <QLocalSocket>

IpcServer::IpcServer(QObject *parent):
    IpcInterfaceSource(parent)
{}

int IpcServer::createPrivilegedProcess()
{
    m_localpid++;

    ProcessDescriptor pd;
//    pd.serverNode->setHostUrl(QUrl(amnezia::getIpcProcessUrl(m_localpid)));
//    pd.serverNode->enableRemoting(pd.ipcProcess.data());



    pd.localServer = QSharedPointer<QLocalServer>(new QLocalServer(this));
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
