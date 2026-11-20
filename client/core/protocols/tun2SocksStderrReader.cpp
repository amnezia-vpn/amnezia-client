#include "tun2SocksStderrReader.h"

#include "rep_ipc_process_interface_replica.h"

#include <QRemoteObjectPendingCallWatcher>

#include <utility>

Tun2SocksStderrReader::Tun2SocksStderrReader(QObject *parent) : QObject(parent), m_readTimeout(this)
{
    m_readTimeout.setSingleShot(true);
    m_readTimeout.setInterval(30000);
    connect(&m_readTimeout, &QTimer::timeout, this, [this]() { finishRead(m_pendingRead, false); });
}

void Tun2SocksStderrReader::setProcess(IpcProcessInterfaceReplica *process)
{
    if (m_process) {
        disconnect(m_process, nullptr, this, nullptr);
    }
    resetRead();
    m_readAgain = false;
    m_process = process;

    if (process) {
        connect(
                process, &IpcProcessInterfaceReplica::readyReadStandardError, this,
                [this, process = m_process]() {
                    if (process && process == m_process) {
                        requestRead();
                    }
                },
                Qt::QueuedConnection);
    }
}

void Tun2SocksStderrReader::requestRead()
{
    if (!m_process) {
        return;
    }
    if (m_pendingRead) {
        m_readAgain = true;
        return;
    }

    auto *call = new QRemoteObjectPendingCallWatcher(m_process->readAllStandardError(), this);
    m_pendingRead = call;
    connect(call, &QRemoteObjectPendingCallWatcher::finished, this, [this](QRemoteObjectPendingCallWatcher *reply) {
        finishRead(reply, reply->error() == QRemoteObjectPendingCall::NoError);
    });
    m_readTimeout.start();
}

void Tun2SocksStderrReader::finishRead(QRemoteObjectPendingCallWatcher *call, bool succeeded)
{
    if (!call || call != m_pendingRead) {
        return;
    }

    m_readTimeout.stop();
    if (succeeded && m_process) {
        const QPointer<Tun2SocksStderrReader> reader(this);
        const QPointer<QRemoteObjectPendingCallWatcher> pendingRead(call);
        emit outputReady(call->returnValue().toByteArray());
        if (!reader || !pendingRead || pendingRead != m_pendingRead) {
            return;
        }
    } else if (!succeeded) {
        qWarning() << "Failed to read output from tun2socks";
    }

    resetRead();
    if (std::exchange(m_readAgain, false)) {
        requestRead();
    }
}

void Tun2SocksStderrReader::resetRead()
{
    m_readTimeout.stop();
    if (auto *call = std::exchange(m_pendingRead, nullptr)) {
        call->deleteLater();
    }
}
