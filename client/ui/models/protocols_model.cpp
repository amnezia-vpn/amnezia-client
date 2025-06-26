#include "protocols_model.h"

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/cloakProtocolConfig.h"
#include "core/models/protocols/openvpnProtocolConfig.h"
#include "core/models/protocols/shadowsocksProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/xrayProtocolConfig.h"

ProtocolsModel::ProtocolsModel(QObject *parent) : QAbstractListModel(parent)
{
}

ProtocolsModel::ProtocolsModel(const QSharedPointer<OpenVpnConfigModel> &openVpnConfigModel,
                               const QSharedPointer<ShadowSocksConfigModel> &shadowSocksConfigModel,
                               const QSharedPointer<CloakConfigModel> &cloakConfigModel,
                               const QSharedPointer<WireGuardConfigModel> &wireGuardConfigModel,
                               const QSharedPointer<AwgConfigModel> &awgConfigModel, const QSharedPointer<XrayConfigModel> &xrayConfigModel,
                               const QSharedPointer<Ikev2ConfigModel> &ikev2ConfigModel,
                               const QSharedPointer<SftpConfigModel> &sftpConfigModel,
                               const QSharedPointer<Socks5ProxyConfigModel> &socks5ProxyConfigModel, QObject *parent)
    : QAbstractListModel(parent),
      m_openVpnConfigModel(openVpnConfigModel),
      m_shadowSocksConfigModel(shadowSocksConfigModel),
      m_cloakConfigModel(cloakConfigModel),
      m_wireGuardConfigModel(wireGuardConfigModel),
      m_awgConfigModel(awgConfigModel),
      m_xrayConfigModel(xrayConfigModel),
      m_ikev2ConfigModel(ikev2ConfigModel),
      m_sftpConfigModel(sftpConfigModel),
      m_socks5ProxyConfigModel(socks5ProxyConfigModel)
{
}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_protocolConfigs.size();
}

QHash<int, QByteArray> ProtocolsModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[ProtocolNameRole] = "protocolName";
    roles[ServerProtocolPageRole] = "serverProtocolPage";
    roles[ClientProtocolPageRole] = "clientProtocolPage";
    roles[ProtocolIndexRole] = "protocolIndex";
    roles[RawConfigRole] = "rawConfig";
    roles[IsClientProtocolExistsRole] = "isClientProtocolExists";

    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_protocolConfigs.size()) {
        return QVariant();
    }

    auto protocolConfig = m_protocolConfigs.at(index.row());
    amnezia::Proto protocol = ProtocolProps::protoFromString(protocolConfig->protocolName);

    switch (role) {
    case ProtocolNameRole: {
        return ProtocolProps::protocolHumanNames().value(protocol);
    }
    case ServerProtocolPageRole: return static_cast<int>(serverProtocolPage(protocol));
    case ClientProtocolPageRole: return static_cast<int>(clientProtocolPage(protocol));
    case ProtocolIndexRole: return protocol;
    // case RawConfigRole: {
    //     auto protocolConfig = m_content.value(ContainerProps::containerTypeToString(m_container)).toObject();
    //     auto lastConfigJsonDoc = QJsonDocument::fromJson(protocolConfig.value(config_key::last_config).toString().toUtf8());
    //     auto lastConfigJson = lastConfigJsonDoc.object();

    //     QString rawConfig;
    //     QStringList lines = lastConfigJson.value(config_key::config).toString().replace("\r", "").split("\n");
    //     for (const QString &l : lines) {
    //         rawConfig.append(l + "\n");
    //     }
    //     return rawConfig;
    // }
    case IsClientProtocolExistsRole: {
        auto protocolVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
        return std::visit([](const auto &ptr) -> bool { return ptr->clientProtocolConfig.isEmpty; }, protocolVariant);
    }
    }

    return QVariant();
}

void ProtocolsModel::updateModel(const QMap<QString, QSharedPointer<ProtocolConfig>> &protocolConfigs)
{
    beginResetModel();
    m_protocolConfigs.clear();
    for (const auto &protocolConfig : protocolConfigs) {
        m_protocolConfigs.push_back(protocolConfig);
    }
    endResetModel();
}

void ProtocolsModel::updateProtocolModel(amnezia::Proto protocol)
{
    QSharedPointer<ProtocolConfig> protocolConfig;

    for (const auto &config : m_protocolConfigs) {
        if (ProtocolProps::protoFromString(config->protocolName) == protocol) {
            protocolConfig = config;
            break;
        }
    }

    switch (protocol) {
    case Proto::OpenVpn: m_openVpnConfigModel->updateModel(config); break;
    case Proto::ShadowSocks: m_shadowSocksConfigModel->updateModel(config); break;
    case Proto::Cloak: m_cloakConfigModel->updateModel(config); break;
    case Proto::WireGuard: m_wireGuardConfigModel->updateModel(config); break;
    case Proto::Awg: m_awgConfigModel->updateModel(config); break;
    case Proto::Xray: m_xrayConfigModel->updateModel(config); break;
#ifdef Q_OS_WINDOWS
    case Proto::Ikev2:
    case Proto::L2tp: m_ikev2ConfigModel->updateModel(config); break;
#endif
    case Proto::Sftp: m_sftpConfigModel->updateModel(config); break;
    case Proto::Socks5Proxy: m_socks5ProxyConfigModel->updateModel(config); break;
    default: break;
    }
}

QMap<QString, QSharedPointer<ProtocolConfig>> ProtocolsModel::getProtocolConfigs()
{
    QMap<QString, QSharedPointer<ProtocolConfig>> protocolConfigs;

    for (const auto &config : m_protocolConfigs) {
        switch (ProtocolProps::protoFromString(config->protocolName)) {
        case Proto::OpenVpn: protocolConfigs.insert(config->protocolName, m_openVpnConfigModel->getConfig()); break;
        case Proto::ShadowSocks: m_shadowSocksConfigModel->updateModel(config); break;
        case Proto::Cloak: m_cloakConfigModel->updateModel(config); break;
        case Proto::WireGuard: m_wireGuardConfigModel->updateModel(config); break;
        case Proto::Awg: protocolConfigs.insert(config->protocolName, m_awgConfigModel->getConfig()); break;
        case Proto::Xray: m_xrayConfigModel->updateModel(config); break;
#ifdef Q_OS_WINDOWS
        case Proto::Ikev2:
        case Proto::L2tp: m_ikev2ConfigModel->updateModel(config); break;
#endif
        case Proto::Sftp: m_sftpConfigModel->updateModel(config); break;
        case Proto::Socks5Proxy: m_socks5ProxyConfigModel->updateModel(config); break;
        default: break;
        }
    }

    return protocolConfigs;
}

PageLoader::PageEnum ProtocolsModel::serverProtocolPage(Proto protocol) const
{
    switch (protocol) {
    case Proto::OpenVpn: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::Cloak: return PageLoader::PageEnum::PageProtocolCloakSettings;
    case Proto::ShadowSocks: return PageLoader::PageEnum::PageProtocolShadowSocksSettings;
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
