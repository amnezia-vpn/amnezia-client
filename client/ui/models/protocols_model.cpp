#include "protocols_model.h"

ProtocolsModel::ProtocolsModel(std::shared_ptr<Settings> settings, QObject *parent)
    : m_settings(settings), QAbstractListModel(parent)
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
    roles[ProtocolPageRole] = "protocolPage";
    roles[ProtocolIndexRole] = "protocolIndex";
    roles[RawConfigRole] = "rawConfig";

    return roles;
}

QVariant ProtocolsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_content.size()) {
        return QVariant();
    }

    switch (role) {
    case ProtocolNameRole: {
        amnezia::Proto proto = ProtocolProps::protoFromString(m_content.keys().at(index.row()));
        return ProtocolProps::protocolHumanNames().value(proto);
    }
    case ProtocolPageRole:
        return static_cast<int>(protocolPage(ProtocolProps::protoFromString(m_content.keys().at(index.row()))));
    case ProtocolIndexRole: return ProtocolProps::protoFromString(m_content.keys().at(index.row()));
    case RawConfigRole: {
        auto protocolConfig = m_content.value(ContainerProps::containerTypeToString(m_container)).toObject();
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
    }

    return QVariant();
}

void ProtocolsModel::updateModel(const QJsonObject &content)
{
    m_container = ContainerProps::containerFromString(content.value(config_key::container).toString());

    m_content = content;
    m_content.remove(config_key::container);
}

QJsonObject ProtocolsModel::getConfig()
{
    QJsonObject config = m_content;
    config.insert(config_key::container, ContainerProps::containerToString(m_container));
    return config;
}

PageLoader::PageEnum ProtocolsModel::protocolPage(Proto protocol) const
{
    switch (protocol) {
    case Proto::OpenVpn: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::Cloak: return PageLoader::PageEnum::PageProtocolCloakSettings;
    case Proto::ShadowSocks: return PageLoader::PageEnum::PageProtocolShadowSocksSettings;
    case Proto::WireGuard: return PageLoader::PageEnum::PageProtocolWireGuardSettings;
    case Proto::Ikev2: return PageLoader::PageEnum::PageProtocolIKev2Settings;
    case Proto::L2tp: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    // non-vpn
    case Proto::TorWebSite: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::Dns: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::FileShare: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    case Proto::Sftp: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    default: return PageLoader::PageEnum::PageProtocolOpenVpnSettings;
    }
}
