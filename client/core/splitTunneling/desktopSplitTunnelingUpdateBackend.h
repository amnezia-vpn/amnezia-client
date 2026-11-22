#ifndef DESKTOPSPLITTUNNELINGUPDATEBACKEND_H
#define DESKTOPSPLITTUNNELINGUPDATEBACKEND_H

#include "splitTunnelingUpdateBackend.h"

class DesktopSplitTunnelingUpdateBackend : public SplitTunnelingUpdateBackend
{
public:
    using SplitTunnelingUpdateBackend::SplitTunnelingUpdateBackend;
    void apply(quint64 requestId, const QJsonObject &configuration) override;
};

#endif
