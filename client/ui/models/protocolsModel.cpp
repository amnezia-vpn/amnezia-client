#include "protocolsModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/openVpnProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

using namespace ProtocolUtils;

ProtocolsModel::ProtocolsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_containerConfig.container != DockerContainer::None ? 1 : 0;
}

QHash<int, QByteArray> ProtocolsModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[ProtocolNameRole] = "protocolName";
    roles[ServerProtocolPageRole] = "serverProtocolPage";
    roles[ClientProtocolPageRole] = "clientProtocolPage";
    roles[ProtocolIndexRole] = "protocolIndex";
    roles[ProtocolStringRole] = "protocolString";
    roles[RawConfigRole] = "rawConfig";
    roles[IsClientProtocolExistsRole] = "isClientProtocolExists";
    roles[IsWireGuardRole] = "isWireGuard";
    roles[IsAwgRole] = "isAwg";
    roles[IsOpenVpnRole] = "isOpenVpn";
    roles[IsXrayRole] = "isXray";
    roles[IsSftpRole] = "isSftp";
    roles[IsIpsecRole] = "isIpsec";
    roles[IsSocks5ProxyRole] = "isSocks5Proxy";

    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    Proto proto = getProtocolType();
    
    switch (role) {
    case ProtocolNameRole: {
        return ProtocolUtils::protocolHumanNames().value(proto);
    }
    case ServerProtocolPageRole:
        return static_cast<int>(serverProtocolPage(proto));
    case ClientProtocolPageRole:
        return static_cast<int>(clientProtocolPage(proto));
    case ProtocolIndexRole: return static_cast<int>(proto);
    case ProtocolStringRole: return ProtocolUtils::protoToString(proto);
    case IsWireGuardRole: return proto == Proto::WireGuard;
    case IsAwgRole: return proto == Proto::Awg;
    case IsOpenVpnRole: return proto == Proto::OpenVpn;
    case IsXrayRole: return proto == Proto::Xray;
    case IsSftpRole: return proto == Proto::Sftp;
    case IsIpsecRole: return proto == Proto::Ikev2;
    case IsSocks5ProxyRole: return proto == Proto::Socks5Proxy;
    case RawConfigRole:
        return getRawConfig();
    case IsClientProtocolExistsRole:
        return isClientProtocolExists();
    }

    return QVariant();
}

void ProtocolsModel::updateModel(const amnezia::ContainerConfig &containerConfig)
{
    beginResetModel();
    m_containerConfig = containerConfig;
    endResetModel();
}

Proto ProtocolsModel::getProtocolType() const
{
    return m_containerConfig.getProtocolType();
}

QString ProtocolsModel::getRawConfig() const
{
    QString configString;
    
    if (auto* awgConfig = std::get_if<amnezia::AwgProtocolConfig>(&m_containerConfig.protocolConfig)) {
        if (awgConfig->clientConfig.has_value() && !awgConfig->clientConfig->nativeConfig.isEmpty()) {
            configString = awgConfig->clientConfig->nativeConfig;
        }
    } else if (auto* wgConfig = std::get_if<amnezia::WireGuardProtocolConfig>(&m_containerConfig.protocolConfig)) {
        if (wgConfig->clientConfig.has_value() && !wgConfig->clientConfig->nativeConfig.isEmpty()) {
            configString = wgConfig->clientConfig->nativeConfig;
        }
    } else if (auto* openVpnConfig = std::get_if<amnezia::OpenVpnProtocolConfig>(&m_containerConfig.protocolConfig)) {
        if (openVpnConfig->clientConfig.has_value() && !openVpnConfig->clientConfig->nativeConfig.isEmpty()) {
            configString = openVpnConfig->clientConfig->nativeConfig;
        }
    } else if (auto* xrayConfig = std::get_if<amnezia::XrayProtocolConfig>(&m_containerConfig.protocolConfig)) {
        if (xrayConfig->clientConfig.has_value() && !xrayConfig->clientConfig->nativeConfig.isEmpty()) {
            configString = xrayConfig->clientConfig->nativeConfig;
        }
    }
    
    QStringList lines = configString.replace("\r", "").split("\n");
    QString rawConfig;
    for (const QString &l : lines) {
        rawConfig.append(l + "\n");
    }
    return rawConfig;
}

bool ProtocolsModel::isClientProtocolExists() const
{
    if (auto* awgConfig = std::get_if<amnezia::AwgProtocolConfig>(&m_containerConfig.protocolConfig)) {
        return awgConfig->clientConfig.has_value() && !awgConfig->clientConfig->nativeConfig.isEmpty();
    } else if (auto* wgConfig = std::get_if<amnezia::WireGuardProtocolConfig>(&m_containerConfig.protocolConfig)) {
        return wgConfig->clientConfig.has_value() && !wgConfig->clientConfig->nativeConfig.isEmpty();
    } else if (auto* openVpnConfig = std::get_if<amnezia::OpenVpnProtocolConfig>(&m_containerConfig.protocolConfig)) {
        return openVpnConfig->clientConfig.has_value() && !openVpnConfig->clientConfig->nativeConfig.isEmpty();
    } else if (auto* xrayConfig = std::get_if<amnezia::XrayProtocolConfig>(&m_containerConfig.protocolConfig)) {
        return xrayConfig->clientConfig.has_value() && !xrayConfig->clientConfig->nativeConfig.isEmpty();
    }
    
    return false;
}

PageLoader::PageEnum ProtocolsModel::serverProtocolPage(Proto protocol) const
{
    switch (protocol) {
    case Proto::OpenVpn: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::WireGuard: return PageLoader::PageEnum::PageProtocolWireGuardSettings;
    case Proto::Awg: return PageLoader::PageEnum::PageProtocolAwgSettings;
    case Proto::Ikev2: return PageLoader::PageEnum::PageProtocolIKev2Settings;
    case Proto::L2tp: return PageLoader::PageEnum::PageProtocolIKev2Settings;
    case Proto::Xray: return PageLoader::PageEnum::PageProtocolXraySettings;
    
    // non-vpn
    case Proto::TorWebSite: return PageLoader::PageEnum::PageServiceTorWebsiteSettings;
    case Proto::Dns: return PageLoader::PageEnum::PageServiceDnsSettings;
    case Proto::Sftp: return PageLoader::PageEnum::PageServiceSftpSettings;
    case Proto::Socks5Proxy: return PageLoader::PageEnum::PageServiceSocksProxySettings;
    default: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    }
}

PageLoader::PageEnum ProtocolsModel::clientProtocolPage(Proto protocol) const
{
    switch (protocol) {
    case Proto::WireGuard: return PageLoader::PageEnum::PageProtocolWireGuardClientSettings;
    case Proto::Awg: return PageLoader::PageEnum::PageProtocolAwgClientSettings;
    default: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    }
}
