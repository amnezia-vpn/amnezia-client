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

ServersModel::ServersModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);

    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        if (serverIndex < 0 || serverIndex >= m_descriptions.size()) {
            return;
        }
        auto defaultContainer = m_descriptions.at(serverIndex).defaultContainer;
        emit ServersModel::defaultServerDefaultContainerChanged(defaultContainer);
        emit ServersModel::defaultServerNameChanged();
    });

    connect(this, &ServersModel::processedServerIndexChanged, this, &ServersModel::processedServerChanged);
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
    case CollapsedServerDescriptionRole:
        return row.collapsedServerDescription;
    case ExpandedServerDescriptionRole:
        return row.expandedServerDescription;
    case HostNameRole:
        return row.hostName;
    case CredentialsRole:
        return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole:
        return serverCredentials(index.row()).userName;
    case IsDefaultRole:
        return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole:
        return index.row() == m_processedServerIndex;
    case HasWriteAccessRole:
        return row.hasWriteAccess;
    case ContainsAmneziaDnsRole:
        return row.primaryDnsIsAmnezia;
    case DefaultContainerRole:
        return QVariant::fromValue(row.defaultContainer);
    case HasInstalledContainers:
        return row.hasInstalledVpnContainers;
    case IsServerFromTelegramApiRole:
        return false;
    case IsServerFromGatewayApiRole:
        return row.isServerFromGatewayApi;
    case ApiConfigRole:
        return QVariant();
    case IsCountrySelectionAvailableRole:
        return row.isCountrySelectionAvailable;
    case ApiAvailableCountriesRole:
        return row.apiAvailableCountries;
    case ApiServerCountryCodeRole:
        return row.apiServerCountryCode;
    case HasAmneziaDns:
        return row.primaryDnsIsAmnezia;
    case IsAdVisibleRole:
        return row.isAdVisible;
    case AdHeaderRole:
        return row.adHeader;
    case AdDescriptionRole:
        return row.adDescription;
    case AdEndpointRole:
        return row.adEndpoint;
    case IsRenewalAvailableRole:
        return row.isRenewalAvailable;
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

void ServersModel::updateModel(const QVector<ServerDescription> &descriptions, int defaultServerIndex)
{
    beginResetModel();
    m_descriptions = descriptions;
    m_defaultServerIndex = defaultServerIndex;
    endResetModel();
    emit defaultServerIndexChanged(m_defaultServerIndex);
    emit processedServerChanged();
}

const int ServersModel::getDefaultServerIndex()
{
    return m_defaultServerIndex;
}

const int ServersModel::getServersCount()
{
    return m_descriptions.size();
}

bool ServersModel::hasServerWithWriteAccess()
{
    for (size_t i = 0; i < getServersCount(); i++) {
        if (qvariant_cast<bool>(data(static_cast<int>(i), HasWriteAccessRole))) {
            return true;
        }
    }
    return false;
}

void ServersModel::setProcessedServerIndex(const int index)
{
    if (m_processedServerIndex != index) {
        m_processedServerIndex = index;
        emit processedServerIndexChanged(m_processedServerIndex);
    }
}

const ServerCredentials ServersModel::getProcessedServerCredentials()
{
    return serverCredentials(m_processedServerIndex);
}

bool ServersModel::isDefaultServerCurrentlyProcessed()
{
    return m_defaultServerIndex == m_processedServerIndex;
}

bool ServersModel::isDefaultServerFromApi()
{
    return data(m_defaultServerIndex, IsServerFromTelegramApiRole).toBool()
            || data(m_defaultServerIndex, IsServerFromGatewayApiRole).toBool();
}

bool ServersModel::isProcessedServerHasWriteAccess()
{
    return qvariant_cast<bool>(data(m_processedServerIndex, HasWriteAccessRole));
}

bool ServersModel::isDefaultServerHasWriteAccess()
{
    return qvariant_cast<bool>(data(m_defaultServerIndex, HasWriteAccessRole));
}

QHash<int, QByteArray> ServersModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[NameRole] = "name";
    roles[ServerDescriptionRole] = "serverDescription";
    roles[CollapsedServerDescriptionRole] = "collapsedServerDescription";
    roles[ExpandedServerDescriptionRole] = "expandedServerDescription";

    roles[HostNameRole] = "hostName";

    roles[CredentialsRole] = "credentials";
    roles[CredentialsLoginRole] = "credentialsLogin";

    roles[IsDefaultRole] = "isDefault";
    roles[IsCurrentlyProcessedRole] = "isCurrentlyProcessed";

    roles[HasWriteAccessRole] = "hasWriteAccess";

    roles[ContainsAmneziaDnsRole] = "containsAmneziaDns";

    roles[DefaultContainerRole] = "defaultContainer";
    roles[HasInstalledContainers] = "hasInstalledContainers";

    roles[IsServerFromTelegramApiRole] = "isServerFromTelegramApi";
    roles[IsServerFromGatewayApiRole] = "isServerFromGatewayApi";
    roles[ApiConfigRole] = "apiConfig";
    roles[IsCountrySelectionAvailableRole] = "isCountrySelectionAvailable";
    roles[ApiAvailableCountriesRole] = "apiAvailableCountries";
    roles[ApiServerCountryCodeRole] = "apiServerCountryCode";

    roles[IsAdVisibleRole] = "isAdVisible";
    roles[AdHeaderRole] = "adHeader";
    roles[AdDescriptionRole] = "adDescription";
    roles[AdEndpointRole] = "adEndpoint";
    roles[IsRenewalAvailableRole] = "isRenewalAvailable";
    roles[IsSubscriptionExpiredRole] = "isSubscriptionExpired";
    roles[IsSubscriptionExpiringSoonRole] = "isSubscriptionExpiringSoon";

    roles[HasAmneziaDns] = "hasAmneziaDns";

    return roles;
}

ServerCredentials ServersModel::serverCredentials(int index) const
{
    if (index < 0 || index >= m_descriptions.size()) {
        return ServerCredentials();
    }
    return m_descriptions.at(index).selfHostedSshCredentials;
}

bool ServersModel::isServerFromApi(const int serverIndex)
{
    return data(serverIndex, IsServerFromTelegramApiRole).toBool()
            || data(serverIndex, IsServerFromGatewayApiRole).toBool();
}

QVariant ServersModel::getDefaultServerData(const QString roleString)
{
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(m_defaultServerIndex, it.key());
        }
    }

    return {};
}

QVariant ServersModel::getProcessedServerData(const QString &roleString)
{
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(m_processedServerIndex, it.key());
        }
    }

    return {};
}

bool ServersModel::serverHasInstalledContainers(const int serverIndex) const
{
    if (serverIndex < 0 || serverIndex >= m_descriptions.size()) {
        return false;
    }
    return m_descriptions.at(serverIndex).hasInstalledVpnContainers;
}
