#ifndef AWGPROTOCOL_H
#define AWGPROTOCOL_H

#include <QObject>

#include "wireguardprotocol.h"

class Awg : public WireguardProtocol
{
    Q_OBJECT

public:
    explicit Awg(const QJsonObject &configuration, QObject *parent = nullptr);
    virtual ~Awg() override;
};

#endif // AWGPROTOCOL_H
