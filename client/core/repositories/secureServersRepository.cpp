#include "secureServersRepository.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSet>
#include <QUuid>

#include "core/utils/serverConfigUtils.h"
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

QString storedServerDisplayName(const SecureServersRepository *repository, const QString &serverId)
{
    using Kind = serverConfigUtils::ConfigType;
    switch (repository->serverKind(serverId)) {
    case Kind::SelfHostedAdmin:
        if (const auto cfg = repository->selfHostedAdminConfig(serverId)) {
            return cfg->displayName;
        }
        break;
    case Kind::SelfHostedUser:
        if (const auto cfg = repository->selfHostedUserConfig(serverId)) {
            return cfg->displayName;
        }
        break;
    case Kind::Native:
        if (const auto cfg = repository->nativeConfig(serverId)) {
            return cfg->displayName;
        }
        break;
    case Kind::AmneziaPremiumV2:
    case Kind::AmneziaFreeV3:
    case Kind::ExternalPremium:
        if (const auto cfg = repository->apiV2Config(serverId)) {
            return cfg->displayName;
        }
        break;
    case Kind::AmneziaPremiumV1:
    case Kind::AmneziaFreeV2:
        if (const auto cfg = repository->legacyApiConfig(serverId)) {
            return cfg->displayName;
        }
        break;
    case Kind::Invalid:
    default:
        break;
    }
    return {};
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
        const serverConfigUtils::ConfigType kind = serverConfigUtils::configTypeFromJson(strippedJson);

        if (m_serverJsonById.contains(serverId) || kind == serverConfigUtils::ConfigType::Invalid) {
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

QString SecureServersRepository::nextAvailableServerName() const
{
    QSet<QString> usedNames;
    usedNames.reserve(m_orderedServerIds.size());

    for (const QString &serverId : m_orderedServerIds) {
        const QString displayName = storedServerDisplayName(this, serverId);
        if (!displayName.isEmpty()) {
            usedNames.insert(displayName);
        }
    }

    int i = 0;
    QString candidate;
    do {
        ++i;
        candidate = tr("Server") + QLatin1Char(' ') + QString::number(i);
    } while (usedNames.contains(candidate));

    return candidate;
}

QString SecureServersRepository::addServer(const QString &serverId, const QJsonObject &serverJson, serverConfigUtils::ConfigType kind)
{
    const QString id = normalizedOrGeneratedServerId(serverId);
    if (m_serverJsonById.contains(id) || kind == serverConfigUtils::ConfigType::Invalid) {
        return id;
    }
    const QJsonObject strippedJson = withoutStorageServerId(serverJson);
    if (serverConfigUtils::configTypeFromJson(strippedJson) != kind) {
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

void SecureServersRepository::editServer(const QString &serverId, const QJsonObject &serverJson, serverConfigUtils::ConfigType kind)
{
    if (indexOfServerId(serverId) < 0 || kind == serverConfigUtils::ConfigType::Invalid) {
        return;
    }
    if (!m_serverJsonById.contains(serverId)) {
        return;
    }

    const QJsonObject oldJson = m_serverJsonById.value(serverId);
    const serverConfigUtils::ConfigType oldKind = serverConfigUtils::configTypeFromJson(withoutStorageServerId(oldJson));

    m_serverJsonById.remove(serverId);

    const QJsonObject strippedNew = withoutStorageServerId(serverJson);
    if (serverConfigUtils::configTypeFromJson(strippedNew) != kind) {
        const QJsonObject strippedOld = withoutStorageServerId(oldJson);
        if (oldKind != serverConfigUtils::ConfigType::Invalid && serverConfigUtils::configTypeFromJson(strippedOld) == oldKind) {
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

serverConfigUtils::ConfigType SecureServersRepository::serverKind(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return serverConfigUtils::ConfigType::Invalid;
    }
    return serverConfigUtils::configTypeFromJson(withoutStorageServerId(it.value()));
}

std::optional<SelfHostedAdminServerConfig> SecureServersRepository::selfHostedAdminConfig(const QString &serverId) const
{
    const auto it = m_serverJsonById.constFind(serverId);
    if (it == m_serverJsonById.constEnd()) {
        return std::nullopt;
    }
    const QJsonObject strippedJson = withoutStorageServerId(it.value());
    if (serverConfigUtils::configTypeFromJson(strippedJson) != serverConfigUtils::ConfigType::SelfHostedAdmin) {
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
    if (serverConfigUtils::configTypeFromJson(strippedJson) != serverConfigUtils::ConfigType::SelfHostedUser) {
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
    if (serverConfigUtils::configTypeFromJson(strippedJson) != serverConfigUtils::ConfigType::Native) {
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
    if (!serverConfigUtils::isApiV2Subscription(serverConfigUtils::configTypeFromJson(strippedJson))) {
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
    if (!serverConfigUtils::isLegacyApiSubscription(serverConfigUtils::configTypeFromJson(strippedJson))) {
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
