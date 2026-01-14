#ifndef CONTAINERCONFIG_H
#define CONTAINERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "containers/containers_defs.h"
#include "protocols/protocols_defs.h"
#include "core/models/protocolConfig.h"

namespace amnezia
{

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;

struct ContainerConfig {
    DockerContainer container;
    ProtocolConfig protocolConfig;
    
    Proto getProtocolType() const;
    QJsonObject toJson() const;
    static ContainerConfig fromJson(const QJsonObject& json);
    
    template<typename Visitor>
    auto visitProtocol(Visitor&& visitor)
    {
        return std::visit(std::forward<Visitor>(visitor), protocolConfig);
    }
    
    template<typename Visitor>
    auto visitProtocol(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), protocolConfig);
    }
};

} // namespace amnezia

#endif // CONTAINERCONFIG_H

