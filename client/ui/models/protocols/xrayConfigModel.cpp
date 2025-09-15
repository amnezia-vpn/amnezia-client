#include "xrayConfigModel.h"

#include "protocols/protocols_defs.h"

XrayConfigModel::XrayConfigModel(QObject *parent)
    : QAbstractListModel(parent),
      m_newXrayProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::Xray)),
      m_oldXrayProtocolConfig(QJsonObject(), ProtocolProps::protoToString(Proto::Xray))
{
}

int XrayConfigModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

bool XrayConfigModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= ContainerProps::allContainers().size()) {
        return false;
    }

    switch (role) {
    case Roles::SiteRole: m_newXrayProtocolConfig.serverProtocolConfig.site = value.toString(); break;
    case Roles::PortRole: m_newXrayProtocolConfig.serverProtocolConfig.port = value.toString(); break;
    }

    emit dataChanged(index, index, QList { role });
    return true;
}

QVariant XrayConfigModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return false;
    }

    switch (role) {
    case Roles::SiteRole: return m_newXrayProtocolConfig.serverProtocolConfig.site;
    case Roles::PortRole: return m_newXrayProtocolConfig.serverProtocolConfig.port;
    }

    return QVariant();
}

void XrayConfigModel::updateModel(const XrayProtocolConfig xrayProtocolConfig)
{
    beginResetModel();
    m_newXrayProtocolConfig = xrayProtocolConfig;
    m_oldXrayProtocolConfig = xrayProtocolConfig;
    endResetModel();
}

QSharedPointer<ProtocolConfig> XrayConfigModel::getConfig()
{
    if (m_oldXrayProtocolConfig.hasEqualServerSettings(m_newXrayProtocolConfig)) {
        m_newXrayProtocolConfig.clearClientSettings();
    }
    return QSharedPointer<XrayProtocolConfig>::create(m_newXrayProtocolConfig);
}

bool XrayConfigModel::isServerSettingsEqual()
{
    return m_oldXrayProtocolConfig.hasEqualServerSettings(m_newXrayProtocolConfig);
}

QHash<int, QByteArray> XrayConfigModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[SiteRole] = "site";
    roles[PortRole] = "port";

    return roles;
}

