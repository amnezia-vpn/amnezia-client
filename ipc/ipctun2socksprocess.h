#ifndef IPCTUN2SOCKSPROCESS_H
#define IPCTUN2SOCKSPROCESS_H

#include <QObject>

#ifndef Q_OS_IOS
#include "rep_ipc_process_tun2socks_source.h"

namespace Vpn
{
Q_NAMESPACE
    enum ConnectionState {
        Unknown,
        Disconnected,
        Preparing,
        Connecting,
        Connected,
        Disconnecting,
        Reconnecting,
        Error
    };
Q_ENUM_NS(ConnectionState)
}


class IpcProcessTun2Socks : public IpcProcessTun2SocksSource
{
    Q_OBJECT
public:
    explicit IpcProcessTun2Socks(QObject *parent = nullptr);
    virtual ~IpcProcessTun2Socks();

    void start() override;
    void stop() override;

signals:

private:
    QSharedPointer<QProcess> m_t2sProcess;
};

#else
class IpcProcessTun2Socks : public QObject
{
    Q_OBJECT

public:
    explicit IpcProcessTun2Socks(QObject *parent = nullptr);
};
#endif

#endif // IPCTUN2SOCKSPROCESS_H
