#ifndef APPLESPLITTUNNELINGUPDATEBACKEND_H
#define APPLESPLITTUNNELINGUPDATEBACKEND_H

#include "splitTunnelingUpdateBackend.h"

class AppleSplitTunnelingUpdateBackend : public SplitTunnelingUpdateBackend
{
public:
    using SplitTunnelingUpdateBackend::SplitTunnelingUpdateBackend;
    bool supportsConfiguration(const QJsonObject &configuration) const override;
    void apply(quint64 requestId, const QJsonObject &configuration) override;
};

#endif
