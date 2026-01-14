#ifndef PROTOCOLCONFIG_H
#define PROTOCOLCONFIG_H

#include <QJsonObject>
#include <variant>

#include "protocols/protocols_defs.h"
#include "containers/containers_defs.h"

#include "protocols/awgProtocolConfig.h"
#include "protocols/wireGuardProtocolConfig.h"
#include "protocols/openVpnProtocolConfig.h"
#include "protocols/xrayProtocolConfig.h"
#include "protocols/ssXrayProtocolConfig.h"
#include "protocols/sftpProtocolConfig.h"
#include "protocols/socks5ProxyProtocolConfig.h"

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
    Socks5ProxyProtocolConfig
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
    
    QString port(const ProtocolConfig& config);
    QString transportProto(const ProtocolConfig& config);
    
    bool hasClientConfig(const ProtocolConfig& config);
    QJsonObject getClientConfigJson(const ProtocolConfig& config);
    void setClientConfigJson(ProtocolConfig& config, const QJsonObject& clientJson);
    void clearClientConfig(ProtocolConfig& config);
    
    QJsonObject toJson(const ProtocolConfig& config, Proto protocolType);
    ProtocolConfig fromJson(const QJsonObject& json, Proto protocolType);
}

} // namespace amnezia

#endif // PROTOCOLCONFIG_H
