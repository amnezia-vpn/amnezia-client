#include "ipcclient.h"
#include "ipc.h"
#include <QRemoteObjectNode>
#include <QtNetwork/qlocalsocket.h>

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    m_localSocket.setServerName(amnezia::getIpcServiceUrl());

    connect(&m_localSocket, &QLocalSocket::connected, this, [this]() {
        m_ClientNode.addClientSideConnection(&m_localSocket);
        m_ipcClient.reset(m_ClientNode.acquire<IpcInterfaceReplica>());
        m_Tun2SocksClient.reset(m_ClientNode.acquire<IpcProcessTun2SocksReplica>());
        m_isSocketConnected = true;
    });

    connect(&m_localSocket, &QLocalSocket::disconnected,  [this]() {
        m_ipcClient.clear();
        m_Tun2SocksClient.clear();
        m_isSocketConnected = false;
    });
}

IpcClient *IpcClient::Instance()
{
    static IpcClient instance;

    QMutexLocker locker(&instance.m_mutex);

    if (!instance.m_isSocketConnected) {
        instance.establishConnection();
    }

    return &instance;
}

QSharedPointer<IpcInterfaceReplica> IpcClient::Interface()
{
    auto rep = Instance()->m_ipcClient;
    if (!rep) {
        qCritical() << "IpcClient::Interface(): Replica is undefined";
        return nullptr;
    }
    if (!rep->waitForSource(1000)) {
        qCritical() << "IpcClient::Interface(): Failed to initialize replica";
        return nullptr;
    }
    if (!rep->isReplicaValid()) {
        qWarning() << "IpcClient::Interface(): Replica is invalid";
    }
    return rep;
}

QSharedPointer<IpcProcessTun2SocksReplica> IpcClient::InterfaceTun2Socks()
{
    auto rep = Instance()->m_Tun2SocksClient;
    if (!rep) {
        qCritical() << "IpcClient::InterfaceTun2Socks: Replica is undefined";
        return nullptr;
    }
    if (!rep->waitForSource(1000)) {
        qCritical() << "IpcClient::InterfaceTun2Socks: Failed to initialize replica";
        return nullptr;
    }
    if (!rep->isReplicaValid()) {
        qWarning() << "IpcClient::InterfaceTun2Socks(): Replica is invalid";
    }
    return rep;
}

bool IpcClient::establishConnection()
{
    m_localSocket.connectToServer();
    return m_localSocket.waitForConnected();
}

QSharedPointer<PrivilegedProcess> IpcClient::CreatePrivilegedProcess()
{
    auto rep = Interface();
    if (!rep) {
        qCritical() << "IpcClient::createPrivilegedProcess : IpcClient IpcClient replica is not valid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> futureResult = rep->createPrivilegedProcess();
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
        } else {
            pd->ipcProcess->waitForSource(1000);
            if (!pd->ipcProcess->isReplicaValid()) {
                qWarning() << "PrivilegedProcess replica is not connected!";
            }

            QObject::connect(pd->ipcProcess.data(), &PrivilegedProcess::destroyed, pd->ipcProcess.data(),
                             [pd]() { pd->replicaNode->deleteLater(); });
        }
    });
    pd->localSocket->connectToServer(amnezia::getIpcProcessUrl(pid));
    pd->localSocket->waitForConnected();

    auto processReplica = QSharedPointer<PrivilegedProcess>(pd->ipcProcess);
    return processReplica;
}
