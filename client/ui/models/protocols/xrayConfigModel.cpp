#include "xrayConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;
using namespace ProtocolUtils;

XrayConfigModel::XrayConfigModel(QObject *parent) : QAbstractListModel(parent)
{
}

int XrayConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool XrayConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerUtils::allContainers().size()) {
        return false;
    }

    QString strValue = value.toString();

    switch (role) {
    case Roles::SiteRole: m_protocolConfig.serverConfig.site = strValue; break;
    case Roles::PortRole: m_protocolConfig.serverConfig.port = strValue; break;
    default:
        return false;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant XrayConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    switch (role) {
    case Roles::SiteRole: return m_protocolConfig.serverConfig.site;
    case Roles::PortRole: return m_protocolConfig.serverConfig.port;
    }

    return QVariant();
}

void XrayConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::XrayProtocolConfig &protocolConfig)
{
    beginResetModel();
    m_container = container;
    
    m_protocolConfig = protocolConfig;
    
    applyDefaultsToServerConfig(m_protocolConfig.serverConfig);
    
    m_originalProtocolConfig = m_protocolConfig;
    
    endResetModel();
}

void XrayConfigModel::applyDefaultsToServerConfig(amnezia::XrayServerConfig& config)
{
    if (config.port.isEmpty()) {
        config.port = protocols::xray::defaultPort;
    }
    if (config.transportProto.isEmpty()) {
        config.transportProto = ProtocolUtils::transportProtoToString(
            ProtocolUtils::defaultTransportProto(amnezia::Proto::Xray), amnezia::Proto::Xray);
    }
    if (config.site.isEmpty()) {
        config.site = protocols::xray::defaultSite;
    }
}

amnezia::XrayProtocolConfig XrayConfigModel::getProtocolConfig()
{
    bool serverSettingsChanged = !m_protocolConfig.serverConfig.hasEqualServerSettings(m_originalProtocolConfig.serverConfig);
    
    if (serverSettingsChanged) {
        m_protocolConfig.clearClientConfig();
    }
    
    return m_protocolConfig;
}

QHash<int, QByteArray> XrayConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SiteRole] = "site";
    roles[PortRole] = "port";

    return roles;
}
