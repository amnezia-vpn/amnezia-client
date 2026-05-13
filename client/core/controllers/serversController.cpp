#include "serversController.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/models/containerConfig.h"

#include "core/models/serverDescription.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <AmneziaVPN-Swift.h>
#endif


ServersController::ServersController(SecureServersRepository *serversRepository,
                                      SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    ensureDefaultServerValid();
    recomputeGatewayStacks();
}

void ServersController::ensureDefaultServerValid()
{
    if (!getServersCount()) {
        return;
    }

    const QString defaultId = getDefaultServerId();
    if (!defaultId.isEmpty() && indexOfServerId(defaultId) >= 0) {
        return;
    }

    const QString firstId = getServerId(0);
    if (!firstId.isEmpty()) {
        setDefaultServer(firstId);
    }
}

bool ServersController::renameServer(const QString &serverId, const QString &name)
{
    switch (m_serversRepository->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) return false;
        cfg->description = name;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedAdmin);
        return true;
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) return false;
        cfg->description = name;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedUser);
        return true;
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        auto cfg = m_serversRepository->nativeConfig(serverId);
        if (!cfg.has_value()) return false;
        cfg->description = name;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::Native);
        return true;
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        auto cfg = m_serversRepository->apiV2Config(serverId);
        if (!cfg.has_value()) return false;
        cfg->name = name;
        cfg->nameOverriddenByUser = true;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::ApiV2);
        return true;
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1:
    case SecureServersRepository::ServerConfigKind::Invalid:
        return false;
    }
    return false;
}

void ServersController::removeServer(const QString &serverId)
{
    m_serversRepository->removeServer(serverId);
}

void ServersController::setDefaultServer(const QString &serverId)
{
    m_serversRepository->setDefaultServer(serverId);
}

void ServersController::setDefaultContainer(const QString &serverId, DockerContainer container)
{
    switch (m_serversRepository->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->defaultContainer = container;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedAdmin);
        return;
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->defaultContainer = container;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::SelfHostedUser);
        return;
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        auto cfg = m_serversRepository->nativeConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->defaultContainer = container;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::Native);
        return;
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        auto cfg = m_serversRepository->apiV2Config(serverId);
        if (!cfg.has_value()) return;
        cfg->defaultContainer = container;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::ApiV2);
        return;
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1: {
        auto cfg = m_serversRepository->legacyApiConfig(serverId);
        if (!cfg.has_value()) return;
        cfg->defaultContainer = container;
        m_serversRepository->editServer(serverId, cfg->toJson(), SecureServersRepository::ServerConfigKind::LegacyApiV1);
        return;
    }
    case SecureServersRepository::ServerConfigKind::Invalid:
        return;
    }
}

QVector<ServerDescription> ServersController::buildServerDescriptions(bool isAmneziaDnsEnabled) const
{
    QVector<ServerDescription> out;
    const QVector<QString> ids = m_serversRepository->orderedServerIds();
    out.reserve(ids.size());

    for (const QString &id : ids) {
        ServerDescription d;
        using Kind = SecureServersRepository::ServerConfigKind;
        const Kind kind = m_serversRepository->serverKind(id);
        switch (kind) {
        case Kind::SelfHostedAdmin: {
            const auto cfg = m_serversRepository->selfHostedAdminConfig(id);
            if (!cfg) {
                continue;
            }
            d = buildServerDescription(*cfg, isAmneziaDnsEnabled);
            break;
        }
        case Kind::SelfHostedUser: {
            const auto cfg = m_serversRepository->selfHostedUserConfig(id);
            if (!cfg) {
                continue;
            }
            d = buildServerDescription(*cfg, isAmneziaDnsEnabled);
            break;
        }
        case Kind::Native: {
            const auto cfg = m_serversRepository->nativeConfig(id);
            if (!cfg) {
                continue;
            }
            d = buildServerDescription(*cfg, isAmneziaDnsEnabled);
            break;
        }
        case Kind::ApiV2: {
            const auto cfg = m_serversRepository->apiV2Config(id);
            if (!cfg) {
                continue;
            }
            d = buildServerDescription(*cfg, isAmneziaDnsEnabled);
            break;
        }
        case Kind::LegacyApiV1: {
            const auto cfg = m_serversRepository->legacyApiConfig(id);
            if (!cfg) {
                continue;
            }
            d = buildServerDescription(*cfg, isAmneziaDnsEnabled);
            break;
        }
        case Kind::Invalid:
            continue;
        }

        d.serverId = id;
        out.append(d);
    }
    return out;
}

QMap<DockerContainer, ContainerConfig> ServersController::getServerContainersMap(const QString &serverId) const
{
    switch (m_serversRepository->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        return cfg.has_value() ? cfg->containers : QMap<DockerContainer, ContainerConfig>{};
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        return cfg.has_value() ? cfg->containers : QMap<DockerContainer, ContainerConfig>{};
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        const auto cfg = m_serversRepository->nativeConfig(serverId);
        return cfg.has_value() ? cfg->containers : QMap<DockerContainer, ContainerConfig>{};
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        const auto cfg = m_serversRepository->apiV2Config(serverId);
        return cfg.has_value() ? cfg->containers : QMap<DockerContainer, ContainerConfig>{};
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1: {
        const auto cfg = m_serversRepository->legacyApiConfig(serverId);
        return cfg.has_value() ? cfg->containers : QMap<DockerContainer, ContainerConfig>{};
    }
    case SecureServersRepository::ServerConfigKind::Invalid:
        return {};
    }
    return {};
}

DockerContainer ServersController::getDefaultContainer(const QString &serverId) const
{
    switch (m_serversRepository->serverKind(serverId)) {
    case SecureServersRepository::ServerConfigKind::SelfHostedAdmin: {
        const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
        return cfg.has_value() ? cfg->defaultContainer : DockerContainer::None;
    }
    case SecureServersRepository::ServerConfigKind::SelfHostedUser: {
        const auto cfg = m_serversRepository->selfHostedUserConfig(serverId);
        return cfg.has_value() ? cfg->defaultContainer : DockerContainer::None;
    }
    case SecureServersRepository::ServerConfigKind::Native: {
        const auto cfg = m_serversRepository->nativeConfig(serverId);
        return cfg.has_value() ? cfg->defaultContainer : DockerContainer::None;
    }
    case SecureServersRepository::ServerConfigKind::ApiV2: {
        const auto cfg = m_serversRepository->apiV2Config(serverId);
        return cfg.has_value() ? cfg->defaultContainer : DockerContainer::None;
    }
    case SecureServersRepository::ServerConfigKind::LegacyApiV1: {
        const auto cfg = m_serversRepository->legacyApiConfig(serverId);
        return cfg.has_value() ? cfg->defaultContainer : DockerContainer::None;
    }
    case SecureServersRepository::ServerConfigKind::Invalid:
        return DockerContainer::None;
    }
    return DockerContainer::None;
}

ContainerConfig ServersController::getContainerConfig(const QString &serverId, DockerContainer container) const
{
    return getServerContainersMap(serverId).value(container);
}

int ServersController::getDefaultServerIndex() const
{
    return m_serversRepository->defaultServerIndex();
}

QString ServersController::getDefaultServerId() const
{
    return m_serversRepository->defaultServerId();
}

int ServersController::getServersCount() const
{
    return m_serversRepository->serversCount();
}

QString ServersController::getServerId(int serverIndex) const
{
    return m_serversRepository->serverIdAt(serverIndex);
}

int ServersController::indexOfServerId(const QString &serverId) const
{
    return m_serversRepository->indexOfServerId(serverId);
}

std::optional<ApiV2ServerConfig> ServersController::apiV2Config(const QString &serverId) const
{
    return m_serversRepository->apiV2Config(serverId);
}

std::optional<SelfHostedAdminServerConfig> ServersController::selfHostedAdminConfig(const QString &serverId) const
{
    return m_serversRepository->selfHostedAdminConfig(serverId);
}

ServerCredentials ServersController::getServerCredentials(const QString &serverId) const
{
    const auto cfg = m_serversRepository->selfHostedAdminConfig(serverId);
    if (cfg.has_value()) {
        const ServerCredentials creds = cfg->credentials();
        if (creds.isValid()) {
            return creds;
        }
    }
    return ServerCredentials {};
}

ServersController::GatewayStacksData ServersController::gatewayStacks() const
{
    return m_gatewayStacks;
}

void ServersController::recomputeGatewayStacks()
{
    GatewayStacksData computed;
    bool hasNewTags = false;

    const QVector<QString> ids = m_serversRepository->orderedServerIds();
    for (const QString &id : ids) {
        const auto apiV2 = m_serversRepository->apiV2Config(id);
        if (!apiV2.has_value()) {
            continue;
        }
        const QString userCountryCode = apiV2->apiConfig.userCountryCode;
        const QString serviceType = apiV2->serviceType();

        if (!userCountryCode.isEmpty()) {
            if (!m_gatewayStacks.userCountryCodes.contains(userCountryCode)) {
                hasNewTags = true;
            }
            computed.userCountryCodes.insert(userCountryCode);
        }

        if (!serviceType.isEmpty()) {
            if (!m_gatewayStacks.serviceTypes.contains(serviceType)) {
                hasNewTags = true;
            }
            computed.serviceTypes.insert(serviceType);
        }
    }

    m_gatewayStacks = std::move(computed);
    if (hasNewTags) {
        emit gatewayStacksExpanded();
    }
}

bool ServersController::GatewayStacksData::operator==(const GatewayStacksData &other) const
{
    return userCountryCodes == other.userCountryCodes && serviceTypes == other.serviceTypes;
}

QJsonObject ServersController::GatewayStacksData::toJson() const
{
    QJsonObject json;

    QJsonArray userCountryCodesArray;
    for (const QString &code : userCountryCodes) {
        userCountryCodesArray.append(code);
    }
    json[apiDefs::key::userCountryCode] = userCountryCodesArray;

    QJsonArray serviceTypesArray;
    for (const QString &type : serviceTypes) {
        serviceTypesArray.append(type);
    }
    json[apiDefs::key::serviceType] = serviceTypesArray;

    return json;
}

bool ServersController::isServerFromApiAlreadyExists(const QString &userCountryCode, const QString &serviceType,
                                                      const QString &serviceProtocol) const
{
    const QVector<QString> ids = m_serversRepository->orderedServerIds();
    for (const QString &id : ids) {
        const auto apiV2 = m_serversRepository->apiV2Config(id);
        if (!apiV2.has_value()) {
            continue;
        }
        if (apiV2->apiConfig.userCountryCode == userCountryCode && apiV2->serviceType() == serviceType
            && apiV2->serviceProtocol() == serviceProtocol) {
            return true;
        }
    }
    return false;
}

bool ServersController::hasInstalledContainers(const QString &serverId) const
{
    const QMap<DockerContainer, ContainerConfig> containers = getServerContainersMap(serverId);

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
