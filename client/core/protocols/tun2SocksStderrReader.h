#ifndef TUN2SOCKSSTDERRREADER_H
#define TUN2SOCKSSTDERRREADER_H

#include <QObject>
#include <QPointer>
#include <QTimer>

class IpcProcessInterfaceReplica;
class QRemoteObjectPendingCallWatcher;

class Tun2SocksStderrReader : public QObject
{
    Q_OBJECT

public:
    explicit Tun2SocksStderrReader(QObject *parent = nullptr);
    void setProcess(IpcProcessInterfaceReplica *process);

signals:
    void outputReady(const QByteArray &output);

private:
    void read();
    void finishRead(QRemoteObjectPendingCallWatcher *call, bool succeeded);
    void resetRead();

    QPointer<IpcProcessInterfaceReplica> m_process;
    QRemoteObjectPendingCallWatcher *m_pendingRead = nullptr;
    QTimer m_readTimeout;
    bool m_readAgain = false;
};

#endif
