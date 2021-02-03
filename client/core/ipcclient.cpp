#include "ipcclient.h"
#include <QRemoteObjectNode>

IpcClient &IpcClient::Instance()
{
    static IpcClient s;
    return s;
}

QSharedPointer<IpcProcessInterfaceReplica> IpcClient::createPrivilegedProcess()
{
    if (! Instance().m_ipcClient || ! Instance().m_ipcClient->isReplicaValid()) {
        qWarning() << "IpcClient::createPrivilegedProcess : IpcClient IpcClient replica is not valid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> futureResult = Instance().m_ipcClient->createPrivilegedProcess();
    futureResult.waitForFinished(1000);

    int pid = futureResult.returnValue();
    QSharedPointer<QRemoteObjectNode> replicaNode(new QRemoteObjectNode);
    //Instance().m_processNodes.insert(pid, replica);


    QSharedPointer<QLocalSocket> socket(new QLocalSocket(replicaNode.data()));
    QSharedPointer<IpcProcessInterfaceReplica> ptr;

    connect(socket.data(), &QLocalSocket::connected, replicaNode.data(), [socket, replicaNode, &ptr]() {
        replicaNode->addClientSideConnection(socket.data());

        ptr.reset(replicaNode->acquire<IpcProcessInterfaceReplica>());

        ptr->waitForSource(1000);

        if (!ptr->isReplicaValid()) {
            qWarning() << "IpcProcessInterfaceReplica replica is not connected!";
        }

    });
    socket->connectToServer(amnezia::getIpcProcessUrl(pid));
    socket->waitForConnected();

    auto proccessReplica = QSharedPointer<IpcProcessInterfaceReplica>(ptr);



//    replica->connectToNode(QUrl(amnezia::getIpcProcessUrl(pid)));
//    auto ptr = QSharedPointer<IpcProcessInterfaceReplica>(replica->acquire<IpcProcessInterfaceReplica>());
    connect(proccessReplica.data(), &IpcProcessInterfaceReplica::destroyed, proccessReplica.data(), [replicaNode](){
        replicaNode->deleteLater();
    });

    return proccessReplica;
}

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
//    m_ClientNode.connectToNode(QUrl(amnezia::getIpcServiceUrl()));
//    qDebug() << QUrl(amnezia::getIpcServiceUrl());


    m_localSocket.reset(new QLocalSocket(this));
    connect(m_localSocket.data(), &QLocalSocket::connected, &m_ClientNode, [this]() {
        m_ClientNode.addClientSideConnection(m_localSocket.data());

        m_ipcClient.reset(m_ClientNode.acquire<IpcInterfaceReplica>());
        m_ipcClient->waitForSource(1000);

        if (!m_ipcClient->isReplicaValid()) {
            qWarning() << "IpcClient replica is not connected!";
        }

    });
    m_localSocket->connectToServer(amnezia::getIpcServiceUrl());



//    connect(m_ipcClient.data(), &IpcInterfaceReplica::stateChanged, [&](QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState){

////        qDebug() << "state" << state;
////        for (int i = 0; i < 10; ++i) {
////            QRemoteObjectPendingReply<qint64> future = m_ipcClient->createPrivilegedProcess("", QStringList());

////            future.waitForFinished();
////            qDebug() << "QRemoteObjectPendingReply" << QDateTime::currentMSecsSinceEpoch() - future.returnValue();

////        }
//    });


}
