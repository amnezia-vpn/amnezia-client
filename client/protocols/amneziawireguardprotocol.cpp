#include "amneziawireguardprotocol.h"

Awg::Awg(const QJsonObject &configuration, QObject *parent)
    : WireguardProtocol(configuration, parent)
{
}

Awg::~Awg()
{
}
