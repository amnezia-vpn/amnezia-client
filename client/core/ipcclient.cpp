#include "ipcclient.h"
#include <QRemoteObjectNode>

IpcClient *IpcClient::m_instance = nullptr;

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{

}

IpcClient::~IpcClient()
{
    if (m_localSocket) m_localSocket->close();
}

bool IpcClient::isSocketConnected() const
{
    return m_isSocketConnected;
}

IpcClient *IpcClient::Instance()
{
    return m_instance;
}

QSharedPointer<IpcInterfaceReplica> IpcClient::Interface()
{
    if (!Instance()) return nullptr;
    return Instance()->m_ipcClient;
}

QSharedPointer<IpcProcessTun2SocksReplica> IpcClient::InterfaceTun2Socks()
{
    if (!Instance()) return nullptr;
    return Instance()->m_Tun2SocksClient;
}

bool IpcClient::init(IpcClient *instance)
{
    m_instance = instance;

    Instance()->m_localSocket = new QLocalSocket(Instance());
    connect(Instance()->m_localSocket.data(), &QLocalSocket::connected, &Instance()->m_ClientNode, []() {
        Instance()->m_ClientNode.addClientSideConnection(Instance()->m_localSocket.data());

        Instance()->m_ipcClient.reset(Instance()->m_ClientNode.acquire<IpcInterfaceReplica>());
        Instance()->m_ipcClient->waitForSource(1000);

        if (!Instance()->m_ipcClient->isReplicaValid()) {
            qWarning() << "IpcClient replica is not connected!";
        }

        Instance()->m_Tun2SocksClient.reset(Instance()->m_ClientNode.acquire<IpcProcessTun2SocksReplica>());
        Instance()->m_Tun2SocksClient->waitForSource(1000);

        if (!Instance()->m_Tun2SocksClient->isReplicaValid()) {
            qWarning() << "IpcClient::m_Tun2SocksClient replica is not connected!";
        }
    });

    connect(Instance()->m_localSocket, &QLocalSocket::disconnected, [instance](){
        instance->m_isSocketConnected = false;
    });

    Instance()->m_localSocket->connectToServer(amnezia::getIpcServiceUrl());
    Instance()->m_localSocket->waitForConnected();

    if (!Instance()->m_ipcClient) {
        qDebug() << "IpcClient::init failed";
        return false;
    }

    qDebug() << "IpcClient::init succeed";

    return (Instance()->m_ipcClient->isReplicaValid() && Instance()->m_Tun2SocksClient->isReplicaValid());
}

QSharedPointer<PrivilegedProcess> IpcClient::CreatePrivilegedProcess()
{
    if (! Instance()->m_ipcClient || ! Instance()->m_ipcClient->isReplicaValid()) {
        qWarning() << "IpcClient::createPrivilegedProcess : IpcClient IpcClient replica is not valid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> futureResult = Instance()->m_ipcClient->createPrivilegedProcess();
    futureResult.waitForFinished(5000);

    int pid = futureResult.returnValue();

    auto pd = QSharedPointer<ProcessDescriptor>(new ProcessDescriptor());
    Instance()->m_processNodes.insert(pid, pd);

    pd->localSocket.reset(new QLocalSocket(pd->replicaNode.data()));

    connect(pd->localSocket.data(), &QLocalSocket::connected, pd->replicaNode.data(), [pd]() {
        pd->replicaNode->addClientSideConnection(pd->localSocket.data());

        IpcProcessInterfaceReplica *repl = pd->replicaNode->acquire<IpcProcessInterfaceReplica>();
        PrivilegedProcess *priv = static_cast<PrivilegedProcess *>(repl);
        pd->ipcProcess.reset(priv);
        if (!pd->ipcProcess) {
            qWarning() << "Acquire PrivilegedProcess failed";
        }
        else {
            pd->ipcProcess->waitForSource(1000);
            if (!pd->ipcProcess->isReplicaValid()) {
                qWarning() << "PrivilegedProcess replica is not connected!";
            }

            QObject::connect(pd->ipcProcess.data(), &PrivilegedProcess::destroyed, pd->ipcProcess.data(), [pd](){
                pd->replicaNode->deleteLater();
            });
        }

    });
    pd->localSocket->connectToServer(amnezia::getIpcProcessUrl(pid));
    pd->localSocket->waitForConnected();

    auto processReplica = QSharedPointer<PrivilegedProcess>(pd->ipcProcess);
    return processReplica;
}


