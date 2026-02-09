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
    
    obj[config_key::container] = ContainerUtils::containerToString(container);
    
    Proto protoType = getProtocolType();
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    QJsonObject protoJson = ProtocolConfigUtils::toJson(protocolConfig, protoType);
    
    obj[protoName] = protoJson;
    
    return obj;
}

ContainerConfig ContainerConfig::fromJson(const QJsonObject& json)
{
    ContainerConfig config;
    
    QString containerStr = json.value(config_key::container).toString();
    config.container = ContainerUtils::containerFromString(containerStr);
    
    Proto protoType = ContainerUtils::defaultProtocol(config.container);
    QString protoName = ProtocolUtils::protoToString(protoType);
    
    QJsonObject protoJson = json.value(protoName).toObject();
    
    config.protocolConfig = ProtocolConfigUtils::fromJson(protoJson, protoType);
    
    return config;
}

AwgProtocolConfig* ContainerConfig::getAwgProtocolConfig()
{
    return std::get_if<AwgProtocolConfig>(&protocolConfig);
}

const AwgProtocolConfig* ContainerConfig::getAwgProtocolConfig() const
{
    return std::get_if<AwgProtocolConfig>(&protocolConfig);
}

WireGuardProtocolConfig* ContainerConfig::getWireGuardProtocolConfig()
{
    return std::get_if<WireGuardProtocolConfig>(&protocolConfig);
}

const WireGuardProtocolConfig* ContainerConfig::getWireGuardProtocolConfig() const
{
    return std::get_if<WireGuardProtocolConfig>(&protocolConfig);
}

OpenVpnProtocolConfig* ContainerConfig::getOpenVpnProtocolConfig()
{
    return std::get_if<OpenVpnProtocolConfig>(&protocolConfig);
}

const OpenVpnProtocolConfig* ContainerConfig::getOpenVpnProtocolConfig() const
{
    return std::get_if<OpenVpnProtocolConfig>(&protocolConfig);
}

XrayProtocolConfig* ContainerConfig::getXrayProtocolConfig()
{
    return std::get_if<XrayProtocolConfig>(&protocolConfig);
}

const XrayProtocolConfig* ContainerConfig::getXrayProtocolConfig() const
{
    return std::get_if<XrayProtocolConfig>(&protocolConfig);
}

SSXrayProtocolConfig* ContainerConfig::getSSXrayProtocolConfig()
{
    return std::get_if<SSXrayProtocolConfig>(&protocolConfig);
}

const SSXrayProtocolConfig* ContainerConfig::getSSXrayProtocolConfig() const
{
    return std::get_if<SSXrayProtocolConfig>(&protocolConfig);
}

SftpProtocolConfig* ContainerConfig::getSftpProtocolConfig()
{
    return std::get_if<SftpProtocolConfig>(&protocolConfig);
}

const SftpProtocolConfig* ContainerConfig::getSftpProtocolConfig() const
{
    return std::get_if<SftpProtocolConfig>(&protocolConfig);
}

Socks5ProxyProtocolConfig* ContainerConfig::getSocks5ProxyProtocolConfig()
{
    return std::get_if<Socks5ProxyProtocolConfig>(&protocolConfig);
}

const Socks5ProxyProtocolConfig* ContainerConfig::getSocks5ProxyProtocolConfig() const
{
    return std::get_if<Socks5ProxyProtocolConfig>(&protocolConfig);
}

Ikev2ProtocolConfig* ContainerConfig::getIkev2ProtocolConfig()
{
    return std::get_if<Ikev2ProtocolConfig>(&protocolConfig);
}

const Ikev2ProtocolConfig* ContainerConfig::getIkev2ProtocolConfig() const
{
    return std::get_if<Ikev2ProtocolConfig>(&protocolConfig);
}

} // namespace amnezia

