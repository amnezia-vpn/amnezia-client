#include "ipcclient.h"
#include <QRemoteObjectNode>

IpcClient &IpcClient::Instance()
{
    static IpcClient s;
    return s;
}

QSharedPointer<IpcProcessInterfaceReplica> IpcClient::createPrivilegedProcess()
{
    if (! Instance().m_ipcClient->isReplicaValid()) return nullptr;

    QRemoteObjectPendingReply<int> futureResult = Instance().m_ipcClient->createPrivilegedProcess();
    futureResult.waitForFinished(1000);

    int pid = futureResult.returnValue();
    QSharedPointer<QRemoteObjectNode> replica(new QRemoteObjectNode);
    //Instance().m_processNodes.insert(pid, replica);

    replica->connectToNode(QUrl(amnezia::getIpcProcessUrl(pid)));
    auto ptr = QSharedPointer<IpcProcessInterfaceReplica>(replica->acquire<IpcProcessInterfaceReplica>());
    connect(ptr.data(), &IpcProcessInterfaceReplica::destroyed, replica.data(), [replica](){
        replica->deleteLater();
    });

    return ptr;
}

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    m_ClientNode.connectToNode(QUrl(QStringLiteral(IPC_SERVICE_URL)));
    m_ipcClient.reset(m_ClientNode.acquire<IpcInterfaceReplica>());
    m_ipcClient->waitForSource(1000);



//    connect(m_ipcClient.data(), &IpcInterfaceReplica::stateChanged, [&](QRemoteObjectReplica::State state, QRemoteObjectReplica::State oldState){

////        qDebug() << "state" << state;
////        for (int i = 0; i < 10; ++i) {
////            QRemoteObjectPendingReply<qint64> future = m_ipcClient->createPrivilegedProcess("", QStringList());

////            future.waitForFinished();
////            qDebug() << "QRemoteObjectPendingReply" << QDateTime::currentMSecsSinceEpoch() - future.returnValue();

////        }
//    });


}
