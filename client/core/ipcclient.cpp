#include "ipcclient.h"
#include <QRemoteObjectNode>

IpcClient &IpcClient::Instance()
{
    static IpcClient s;
    return s;
}

bool IpcClient::init()
{
    Instance().m_localSocket->waitForConnected();

    if (!Instance().m_ipcClient) {
        qDebug() << "IpcClient::init failed";
        return false;
    }
    return Instance().m_ipcClient->isReplicaValid();
}

QSharedPointer<IpcProcessInterfaceReplica> IpcClient::CreatePrivilegedProcess()
{
    if (! Instance().m_ipcClient || ! Instance().m_ipcClient->isReplicaValid()) {
        qWarning() << "IpcClient::createPrivilegedProcess : IpcClient IpcClient replica is not valid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> futureResult = Instance().m_ipcClient->createPrivilegedProcess();
    futureResult.waitForFinished(1000);

    int pid = futureResult.returnValue();

    auto pd = QSharedPointer<ProcessDescriptor>(new ProcessDescriptor());
    Instance().m_processNodes.insert(pid, pd);

    pd->localSocket.reset(new QLocalSocket(pd->replicaNode.data()));

    connect(pd->localSocket.data(), &QLocalSocket::connected, pd->replicaNode.data(), [pd]() {
        pd->replicaNode->addClientSideConnection(pd->localSocket.data());

        pd->ipcProcess.reset(pd->replicaNode->acquire<IpcProcessInterfaceReplica>());
        if (!pd->ipcProcess) {
            qWarning() << "Acquire IpcProcessInterfaceReplica failed";
        }
        else {
            pd->ipcProcess->waitForSource(1000);
            if (!pd->ipcProcess->isReplicaValid()) {
                qWarning() << "IpcProcessInterfaceReplica replica is not connected!";
            }

            connect(pd->ipcProcess.data(), &IpcProcessInterfaceReplica::destroyed, pd->ipcProcess.data(), [pd](){
                pd->replicaNode->deleteLater();
            });
        }

    });
    pd->localSocket->connectToServer(amnezia::getIpcProcessUrl(pid));
    pd->localSocket->waitForConnected();

    auto proccessReplica = QSharedPointer<IpcProcessInterfaceReplica>(pd->ipcProcess);
    return proccessReplica;
}

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
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
}
