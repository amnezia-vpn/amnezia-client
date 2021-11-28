#ifndef ANDROID_VPNPROTOCOL_H
#define ANDROID_VPNPROTOCOL_H

#include <QAndroidBinder>
#include <QAndroidServiceConnection>

#include "vpnprotocol.h"
#include "protocols/protocols_defs.h"

using namespace amnezia;



class AndroidVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit AndroidVpnProtocol(Protocol protocol, const QJsonObject& configuration, QObject* parent = nullptr);
    virtual ~AndroidVpnProtocol() override = default;

    ErrorCode start() override;
    void stop() override;

signals:


protected slots:

protected:


private:
    Protocol m_protocol;

};

#endif // ANDROID_VPNPROTOCOL_H
