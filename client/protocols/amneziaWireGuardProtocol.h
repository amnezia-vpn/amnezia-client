#ifndef AMNEZIAWIREGUARDPROTOCOL_H
#define AMNEZIAWIREGUARDPROTOCOL_H

#include <QObject>

#include "wireguardprotocol.h"

class AmneziaWireGuardProtocol : public WireguardProtocol
{
    Q_OBJECT

public:
    explicit AmneziaWireGuardProtocol(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~AmneziaWireGuardProtocol() override;
};

#endif // AMNEZIAWIREGUARDPROTOCOL_H
