#ifndef CONTAINERCONFIG_H
#define CONTAINERCONFIG_H

#include <QJsonObject>
#include <QMap>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
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
    
    AwgProtocolConfig* getAwgProtocolConfig();
    const AwgProtocolConfig* getAwgProtocolConfig() const;
    
    WireGuardProtocolConfig* getWireGuardProtocolConfig();
    const WireGuardProtocolConfig* getWireGuardProtocolConfig() const;
    
    OpenVpnProtocolConfig* getOpenVpnProtocolConfig();
    const OpenVpnProtocolConfig* getOpenVpnProtocolConfig() const;
    
    XrayProtocolConfig* getXrayProtocolConfig();
    const XrayProtocolConfig* getXrayProtocolConfig() const;
    
    SftpProtocolConfig* getSftpProtocolConfig();
    const SftpProtocolConfig* getSftpProtocolConfig() const;
    
    Socks5ProxyProtocolConfig* getSocks5ProxyProtocolConfig();
    const Socks5ProxyProtocolConfig* getSocks5ProxyProtocolConfig() const;
    
    MtProxyProtocolConfig* getMtProxyProtocolConfig();
    const MtProxyProtocolConfig* getMtProxyProtocolConfig() const;

    Ikev2ProtocolConfig* getIkev2ProtocolConfig();
    const Ikev2ProtocolConfig* getIkev2ProtocolConfig() const;
    
    TorProtocolConfig* getTorProtocolConfig();
    const TorProtocolConfig* getTorProtocolConfig() const;
    
    DnsProtocolConfig* getDnsProtocolConfig();
    const DnsProtocolConfig* getDnsProtocolConfig() const;
};

} // namespace amnezia

#endif // CONTAINERCONFIG_H

