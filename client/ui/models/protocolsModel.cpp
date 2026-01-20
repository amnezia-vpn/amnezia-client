#include "protocolsModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/constants/configKeys.h"
#include <QJsonDocument>

using namespace ProtocolUtils;

ProtocolsModel::ProtocolsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProtocolsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_content.size();
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
    if (!index.isValid() || index.row() < 0 || index.row() >= m_content.size()) {
        return QVariant();
    }

    amnezia::Proto proto = ProtocolUtils::protoFromString(m_content.keys().at(index.row()));
    
    switch (role) {
    case ProtocolNameRole: {
        return ProtocolUtils::protocolHumanNames().value(proto);
    }
    case ServerProtocolPageRole:
        return static_cast<int>(serverProtocolPage(proto));
    case ClientProtocolPageRole:
        return static_cast<int>(clientProtocolPage(proto));
    case ProtocolIndexRole: return proto;
    case ProtocolStringRole: return ProtocolUtils::protoToString(proto);
    case IsWireGuardRole: return proto == Proto::WireGuard;
    case IsAwgRole: return proto == Proto::Awg;
    case IsOpenVpnRole: return proto == Proto::OpenVpn;
    case IsXrayRole: return proto == Proto::Xray;
    case IsSftpRole: return proto == Proto::Sftp;
    case IsIpsecRole: return proto == Proto::Ikev2;
    case IsSocks5ProxyRole: return proto == Proto::Socks5Proxy;
    case RawConfigRole: {
        auto protocolConfig = m_content.value(ContainerUtils::containerTypeToProtocolString(m_container)).toObject();
        auto lastConfigJsonDoc =
                QJsonDocument::fromJson(protocolConfig.value(config_key::last_config).toString().toUtf8());
        auto lastConfigJson = lastConfigJsonDoc.object();

        QString rawConfig;
        QStringList lines = lastConfigJson.value(config_key::config).toString().replace("\r", "").split("\n");
        for (const QString &l : lines) {
            rawConfig.append(l + "\n");
        }
        return rawConfig;
    }
    case IsClientProtocolExistsRole: {
        QString protocolKey = ContainerUtils::containerTypeToProtocolString(m_container);
        auto protocolConfig = m_content.value(protocolKey).toObject();
        auto lastConfigJsonDoc =
                QJsonDocument::fromJson(protocolConfig.value(config_key::last_config).toString().toUtf8());
        auto lastConfigJson = lastConfigJsonDoc.object();

        auto configString = lastConfigJson.value(config_key::config).toString();
        return !configString.isEmpty();
    }
    }

    return QVariant();
}

void ProtocolsModel::updateModel(const QJsonObject &content)
{
    m_container = ContainerUtils::containerFromString(content.value(config_key::container).toString());

    m_content = content;
    m_content.remove(config_key::container);
}

QJsonObject ProtocolsModel::getConfig()
{
    QJsonObject config = m_content;
    config.insert(config_key::container, ContainerUtils::containerToString(m_container));
    return config;
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
