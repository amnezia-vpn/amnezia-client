#include "containerConfig.h"

#include <QJsonDocument>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

namespace amnezia
{

using namespace ContainerEnumNS;
using namespace ProtocolEnumNS;
using namespace ProtocolUtils;

Proto ContainerConfig::getProtocolType() const
{
    return ContainerUtils::defaultProtocol(container);
}

QJsonObject ContainerConfig::toJson() const
{
    QJsonObject obj;
    
    obj[configKey::container] = ContainerUtils::containerToString(container);
    
    Proto protoType = getProtocolType();
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    obj[protoName] = protocolConfig.toJson();
    
    return obj;
}

ContainerConfig ContainerConfig::fromJson(const QJsonObject& json)
{
    ContainerConfig config;
    
    QString containerStr = json.value(configKey::container).toString();
    config.container = ContainerUtils::containerFromString(containerStr);
    
    Proto protoType = ContainerUtils::defaultProtocol(config.container);
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    QJsonObject protoJson = json.value(protoName).toObject();
    
    config.protocolConfig = ProtocolConfig::fromJson(protoJson, protoType);
    
    return config;
}

AwgProtocolConfig* ContainerConfig::getAwgProtocolConfig()
{
    return protocolConfig.as<AwgProtocolConfig>();
}

const AwgProtocolConfig* ContainerConfig::getAwgProtocolConfig() const
{
    return protocolConfig.as<AwgProtocolConfig>();
}

WireGuardProtocolConfig* ContainerConfig::getWireGuardProtocolConfig()
{
    return protocolConfig.as<WireGuardProtocolConfig>();
}

const WireGuardProtocolConfig* ContainerConfig::getWireGuardProtocolConfig() const
{
    return protocolConfig.as<WireGuardProtocolConfig>();
}

OpenVpnProtocolConfig* ContainerConfig::getOpenVpnProtocolConfig()
{
    return protocolConfig.as<OpenVpnProtocolConfig>();
}

const OpenVpnProtocolConfig* ContainerConfig::getOpenVpnProtocolConfig() const
{
    return protocolConfig.as<OpenVpnProtocolConfig>();
}

XrayProtocolConfig* ContainerConfig::getXrayProtocolConfig()
{
    return protocolConfig.as<XrayProtocolConfig>();
}

const XrayProtocolConfig* ContainerConfig::getXrayProtocolConfig() const
{
    return protocolConfig.as<XrayProtocolConfig>();
}

SftpProtocolConfig* ContainerConfig::getSftpProtocolConfig()
{
    return protocolConfig.as<SftpProtocolConfig>();
}

const SftpProtocolConfig* ContainerConfig::getSftpProtocolConfig() const
{
    return protocolConfig.as<SftpProtocolConfig>();
}

Socks5ProxyProtocolConfig* ContainerConfig::getSocks5ProxyProtocolConfig()
{
    return protocolConfig.as<Socks5ProxyProtocolConfig>();
}

const Socks5ProxyProtocolConfig* ContainerConfig::getSocks5ProxyProtocolConfig() const
{
    return protocolConfig.as<Socks5ProxyProtocolConfig>();
}

MtProxyProtocolConfig* ContainerConfig::getMtProxyProtocolConfig()
{
    return protocolConfig.as<MtProxyProtocolConfig>();
}

const MtProxyProtocolConfig* ContainerConfig::getMtProxyProtocolConfig() const
{
    return protocolConfig.as<MtProxyProtocolConfig>();
}

TelemtProtocolConfig* ContainerConfig::getTelemtProtocolConfig()
{
    return protocolConfig.as<TelemtProtocolConfig>();
}

const TelemtProtocolConfig* ContainerConfig::getTelemtProtocolConfig() const
{
    return protocolConfig.as<TelemtProtocolConfig>();
}

Ikev2ProtocolConfig* ContainerConfig::getIkev2ProtocolConfig()
{
    return protocolConfig.as<Ikev2ProtocolConfig>();
}

const Ikev2ProtocolConfig* ContainerConfig::getIkev2ProtocolConfig() const
{
    return protocolConfig.as<Ikev2ProtocolConfig>();
}

TorProtocolConfig* ContainerConfig::getTorProtocolConfig()
{
    return protocolConfig.as<TorProtocolConfig>();
}

const TorProtocolConfig* ContainerConfig::getTorProtocolConfig() const
{
    return protocolConfig.as<TorProtocolConfig>();
}

DnsProtocolConfig* ContainerConfig::getDnsProtocolConfig()
{
    return protocolConfig.as<DnsProtocolConfig>();
}

const DnsProtocolConfig* ContainerConfig::getDnsProtocolConfig() const
{
    return protocolConfig.as<DnsProtocolConfig>();
}

} // namespace amnezia

