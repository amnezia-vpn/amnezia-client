#include "serversModel.h"

#include <QHash>
#include <QSet>
#include <QJsonDocument>

#include "core/models/serverConfig.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/networkUtilities.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

#include "core/utils/api/apiUtils.h"

using namespace amnezia;

namespace
{
    namespace configKey
    {
        constexpr char apiConfig[] = "api_config";
        constexpr char serviceInfo[] = "service_info";
        constexpr char availableCountries[] = "available_countries";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serverCountryName[] = "server_country_name";
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char publicKeyInfo[] = "public_key";
        constexpr char expiresAt[] = "expires_at";
    }

    QString normalizeVpnKey(const QString &vpnKey)
    {
        QString normalized = vpnKey.trimmed();
        if (normalized.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
            normalized = normalized.mid(QStringLiteral("vpn://").size());
        }
        return normalized;
    }
}

ServersModel::ServersModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);

    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        if (serverIndex < 0 || serverIndex >= m_servers.size()) {
            return;
        }
        auto defaultContainer = ServerConfigUtils::defaultContainer(m_servers.at(serverIndex));
        emit ServersModel::defaultServerDefaultContainerChanged(defaultContainer);
        emit ServersModel::defaultServerNameChanged();
    });

    connect(this, &ServersModel::processedServerIndexChanged, this, &ServersModel::processedServerChanged);
}

int ServersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(m_servers.size());
}

QVariant ServersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_servers.size())) {
        return QVariant();
    }

    const ServerConfig &server = m_servers.at(index.row());
    const int configVersion = ServerConfigUtils::configVersion(server);
    
    switch (role) {
    case NameRole: {
        if (configVersion) {
            if (ServerConfigUtils::isApiV1Config(server)) {
                return ServerConfigUtils::asApiV1(server).name;
            } else if (ServerConfigUtils::isApiV2Config(server)) {
                return ServerConfigUtils::asApiV2(server).name;
            }
        }
        QString name = ServerConfigUtils::description(server);
        if (name.isEmpty()) {
            return ServerConfigUtils::hostName(server);
        }
        return name;
    }
    case ServerDescriptionRole: {
        auto description = getServerDescription(server, index.row());
        return configVersion ? description : description + ServerConfigUtils::hostName(server);
    }
    case HostNameRole: return ServerConfigUtils::hostName(server);
    case CredentialsRole: return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole: return serverCredentials(index.row()).userName;
    case IsDefaultRole: return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole: return index.row() == m_processedServerIndex;
    case HasWriteAccessRole: {
        auto credentials = serverCredentials(index.row());
        return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
    }
    case ContainsAmneziaDnsRole: {
        QString primaryDns = ServerConfigUtils::dns1(server);
        return primaryDns == protocols::dns::amneziaDnsIp;
    }
    case DefaultContainerRole: {
        return ServerConfigUtils::defaultContainer(server);
    }
    case HasInstalledContainers: {
        return serverHasInstalledContainers(index.row());
    }
    case IsServerFromTelegramApiRole: {
        return configVersion == apiDefs::ConfigSource::Telegram;
    }
    case IsServerFromGatewayApiRole: {
        return configVersion == apiDefs::ConfigSource::AmneziaGateway;
    }
    case ApiConfigRole: {
        return QVariant();
    }
    case IsCountrySelectionAvailableRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return !ServerConfigUtils::asApiV2(server).apiConfig.availableCountries.isEmpty();
        }
        return false;
    }
    case ApiAvailableCountriesRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.availableCountries;
        }
        return QJsonArray();
    }
    case ApiServerCountryCodeRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.serverCountryCode;
        }
        return QString();
    }
    case HasAmneziaDns: {
        QString primaryDns = ServerConfigUtils::dns1(server);
        return primaryDns == protocols::dns::amneziaDnsIp;
    }
    case IsAdVisibleRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.serviceInfo.isAdVisible;
        }
        return false;
    }
    case AdHeaderRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.serviceInfo.adHeader;
        }
        return QString();
    }
    case AdDescriptionRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.serviceInfo.adDescription;
        }
        return QString();
    }
    case AdEndpointRole: {
        if (ServerConfigUtils::isApiV2Config(server)) {
            return ServerConfigUtils::asApiV2(server).apiConfig.serviceInfo.adEndpoint;
        }
        return QString();
    }
    }

    return QVariant();
}

QVariant ServersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ServersModel::updateModel(const QVector<ServerConfig> &servers, int defaultServerIndex, bool isAmneziaDnsEnabled)
{
    beginResetModel();
    m_servers = servers;
    m_defaultServerIndex = defaultServerIndex;
    m_isAmneziaDnsEnabled = isAmneziaDnsEnabled;
    endResetModel();
    emit defaultServerIndexChanged(m_defaultServerIndex);
}

const int ServersModel::getDefaultServerIndex()
{
    return m_defaultServerIndex;
}

QString ServersModel::getServerDescription(const ServerConfig &server, const int index) const
{
    const int configVersion = ServerConfigUtils::configVersion(server);
    QString description;

    if (ServerConfigUtils::isApiV2Config(server)) {
        const ApiV2ServerConfig &apiV2 = ServerConfigUtils::asApiV2(server);
        if (!apiV2.apiConfig.serverCountryCode.isEmpty()) {
            return apiV2.apiConfig.serverCountryName;
        }
        return apiV2.description;
    } else if (ServerConfigUtils::isApiV1Config(server)) {
        const ApiV1ServerConfig &apiV1 = ServerConfigUtils::asApiV1(server);
        return apiV1.description;
    } else if (data(index, HasWriteAccessRole).toBool()) {
        QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
        bool isDnsInstalled = containers.contains(DockerContainer::Dns);
        if (m_isAmneziaDnsEnabled && isDnsInstalled) {
            description += "Amnezia DNS | ";
        }
    } else {
        if (data(index, HasAmneziaDns).toBool()) {
            description += "Amnezia DNS | ";
        }
    }
    
    description += ServerConfigUtils::description(server);
    return description;
}

const int ServersModel::getServersCount()
{
    return m_servers.size();
}

bool ServersModel::hasServerWithWriteAccess()
{
    for (size_t i = 0; i < getServersCount(); i++) {
        if (qvariant_cast<bool>(data(i, HasWriteAccessRole))) {
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

    return roles;
}

ServerCredentials ServersModel::serverCredentials(int index) const
{
    const ServerConfig &server = m_servers.at(index);
    
    if (ServerConfigUtils::isSelfHosted(server)) {
        const SelfHostedServerConfig &selfHosted = ServerConfigUtils::asSelfHosted(server);
        ServerCredentials credentials;
        credentials.hostName = selfHosted.hostName;
        credentials.userName = selfHosted.userName.value_or("");
        credentials.secretData = selfHosted.password.value_or("");
        credentials.port = selfHosted.port.value_or(22);
        return credentials;
    }
    
    return ServerCredentials();
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
    const ServerConfig &server = m_servers.at(serverIndex);
    QMap<DockerContainer, ContainerConfig> containers = ServerConfigUtils::containers(server);
    
    for (auto it = containers.begin(); it != containers.end(); ++it) {
        DockerContainer container = it.key();
        if (ContainerUtils::containerService(container) == ServiceType::Vpn) {
            return true;
        }
        if (container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

