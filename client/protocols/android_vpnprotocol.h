#ifndef ANDROID_VPNPROTOCOL_H
#define ANDROID_VPNPROTOCOL_H

#include "vpnprotocol.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;

class AndroidVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit AndroidVpnProtocol(Proto protocol, const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~AndroidVpnProtocol() override = default;

    ErrorCode start() override;
    void stop() override;

signals:


public slots:
    void connectionDataUpdated(QString totalRx, QString totalTx, QString endpoint, QString deviceIPv4);

protected slots:

protected:


private:
    Proto m_protocol;
};

#endif // ANDROID_VPNPROTOCOL_H
