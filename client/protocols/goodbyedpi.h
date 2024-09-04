#ifndef GOODBYEDPI_H
#define GOODBYEDPI_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>

#include "vpnprotocol.h"
#include "core/ipcclient.h"

class GoodbyeDPIProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit GoodbyeDPIProtocol(const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~GoodbyeDPIProtocol() override;

    ErrorCode start() override;
    void stop() override;

private:
    QSharedPointer<PrivilegedProcess> m_goodbyeDPIProcess;
};

#endif // GOODBYEDPI_H
