#include "ipcclient.h"
#include "ipc.h"
#include <QRemoteObjectNode>
#include <QtNetwork/qlocalsocket.h>

namespace
{
    thread_local IpcClient ipcClient;
}

IpcClient::IpcClient(QObject *parent) : QObject(parent)
{
    connect(&m_localSocket, &QLocalSocket::connected, this, [this]() {
        m_ClientNode.reset(new QRemoteObjectNode);
        m_ClientNode->addClientSideConnection(&m_localSocket);
        m_ipcClient.reset(m_ClientNode->acquire<IpcInterfaceReplica>());
        m_Tun2SocksClient.reset(m_ClientNode->acquire<IpcProcessTun2SocksReplica>());
        m_isSocketConnected = true;
    });

    connect(&m_localSocket, &QLocalSocket::disconnected, this, [this]() {
        m_ClientNode.clear();
        m_ipcClient.clear();
        m_Tun2SocksClient.clear();
        m_isSocketConnected = false;
    });
}

IpcClient *IpcClient::Instance()
{
    if (!ipcClient.m_isSocketConnected) {
        ipcClient.establishConnection();
    }

    return &ipcClient;
}

QSharedPointer<IpcInterfaceReplica> IpcClient::Interface()
{
    QSharedPointer<IpcInterfaceReplica> rep = Instance()->m_ipcClient;
    if (rep.isNull()) {
        qCritical() << "IpcClient::Interface(): Failed to acquire replica";
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
    QSharedPointer<IpcProcessTun2SocksReplica> rep = Instance()->m_Tun2SocksClient;
    if (rep.isNull()) {
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
    m_localSocket.connectToServer(amnezia::getIpcServiceUrl());
    return m_localSocket.waitForConnected();
}

QSharedPointer<PrivilegedProcess> IpcClient::CreatePrivilegedProcess()
{
    QSharedPointer<IpcInterfaceReplica> rep = Interface();
    if (!rep) {
        qCritical() << "IpcClient::createPrivilegedProcess: Replica is invalid";
        return nullptr;
    }

    QRemoteObjectPendingReply<int> pidReply = rep->createPrivilegedProcess();
    if (!pidReply.waitForFinished(5000)){
        qCritical() << "IpcClient::createPrivilegedProcess: Failed to execute RO createPrivilegedProcess call";
        return nullptr;
    }

    int pid = pidReply.returnValue();
    QSharedPointer<ProcessDescriptor> pd(new ProcessDescriptor());

    pd->localSocket.reset(new QLocalSocket(pd->replicaNode.data()));

    connect(pd->localSocket.data(), &QLocalSocket::connected, pd->replicaNode.data(), [pd]() {
        pd->replicaNode->addClientSideConnection(pd->localSocket.data());

        IpcProcessInterfaceReplica *repl = pd->replicaNode->acquire<IpcProcessInterfaceReplica>();
        // TODO: rework the unsafe cast below
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
    if (!pd->localSocket->waitForConnected()) {
        qCritical() << "IpcClient::createPrivilegedProcess: Failed to connect to process' socket";
        return nullptr;
    }

    auto processReplica = QSharedPointer<PrivilegedProcess>(pd->ipcProcess);
    return processReplica;
}
