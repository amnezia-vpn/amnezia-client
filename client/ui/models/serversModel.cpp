#include "serversModel.h"

#include "core/models/serverDescription.h"

#include <QHash>
#include <QSet>
#include <QJsonDocument>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

#include "core/utils/api/apiUtils.h"

using namespace amnezia;

namespace {
int rowForServerId(const QVector<ServerDescription> &descriptions, const QString &serverId)
{
    if (serverId.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < descriptions.size(); ++i) {
        if (descriptions.at(i).serverId == serverId) {
            return i;
        }
    }

    return -1;
}
} // namespace

ServersModel::ServersModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_descriptions.size());
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_descriptions.size())) {
        return QVariant();
    }

    const ServerDescription &row = m_descriptions.at(index.row());
    const int configVersion = row.configVersion;

    switch (role) {
    case NameRole:
        return row.serverName;
    case ServerDescriptionRole:
        return configVersion ? row.baseDescription : (row.baseDescription + row.hostName);
    case HostNameRole:
        return row.hostName;
    case ServerIdRole:
        return row.serverId;
    case CredentialsLoginRole:
        return serverCredentials(index.row()).userName;
    case IsDefaultRole:
        return row.serverId == m_defaultServerId;
    case HasWriteAccessRole:
        return row.hasWriteAccess;
    case DefaultContainerRole:
        return QVariant::fromValue(row.defaultContainer);
    case HasInstalledContainers:
        return row.hasInstalledVpnContainers;
    case IsServerFromGatewayApiRole:
        return row.isServerFromGatewayApi;
    case IsSubscriptionExpiredRole:
        return row.isSubscriptionExpired;
    case IsSubscriptionExpiringSoonRole:
        return row.isSubscriptionExpiringSoon;
    }

    return QVariant();
}

QVariant ServersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ServersModel::updateModel(const QVector<ServerDescription> &descriptions,
                               const QString &defaultServerId)
{
    beginResetModel();
    m_descriptions = descriptions;
    m_defaultServerId = defaultServerId;
    endResetModel();
}

void ServersModel::setDefaultServerId(const QString &serverId)
{
    if (m_defaultServerId == serverId) {
        return;
    }

    const int oldIndex = rowForServerId(m_descriptions, m_defaultServerId);
    const int newIndex = rowForServerId(m_descriptions, serverId);
    m_defaultServerId = serverId;

    const QVector<int> roles = { IsDefaultRole };
    if (oldIndex >= 0 && oldIndex < m_descriptions.size()) {
        emit dataChanged(this->index(oldIndex), this->index(oldIndex), roles);
    }
    if (newIndex >= 0 && newIndex < m_descriptions.size()) {
        emit dataChanged(this->index(newIndex), this->index(newIndex), roles);
    }
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NameRole] = "name";
    roles[ServerDescriptionRole] = "serverDescription";

    roles[HostNameRole] = "hostName";
    roles[ServerIdRole] = "serverId";

    roles[CredentialsLoginRole] = "credentialsLogin";

    roles[IsDefaultRole] = "isDefault";
    roles[HasWriteAccessRole] = "hasWriteAccess";

    roles[DefaultContainerRole] = "defaultContainer";
    roles[HasInstalledContainers] = "hasInstalledContainers";

    roles[IsServerFromGatewayApiRole] = "isServerFromGatewayApi";
    roles[IsSubscriptionExpiredRole] = "isSubscriptionExpired";
    roles[IsSubscriptionExpiringSoonRole] = "isSubscriptionExpiringSoon";

    return roles;
}

ServerCredentials ServersModel::serverCredentials(int index) const
{
    if (index < 0 || index >= m_descriptions.size()) {
        return ServerCredentials();
    }
    return m_descriptions.at(index).selfHostedSshCredentials;
}

