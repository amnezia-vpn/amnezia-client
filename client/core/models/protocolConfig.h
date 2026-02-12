#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <QJsonObject>
#include <variant>

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/ssXrayProtocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/models/protocols/torProtocolConfig.h"
#include "core/models/protocols/dnsProtocolConfig.h"

namespace amnezia
{

using Proto = ProtocolEnumNS::Proto;

using ProtocolConfig = std::variant<
    AwgProtocolConfig,
    WireGuardProtocolConfig,
    OpenVpnProtocolConfig,
    XrayProtocolConfig,
    SSXrayProtocolConfig,
    SftpProtocolConfig,
    Socks5ProxyProtocolConfig,
    Ikev2ProtocolConfig,
    TorProtocolConfig,
    DnsProtocolConfig
>;

namespace ProtocolConfigUtils {
    Proto getProtocolType(const ProtocolConfig& config);
    
    AwgProtocolConfig& asAwg(ProtocolConfig& config);
    const AwgProtocolConfig& asAwg(const ProtocolConfig& config);
    
    WireGuardProtocolConfig& asWireGuard(ProtocolConfig& config);
    const WireGuardProtocolConfig& asWireGuard(const ProtocolConfig& config);
    
    OpenVpnProtocolConfig& asOpenVpn(ProtocolConfig& config);
    const OpenVpnProtocolConfig& asOpenVpn(const ProtocolConfig& config);
    
    XrayProtocolConfig& asXray(ProtocolConfig& config);
    const XrayProtocolConfig& asXray(const ProtocolConfig& config);
    
    SSXrayProtocolConfig& asSSXray(ProtocolConfig& config);
    const SSXrayProtocolConfig& asSSXray(const ProtocolConfig& config);
    
    SftpProtocolConfig& asSftp(ProtocolConfig& config);
    const SftpProtocolConfig& asSftp(const ProtocolConfig& config);
    
    Socks5ProxyProtocolConfig& asSocks5Proxy(ProtocolConfig& config);
    const Socks5ProxyProtocolConfig& asSocks5Proxy(const ProtocolConfig& config);
    
    Ikev2ProtocolConfig& asIkev2(ProtocolConfig& config);
    const Ikev2ProtocolConfig& asIkev2(const ProtocolConfig& config);
    
    TorProtocolConfig& asTor(ProtocolConfig& config);
    const TorProtocolConfig& asTor(const ProtocolConfig& config);
    
    DnsProtocolConfig& asDns(ProtocolConfig& config);
    const DnsProtocolConfig& asDns(const ProtocolConfig& config);
    
    QString port(const ProtocolConfig& config);
    QString transportProto(const ProtocolConfig& config);
    
    QString portWithDefault(const ProtocolConfig& config, Proto protocol);
    QString transportProtoWithDefault(const ProtocolConfig& config, Proto protocol);
    
    bool hasClientConfig(const ProtocolConfig& config);
    QString clientId(const ProtocolConfig& config);
    QJsonObject getClientConfigJson(const ProtocolConfig& config);
    void setClientConfigJson(ProtocolConfig& config, const QJsonObject& clientJson);
    void clearClientConfig(ProtocolConfig& config);
    
    QString nativeConfig(const ProtocolConfig& config);
    
    bool isThirdPartyConfig(const ProtocolConfig& config);
    
    QJsonObject toJson(const ProtocolConfig& config, Proto protocolType);
    ProtocolConfig fromJson(const QJsonObject& json, Proto protocolType);
}

} // namespace amnezia

#endif // PROTOCOLCONFIG_H
