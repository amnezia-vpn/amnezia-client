#include "servers_model.h"

#include <QHash>
#include <QSet>
#include <QJsonDocument>

#include "core/api/apiDefs.h"
#include "core/controllers/serverController.h"
#include "core/networkUtilities.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif

#include "core/api/apiUtils.h"

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
}

ServersModel::ServersModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this, &ServersModel::defaultServerIndexChanged, this, &ServersModel::defaultServerNameChanged);

    connect(this, &ServersModel::defaultServerIndexChanged, this, [this](const int serverIndex) {
        if (serverIndex < 0 || serverIndex >= m_servers.size()) {
            return;
        }
        auto defaultContainer =
                ContainerProps::containerFromString(m_servers.at(serverIndex).toObject().value(config_key::defaultContainer).toString());
        emit ServersModel::defaultServerDefaultContainerChanged(defaultContainer);
        emit ServersModel::defaultServerNameChanged();
        updateDefaultServerContainersModel();
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

    const QJsonObject server = m_servers.at(index.row()).toObject();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();
    const auto configVersion = server.value(config_key::configVersion).toInt();
    switch (role) {
    case NameRole: {
        if (configVersion) {
            return server.value(config_key::name).toString();
        }
        auto name = server.value(config_key::description).toString();
        if (name.isEmpty()) {
            return server.value(config_key::hostName).toString();
        }
        return name;
    }
    case ServerDescriptionRole: {
        auto description = getServerDescription(server, index.row());
        return configVersion ? description : description + server.value(config_key::hostName).toString();
    }
    case HostNameRole: return server.value(config_key::hostName).toString();
    case CredentialsRole: return QVariant::fromValue(serverCredentials(index.row()));
    case CredentialsLoginRole: return serverCredentials(index.row()).userName;
    case IsDefaultRole: return index.row() == m_defaultServerIndex;
    case IsCurrentlyProcessedRole: return index.row() == m_processedServerIndex;
    case HasWriteAccessRole: {
        auto credentials = serverCredentials(index.row());
        return (!credentials.userName.isEmpty() && !credentials.secretData.isEmpty());
    }
    case ContainsAmneziaDnsRole: {
        QString primaryDns = server.value(config_key::dns1).toString();
        return primaryDns == protocols::dns::amneziaDnsIp;
    }
    case DefaultContainerRole: {
        return ContainerProps::containerFromString(server.value(config_key::defaultContainer).toString());
    }
    case HasInstalledContainers: {
        return serverHasInstalledContainers(index.row());
    }
    case IsServerFromTelegramApiRole: {
        return server.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::Telegram;
    }
    case IsServerFromGatewayApiRole: {
        return server.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway;
    }
    case ApiConfigRole: {
        return apiConfig;
    }
    case IsCountrySelectionAvailableRole: {
        return !apiConfig.value(configKey::availableCountries).toArray().isEmpty();
    }
    case ApiAvailableCountriesRole: {
        return apiConfig.value(configKey::availableCountries).toArray();
    }
    case ApiServerCountryCodeRole: {
        return apiConfig.value(configKey::serverCountryCode).toString();
    }
    case HasAmneziaDns: {
        QString primaryDns = server.value(config_key::dns1).toString();
        return primaryDns == protocols::dns::amneziaDnsIp;
    }
    case IsAdVisibleRole:{
        return apiConfig.value(apiDefs::key::serviceInfo).toObject().value(apiDefs::key::isAdVisible).toBool(false);
    }
    case AdHeaderRole: {
        return apiConfig.value(apiDefs::key::serviceInfo).toObject().value(apiDefs::key::adHeader).toString();
    }
    case AdDescriptionRole: {
        return apiConfig.value(apiDefs::key::serviceInfo).toObject().value(apiDefs::key::adDescription).toString();
    }
    case AdEndpointRole: {
        return apiConfig.value(apiDefs::key::serviceInfo).toObject().value(apiDefs::key::adEndpoint).toString();
    }
    }

    return QVariant();
}

QVariant ServersModel::data(const int index, int role) const
{
    QModelIndex modelIndex = this->index(index);
    return data(modelIndex, role);
}

void ServersModel::updateModel(const QJsonArray &servers, int defaultServerIndex, bool isAmneziaDnsEnabled)
{
    beginResetModel();
    m_servers = servers;
    m_defaultServerIndex = defaultServerIndex;
    m_processedServerIndex = defaultServerIndex;
    m_isAmneziaDnsEnabled = isAmneziaDnsEnabled;
    endResetModel();
    emit defaultServerIndexChanged(m_defaultServerIndex);
    updateContainersModel();
    updateDefaultServerContainersModel();
}

const int ServersModel::getDefaultServerIndex()
{
    return m_defaultServerIndex;
}

QString ServersModel::getServerDescription(const QJsonObject &server, const int index) const
{
    const auto configVersion = server.value(config_key::configVersion).toInt();
    const auto apiConfig = server.value(configKey::apiConfig).toObject();

    QString description;

    if (configVersion && !apiConfig.value(configKey::serverCountryCode).toString().isEmpty()) {
        return apiConfig.value(configKey::serverCountryName).toString();
    } else if (configVersion) {
        return server.value(config_key::description).toString();
    } else if (data(index, HasWriteAccessRole).toBool()) {
        const QJsonArray containers = server.value(config_key::containers).toArray();
        bool isDnsInstalled = false;
        for (const auto &containerValue : containers) {
            QJsonObject containerObj = containerValue.toObject();
            DockerContainer containerType = ContainerProps::containerFromString(containerObj.value(config_key::container).toString());
            if (containerType == DockerContainer::Dns) {
                isDnsInstalled = true;
                break;
            }
        }
        if (m_isAmneziaDnsEnabled && isDnsInstalled) {
            description += "Amnezia DNS | ";
        }
    } else {
        if (data(index, HasAmneziaDns).toBool()) {
            description += "Amnezia DNS | ";
        }
    }
    return description;
}

const int ServersModel::getServersCount()
{
    return m_servers.count();
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
    m_processedServerIndex = index;
    updateContainersModel();
    if (data(index, IsServerFromGatewayApiRole).toBool()) {
        if (data(index, IsCountrySelectionAvailableRole).toBool()) {
            emit updateApiCountryModel();
        }
        emit updateApiServicesModel();
    }
    emit processedServerIndexChanged(m_processedServerIndex);
}

int ServersModel::getProcessedServerIndex()
{
    return m_processedServerIndex;
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
    const QJsonObject &s = m_servers.at(index).toObject();

    ServerCredentials credentials;
    credentials.hostName = s.value(config_key::hostName).toString();
    credentials.userName = s.value(config_key::userName).toString();
    credentials.secretData = s.value(config_key::password).toString();
    credentials.port = s.value(config_key::port).toInt();

    return credentials;
}

void ServersModel::updateContainersModel()
{
    auto containers = m_servers.at(m_processedServerIndex).toObject().value(config_key::containers).toArray();
    emit containersUpdated(containers);
}

void ServersModel::updateDefaultServerContainersModel()
{
    auto containers = m_servers.at(m_defaultServerIndex).toObject().value(config_key::containers).toArray();
    emit defaultServerContainersUpdated(containers);
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

QVariant ServersModel::getProcessedServerData(const QString roleString)
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
    QJsonObject server = m_servers.at(serverIndex).toObject();
    const auto containers = server.value(config_key::containers).toArray();
    for (auto it = containers.begin(); it != containers.end(); it++) {
        auto container = ContainerProps::containerFromString(it->toObject().value(config_key::container).toString());
        if (ContainerProps::containerService(container) == ServiceType::Vpn) {
            return true;
        }
        if (container == DockerContainer::SSXray) {
            return true;
        }
    }
    return false;
}

