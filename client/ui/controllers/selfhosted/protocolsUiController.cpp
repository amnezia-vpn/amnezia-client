#include "protocolsUiController.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/protocolEnum.h"

using namespace amnezia;
using namespace ProtocolUtils;

ProtocolsUiController::ProtocolsUiController(ProtocolsModel* protocolsModel, QObject *parent)
    : QObject(parent),
      m_protocolsModel(protocolsModel)
{
}

void ProtocolsUiController::updateProtocols(const QJsonObject &config)
{
    m_protocolsModel->updateModel(config);
}

QJsonObject ProtocolsUiController::getProtocolsConfig()
{
    return m_protocolsModel->getConfig();
}

void ProtocolsUiController::openServerSettings(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    
    switch (proto) {
    case Proto::OpenVpn:
        emit openOpenVpnServerSettings();
        break;
    case Proto::WireGuard:
        emit openWireGuardServerSettings();
        break;
    case Proto::Awg:
        emit openAwgServerSettings();
        break;
    case Proto::Xray:
        emit openXrayServerSettings();
        break;
    case Proto::Sftp:
        emit openSftpServerSettings();
        break;
    case Proto::Ikev2:
        emit openIkev2ServerSettings();
        break;
    case Proto::Socks5Proxy:
        emit openSocks5ProxyServerSettings();
        break;
    default:
        break;
    }
}

void ProtocolsUiController::openClientSettings(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    
    switch (proto) {
    case Proto::WireGuard:
        emit openWireGuardClientSettings();
        break;
    case Proto::Awg:
        emit openAwgClientSettings();
        break;
    default:
        break;
    }
}

int ProtocolsUiController::defaultPort(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultPort(proto);
}

int ProtocolsUiController::getPortForInstall(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::getPortForInstall(proto);
}

int ProtocolsUiController::defaultTransportProto(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return static_cast<int>(ProtocolUtils::defaultTransportProto(proto));
}

bool ProtocolsUiController::defaultPortChangeable(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultPortChangeable(proto);
}

bool ProtocolsUiController::defaultTransportProtoChangeable(int protocolIndex)
{
    Proto proto = static_cast<Proto>(protocolIndex);
    return ProtocolUtils::defaultTransportProtoChangeable(proto);
}

