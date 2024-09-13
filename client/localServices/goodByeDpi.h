#ifndef GOODBYEDPI_H
#define GOODBYEDPI_H

#include <QObject>

#include "core/defs.h"
#include "core/privileged_process.h"
#include "protocols/vpnprotocol.h"

class GoodByeDpi : public QObject
{
    Q_OBJECT
public:
    explicit GoodByeDpi(QObject *parent = nullptr);

    amnezia::ErrorCode start(const QString &blackListFile, const int modset);
    void stop();

private:
    QSharedPointer<PrivilegedProcess> m_goodbyeDPIProcess;
signals:
    void serviceStateChanged(Vpn::ConnectionState state);
};

#endif // GOODBYEDPI_H
