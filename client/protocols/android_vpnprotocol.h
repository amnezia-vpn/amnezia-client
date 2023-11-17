#ifndef ANDROID_VPNPROTOCOL_H
#define ANDROID_VPNPROTOCOL_H

#include "vpnprotocol.h"

using namespace amnezia;

class AndroidVpnProtocol : public VpnProtocol
{
    Q_OBJECT

public:
    explicit AndroidVpnProtocol(Proto protocol, const QJsonObject& configuration, QObject* parent = nullptr);
    ~AndroidVpnProtocol() override = default;

    ErrorCode start() override;
    void stop() override;

public slots:
    void connectionDataUpdated(quint64 rxBytes, quint64 txBytes);

};

#endif // ANDROID_VPNPROTOCOL_H
