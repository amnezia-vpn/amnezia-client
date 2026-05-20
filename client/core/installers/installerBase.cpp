#include "installerBase.h"

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/protocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"
#include "core/models/protocols/mtProxyProtocolConfig.h"
#include "core/models/protocols/telemtProtocolConfig.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"
#include "core/models/protocols/torProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

InstallerBase::InstallerBase(QObject *parent)
    : QObject(parent)
{
}

ContainerConfig InstallerBase::generateConfig(DockerContainer container, int port, TransportProto transportProto)
{
    return createBaseConfig(container, port, transportProto);
}

ErrorCode InstallerBase::extractConfigFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                   SshSession* sshSession, ContainerConfig &config)
{
    Q_UNUSED(container);
    Q_UNUSED(credentials);
    Q_UNUSED(sshSession);
    Q_UNUSED(config);
    return ErrorCode::NoError;
}

ContainerConfig InstallerBase::createBaseConfig(DockerContainer container, int port, TransportProto transportProto)
{
    ContainerConfig config;
    config.container = container;
    
    Proto protocol = ContainerUtils::defaultProtocol(container);
    QString portStr = QString::number(port);
    QString transportProtoStr = ProtocolUtils::transportProtoToString(transportProto, protocol);
    
    switch (protocol) {
        case Proto::Awg: {
            AwgProtocolConfig awgConfig;
            awgConfig.serverConfig.port = portStr;
            awgConfig.serverConfig.transportProto = transportProtoStr;
            config.protocolConfig = awgConfig;
            break;
        }
        case Proto::WireGuard: {
            WireGuardProtocolConfig wgConfig;
            wgConfig.serverConfig.port = portStr;
            wgConfig.serverConfig.transportProto = transportProtoStr;
            config.protocolConfig = wgConfig;
            break;
        }
        case Proto::OpenVpn: {
            OpenVpnProtocolConfig ovpnConfig;
            ovpnConfig.serverConfig.port = portStr;
            ovpnConfig.serverConfig.transportProto = transportProtoStr;
            config.protocolConfig = ovpnConfig;
            break;
        }
        case Proto::Xray:
        case Proto::SSXray: {
            XrayProtocolConfig xrayConfig;
            xrayConfig.serverConfig.port = portStr;
            xrayConfig.serverConfig.transportProto = transportProtoStr;
            config.protocolConfig = xrayConfig;
            break;
        }
        case Proto::Sftp: {
            SftpProtocolConfig sftpConfig;
            sftpConfig.port = portStr;
            config.protocolConfig = sftpConfig;
            break;
        }
        case Proto::Socks5Proxy: {
            Socks5ProxyProtocolConfig socks5Config;
            socks5Config.port = portStr;
            config.protocolConfig = socks5Config;
            break;
        }
        case Proto::MtProxy: {
            MtProxyProtocolConfig mtConfig;
            mtConfig.port = portStr;
            config.protocolConfig = mtConfig;
            break;
        }
        case Proto::Telemt: {
            TelemtProtocolConfig telemtConfig;
            telemtConfig.port = portStr;
            config.protocolConfig = telemtConfig;
            break;
        }
        case Proto::Ikev2: {
            Ikev2ProtocolConfig ikev2Config;
            config.protocolConfig = ikev2Config;
            break;
        }
        case Proto::TorWebSite: {
            TorProtocolConfig torConfig;
            config.protocolConfig = torConfig;
            break;
        }
        case Proto::Dns: {
            DnsProtocolConfig dnsConfig;
            config.protocolConfig = dnsConfig;
            break;
        }
        default: {
            break;
        }
    }
    
    return config;
}

