#include "secureServersRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QUuid>

#include "core/utils/api/apiEnums.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"

using namespace amnezia;

namespace {

QString readStorageServerId(const QJsonObject &json)
{
    return json.value(QString(configKey::storageServerId)).toString().trimmed();
}

QJsonObject withoutStorageServerId(const QJsonObject &json)
{
    QJsonObject o = json;
    o.remove(QString(configKey::storageServerId));
    return o;
}

QJsonObject embedStorageServerId(const QString &serverId, const QJsonObject &payloadSansId)
{
    QJsonObject o = payloadSansId;
    o.insert(QString(configKey::storageServerId), serverId);
    return o;
}

bool hasThirdPartyConfig(const QJsonObject &json)
{
    const QJsonArray containersArray = json.value(configKey::containers).toArray();
    for (const QJsonValue &val : containersArray) {
        const QJsonObject containerObj = val.toObject();
        for (auto it = containerObj.begin(); it != containerObj.end(); ++it) {
            if (it.key() == configKey::container) {
                continue;
            }
            const QJsonObject protocolObj = it.value().toObject();
            if (protocolObj.contains(configKey::isThirdPartyConfig)
                && protocolObj.value(configKey::isThirdPartyConfig).toBool()) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

SecureServersRepository::SecureServersRepository(SecureQSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    loadFromStorage();
    persistDefaultServerFields();
}

QVariant SecureServersRepository::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SecureServersRepository::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

void SecureServersRepository::clearServerStateMaps()
{
    m_serverJsonById.clear();
    m_orderedServerIds.clear();
}

QString SecureServersRepository::normalizedOrGeneratedServerId(const QString &candidateId) const
{
    const QString trimmed = candidateId.trimmed();
    if (!trimmed.isEmpty() && !m_serverJsonById.contains(trimmed)) {
        return trimmed;
    }
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

SecureServersRepository::ServerConfigKind SecureServersRepository::kindFromJson(const QJsonObject &serverJson) const
{
    const apiDefs::ConfigType configType = apiUtils::getConfigType(serverJson);
    switch (configType) {
    case apiDefs::ConfigType::AmneziaPremiumV1:
    case apiDefs::ConfigType::AmneziaFreeV2:
        return ServerConfigKind::LegacyApiV1;
    case apiDefs::ConfigType::AmneziaPremiumV2:
    case apiDefs::ConfigType::AmneziaFreeV3:
    case apiDefs::ConfigType::ExternalPremium:
        return ServerConfigKind::ApiV2;
    default:
        break;
    }

    if (hasThirdPartyConfig(serverJson)) {
        return ServerConfigKind::Native;
    }

    const SelfHostedAdminServerConfig adminProbe = SelfHostedAdminServerConfig::fromJson(serverJson);
    return adminProbe.hasCredentials() ? ServerConfigKind::SelfHostedAdmin : ServerConfigKind::SelfHostedUser;
}

void SecureServersRepository::updateDefaultServerFromStorage()
{
    const QString storedDefaultId = value(QStringLiteral("Servers/defaultServerId"), QString()).toString();
    if (!storedDefaultId.isEmpty() && m_serverJsonById.contains(storedDefaultId)) {
        m_defaultServerId = storedDefaultId;
        return;
    }

    const int storedDefaultIndex = value("Servers/defaultServerIndex", 0).toInt();
    if (storedDefaultIndex >= 0 && storedDefaultIndex < m_orderedServerIds.size()) {
        m_defaultServerId = m_orderedServerIds.at(storedDefaultIndex);
        return;
    }

    if (!m_orderedServerIds.isEmpty()) {
        m_defaultServerId = m_orderedServerIds.first();
        return;
    }

    m_defaultServerId.clear();
}

void SecureServersRepository::persistDefaultServerFields()
{
    if (m_orderedServerIds.isEmpty()) {
        m_defaultServerId.clear();
    } else if (!m_orderedServerIds.contains(m_defaultServerId)) {
        m_defaultServerId = m_orderedServerIds.first();
    }

    setValue("Servers/defaultServerId", m_defaultServerId);
}

void SecureServersRepository::loadFromStorage()
{
    clearServerStateMaps();

    const QJsonArray serversArray =
            QJsonDocument::fromJson(value(QStringLiteral("Servers/serversList"), QByteArray()).toByteArray())
                    .array();

    for (int i = 0; i < serversArray.size(); ++i) {
        const QJsonObject json = serversArray.at(i).toObject();
        const QString candidateId = readStorageServerId(json);
        const QString serverId = normalizedOrGeneratedServerId(candidateId);
        const QJsonObject strippedJson = withoutStorageServerId(json);
        const ServerConfigKind kind = kindFromJson(strippedJson);

        if (m_serverJsonById.contains(serverId) || kind == ServerConfigKind::Invalid) {
            continue;
        }
        m_serverJsonById.insert(serverId, embedStorageServerId(serverId, strippedJson));
        m_orderedServerIds.append(serverId);
    }

    updateDefaultServerFromStorage();
}

void SecureServersRepository::syncToStorage()
{
    QJsonArray serversArray;

    for (const QString &serverId : m_orderedServerIds) {
        if (!m_serverJsonById.contains(serverId)) {
            continue;
        }
        serversArray.append(m_serverJsonById.value(serverId));
    }

    setValue("Servers/serversList", QJsonDocument(serversArray).toJson());
    persistDefaultServerFields();
}

void SecureServersRepository::invalidateCache()
{
    loadFromStorage();
}

void SecureServersRepository::clearServers()
{
    clearServerStateMaps();

    m_defaultServerId.clear();

    syncToStorage();
}

QString SecureServersRepository::addServer(const QString &serverId, const QJsonObject &serverJson, ServerConfigKind kind)
{
    const QString id = normalizedOrGeneratedServerId(serverId);
    if (m_serverJsonById.contains(id) || kind == ServerConfigKind::Invalid) {
        return id;
    }
    const QJsonObject strippedJson = withoutStorageServerId(serverJson);
    if (kindFromJson(strippedJson) != kind) {
        return id;
    }
    m_serverJsonById.insert(id, embedStorageServerId(id, strippedJson));

    m_orderedServerIds.append(id);

    if (m_defaultServerId.isEmpty()) {
        m_defaultServerId = id;
    }

    syncToStorage();
    emit serverAdded(id);
    return id;
}

void SecureServersRepository::editServer(const QString &serverId, const QJsonObject &serverJson, ServerConfigKind kind)
{
    if (indexOfServerId(serverId) < 0 || kind == ServerConfigKind::Invalid) {
        return;
    }
    if (!m_serverJsonById.contains(serverId)) {
        return;
    }

    const QJsonObject oldJson = m_serverJsonById.value(serverId);
    const ServerConfigKind oldKind = kindFromJson(withoutStorageServerId(oldJson));

    m_serverJsonById.remove(serverId);

    const QJsonObject strippedNew = withoutStorageServerId(serverJson);
    if (kindFromJson(strippedNew) != kind) {
        const QJsonObject strippedOld = withoutStorageServerId(oldJson);
        if (oldKind != ServerConfigKind::Invalid && kindFromJson(strippedOld) == oldKind) {
            m_serverJsonById.insert(serverId, embedStorageServerId(serverId, strippedOld));
        }
        return;
    }
    m_serverJsonById.insert(serverId, embedStorageServerId(serverId, strippedNew));

    syncToStorage();
    emit serverEdited(serverId);
}

void SecureServersRepository::removeServer(const QString &serverId)
{
    const int removedIndex = indexOfServerId(serverId);
    if (removedIndex < 0) {
        return;
    }
    if (!m_serverJsonById.contains(serverId)) {
        return;
    }

    const QString previousDefaultId = m_defaultServerId;
    const int previousDefaultIndex = defaultServerIndex();

    m_serverJsonById.remove(serverId);
    m_orderedServerIds.removeAt(removedIndex);

    if (m_orderedServerIds.isEmpty()) {
        m_defaultServerId.clear();
    } else if (m_defaultServerId == serverId) {
        const int fallbackIndex = qMin(removedIndex, m_orderedServerIds.size() - 1);
        m_defaultServerId = m_orderedServerIds.at(fallbackIndex);
    } else if (!m_orderedServerIds.contains(m_defaultServerId)) {
        m_defaultServerId = m_orderedServerIds.first();
    }

    const int newDefaultIndex = defaultServerIndex();
    if (previousDefaultId != m_defaultServerId || previousDefaultIndex != newDefaultIndex) {
        emit defaultServerChanged(m_defaultServerId);
    }

    syncToStorage();
    emit serverRemoved(serverId, removedIndex);
}

SecureServersRepository::ServerConfigKind SecureServersRepository::serverKind(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return ServerConfigKind::Invalid;
    }
    return kindFromJson(withoutStorageServerId(it.value()));
}

std::optional<SelfHostedAdminServerConfig> SecureServersRepository::selfHostedAdminConfig(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (kindFromJson(strippedJson) != ServerConfigKind::SelfHostedAdmin) {
        return std::nullopt;
    }
    return SelfHostedAdminServerConfig::fromJson(strippedJson);
}

std::optional<SelfHostedUserServerConfig> SecureServersRepository::selfHostedUserConfig(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (kindFromJson(strippedJson) != ServerConfigKind::SelfHostedUser) {
        return std::nullopt;
    }
    return SelfHostedUserServerConfig::fromJson(strippedJson);
}

std::optional<NativeServerConfig> SecureServersRepository::nativeConfig(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (kindFromJson(strippedJson) != ServerConfigKind::Native) {
        return std::nullopt;
    }
    return NativeServerConfig::fromJson(strippedJson);
}

std::optional<ApiV2ServerConfig> SecureServersRepository::apiV2Config(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (kindFromJson(strippedJson) != ServerConfigKind::ApiV2) {
        return std::nullopt;
    }
    return ApiV2ServerConfig::fromJson(strippedJson);
}

std::optional<LegacyApiServerConfig> SecureServersRepository::legacyApiConfig(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (kindFromJson(strippedJson) != ServerConfigKind::LegacyApiV1) {
        return std::nullopt;
    }
    return LegacyApiServerConfig::fromJson(strippedJson);
}

int SecureServersRepository::serversCount() const
{
    return m_orderedServerIds.size();
}

QString SecureServersRepository::serverIdAt(int index) const
{
    if (index < 0 || index >= m_orderedServerIds.size()) {
        return QString();
    }
    return m_orderedServerIds.at(index);
}

QVector<QString> SecureServersRepository::orderedServerIds() const
{
    return m_orderedServerIds;
}

int SecureServersRepository::indexOfServerId(const QString &serverId) const
{
    return m_orderedServerIds.indexOf(serverId);
}

int SecureServersRepository::defaultServerIndex() const
{
    if (m_orderedServerIds.isEmpty()) {
        return 0;
    }
    const int idx = m_orderedServerIds.indexOf(m_defaultServerId);
    return idx >= 0 ? idx : 0;
}

QString SecureServersRepository::defaultServerId() const
{
    return m_defaultServerId;
}

void SecureServersRepository::setDefaultServer(const QString &serverId)
{
    if (m_orderedServerIds.isEmpty()) {
        return;
    }
    if (!m_serverJsonById.contains(serverId)) {
        return;
    }

    if (indexOfServerId(serverId) < 0) {
        return;
    }

    if (m_defaultServerId == serverId) {
        return;
    }

    m_defaultServerId = serverId;
    persistDefaultServerFields();
    emit defaultServerChanged(m_defaultServerId);
}
