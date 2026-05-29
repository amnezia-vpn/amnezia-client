#include "configmanager.h"

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/containers/containerUtils.h"
#include "portavailabilityhelper.h"
#include "proxylogger.h"
#include "version.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include <QSysInfo>
#include <QStandardPaths>
#include <QUuid>

using namespace amnezia;

ConfigManager::ConfigManager(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository)
    : m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    ProxyLogger::getInstance().debug("ConfigManager initialized (repository-backed)");
}

namespace {
namespace gateway_key {
constexpr char vless[] = "vless";
} // namespace gateway_key

constexpr quint16 kDefaultProxyPort = 10808;
constexpr int kProxyPortMin = 1024;
constexpr int kProxyPortMax = 65535;

int resolveProxyPort(SecureAppSettingsRepository *appSettings)
{
    if (!appSettings) {
        return kDefaultProxyPort;
    }

    const quint16 port = appSettings->localProxyPort();
    if (port < kProxyPortMin || port > kProxyPortMax) {
        return kDefaultProxyPort;
    }

    return static_cast<int>(port);
}

} // namespace

bool ConfigManager::applyProxyPortToConfig(QJsonObject &config, int port) const
{
    if (!config.contains("inbounds") || !config.value("inbounds").isArray()) {
        return false;
    }

    QJsonArray inbounds = config.value("inbounds").toArray();
    if (inbounds.isEmpty() || !inbounds.at(0).isObject()) {
        return false;
    }

    QJsonObject firstInbound = inbounds.at(0).toObject();
    firstInbound.insert("port", port);
    inbounds[0] = firstInbound;
    config.insert("inbounds", inbounds);
    return true;
}

QString ConfigManager::serializeConfig(const QJsonObject &config) const
{
    return QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact));
}

std::optional<ConfigManager::ConfigData> ConfigManager::buildConfig(QString &errorDescription) const
{
    errorDescription.clear();

    if (!m_serversRepository || !m_appSettingsRepository) {
        const QString message = QStringLiteral("Local proxy repositories are not available");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    const QString ownerId = m_appSettingsRepository->localProxyOwnerId();
    if (ownerId.isEmpty()) {
        const QString message = QStringLiteral("Local proxy owner server id is not configured");
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    const auto ownerServer = m_serversRepository->serverJsonById(ownerId);
    if (!ownerServer) {
        const QString message = QStringLiteral("Owner server with id %1 not found").arg(ownerId);
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    if (!apiUtils::isPremiumServer(*ownerServer)) {
        const QString message = QStringLiteral("Server %1 is not premium, local proxy is unavailable")
                                        .arg(ownerServer->value(configKey::name).toString());
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    const auto serializedConfig = extractSerializedXrayConfig(*ownerServer);
    if (!serializedConfig || serializedConfig->isEmpty()) {
        const QString message = QStringLiteral("Server %1 lacks Xray last_config payload")
                                        .arg(ownerServer->value(configKey::name).toString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(serializedConfig->toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString message = QStringLiteral("Failed to parse Xray config JSON: %1").arg(parseError.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    ConfigData data;
    data.ownerId = ownerId;
    data.serverName = ownerServer->value(configKey::name).toString();
    data.parsedConfig = doc.object();
    const int proxyPort = resolveProxyPort(m_appSettingsRepository);
    if (applyProxyPortToConfig(data.parsedConfig, proxyPort)) {
        data.serializedConfig = serializeConfig(data.parsedConfig);
    } else {
        ProxyLogger::getInstance().warning(QStringLiteral("Failed to override local proxy inbound port; using original config"));
        data.serializedConfig = *serializedConfig;
    }

    return data;
}

std::optional<ConfigManager::ConfigData> ConfigManager::buildConfigWithFetch(QString &errorDescription) const
{
    errorDescription.clear();

    if (!m_serversRepository || !m_appSettingsRepository) {
        const QString message = QStringLiteral("Local proxy repositories are not available");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    const QString ownerId = m_appSettingsRepository->localProxyOwnerId();
    if (ownerId.isEmpty()) {
        const QString message = QStringLiteral("Local proxy owner server id is not configured");
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    const auto ownerServer = m_serversRepository->serverJsonById(ownerId);
    if (!ownerServer) {
        const QString message = QStringLiteral("Owner server with id %1 not found").arg(ownerId);
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    if (!apiUtils::isPremiumServer(*ownerServer)) {
        const QString message = QStringLiteral("Server %1 is not premium, local proxy is unavailable")
                                        .arg(ownerServer->value(configKey::name).toString());
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    auto serializedConfig = extractSerializedXrayConfig(*ownerServer);
    if (!serializedConfig || serializedConfig->isEmpty()) {
        auto fetchedConfig = fetchSerializedXrayConfigFromGateway(*ownerServer, errorDescription);
        if (!fetchedConfig || fetchedConfig->isEmpty()) {
            return std::nullopt;
        }
        serializedConfig = fetchedConfig;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(serializedConfig->toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString message = QStringLiteral("Failed to parse Xray config JSON: %1").arg(parseError.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    ConfigData data;
    data.ownerId = ownerId;
    data.serverName = ownerServer->value(configKey::name).toString();
    data.parsedConfig = doc.object();

    int selectedPort = resolveProxyPort(m_appSettingsRepository);
    const bool isUserDefinedPort = m_appSettingsRepository->isLocalProxyPortUserDefined();

    if (!PortAvailabilityHelper::isPortAvailable(selectedPort)) {
        const bool canAutoSelect = !isUserDefinedPort && selectedPort == kDefaultProxyPort;
        if (canAutoSelect) {
            const auto freePort = PortAvailabilityHelper::findFirstAvailablePort(kDefaultProxyPort + 1, kProxyPortMax);
            if (!freePort) {
                errorDescription = QStringLiteral("No available local proxy port in range %1-%2")
                                       .arg(kDefaultProxyPort + 1)
                                       .arg(kProxyPortMax);
                ProxyLogger::getInstance().error(errorDescription);
                return std::nullopt;
            }
            selectedPort = *freePort;
        } else {
            errorDescription = QStringLiteral("Local proxy port %1 is already in use")
                                   .arg(selectedPort);
            ProxyLogger::getInstance().error(errorDescription);
            return std::nullopt;
        }
    }

    if (applyProxyPortToConfig(data.parsedConfig, selectedPort)) {
        data.serializedConfig = serializeConfig(data.parsedConfig);
    } else {
        ProxyLogger::getInstance().warning(QStringLiteral("Failed to override local proxy inbound port; using original config"));
        data.serializedConfig = *serializedConfig;
    }

    return data;
}

bool ConfigManager::writeTempConfig(const QString &serializedConfig, QString &configPath, QString &errorDescription) const
{
    errorDescription.clear();
    configPath.clear();

    const QString directory = tempDirectory();
    if (!QDir().mkpath(directory)) {
        const QString message = QStringLiteral("Failed to create temp config directory: %1").arg(directory);
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return false;
    }

    const QString path = tempConfigPath();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString message = QStringLiteral("Failed to open temp config file %1: %2").arg(path, file.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return false;
    }

    if (file.write(serializedConfig.toUtf8()) == -1) {
        const QString message = QStringLiteral("Failed to write temp config file %1: %2").arg(path, file.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return false;
    }

    if (!file.commit()) {
        const QString message = QStringLiteral("Failed to commit temp config file %1").arg(path);
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return false;
    }

    ProxyLogger::getInstance().info(QStringLiteral("Xray config saved to %1").arg(path));
    configPath = path;
    return true;
}

bool ConfigManager::removeTempConfig() const
{
    const QString path = tempConfigPath();
    QFile file(path);
    if (!file.exists()) {
        return true;
    }

    if (!file.remove()) {
        ProxyLogger::getInstance().warning(QStringLiteral("Failed to remove temp config file %1: %2").arg(path, file.errorString()));
        return false;
    }

    ProxyLogger::getInstance().debug(QStringLiteral("Removed temp config file %1").arg(path));
    return true;
}

QString ConfigManager::tempConfigPath() const
{
    return QDir(tempDirectory()).filePath(QStringLiteral("xray_active.json"));
}

std::optional<QString> ConfigManager::extractSerializedXrayConfig(const QJsonObject &server) const
{
    const QJsonArray containers = server.value(configKey::containers).toArray();
    const QString targetContainer = ContainerUtils::containerToString(DockerContainer::Xray);
    const QString protoKey = QString(configKey::xray);

    for (const QJsonValue &value : containers) {
        const QJsonObject container = value.toObject();
        if (container.value(configKey::container).toString() != targetContainer) {
            continue;
        }

        const QJsonObject proto = container.value(protoKey).toObject();
        const QString serialized = proto.value(configKey::lastConfig).toString();
        if (!serialized.isEmpty()) {
            return serialized;
        }
    }

    return std::nullopt;
}

std::optional<QString> ConfigManager::fetchSerializedXrayConfigFromGateway(const QJsonObject &server, QString &errorDescription) const
{
    errorDescription.clear();

    if (!m_appSettingsRepository) {
        const QString message = QStringLiteral("App settings repository is not available");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
    if (apiConfig.isEmpty()) {
        const QString message = QStringLiteral("Server API config is missing");
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    const QString userCountryCode = apiConfig.value(apiDefs::key::userCountryCode).toString();
    const QString serviceType = apiConfig.value(apiDefs::key::serviceType).toString();
    if (userCountryCode.isEmpty() || serviceType.isEmpty()) {
        const QString message = QStringLiteral("Server API config lacks service identifiers");
        ProxyLogger::getInstance().warning(message);
        errorDescription = message;
        return std::nullopt;
    }

    QJsonObject apiPayload;
    apiPayload[apiDefs::key::osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::appVersion] = QString(APP_VERSION);

    const QString appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    if (!appLanguage.isEmpty()) {
        apiPayload[apiDefs::key::appLanguage] = appLanguage;
    }

    apiPayload[apiDefs::key::uuid] = m_appSettingsRepository->getInstallationUuid(true);
    apiPayload[apiDefs::key::userCountryCode] = userCountryCode;
    apiPayload[apiDefs::key::serviceType] = serviceType;
    apiPayload[apiDefs::key::serviceProtocol] = QString(gateway_key::vless);
    apiPayload[apiDefs::key::publicKey] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QJsonObject authData = server.value(apiDefs::key::authData).toObject();
    if (!authData.isEmpty()) {
        apiPayload[apiDefs::key::authData] = authData;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled());

    QByteArray responseBody;
    const amnezia::ErrorCode errorCode = gatewayController.post(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != amnezia::ErrorCode::NoError) {
        const QString message = QStringLiteral("Gateway request failed with error code %1").arg(static_cast<int>(errorCode));
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    QJsonParseError responseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseBody, &responseError);
    if (responseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        const QString message = QStringLiteral("Failed to parse gateway response: %1").arg(responseError.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    QString data = responseDoc.object().value(configKey::config).toString();
    if (data.isEmpty()) {
        const QString message = QStringLiteral("Gateway response lacks config payload");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    data.replace("vpn://", "");
    QByteArray decoded = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    if (decoded.isEmpty()) {
        const QString message = QStringLiteral("Gateway config payload is empty");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    const QByteArray uncompressed = qUncompress(decoded);
    if (!uncompressed.isEmpty()) {
        decoded = uncompressed;
    }

    QJsonParseError configError;
    const QJsonDocument configDoc = QJsonDocument::fromJson(decoded, &configError);
    if (configError.error != QJsonParseError::NoError || !configDoc.isObject()) {
        const QString message = QStringLiteral("Failed to parse gateway config JSON: %1").arg(configError.errorString());
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    const auto serializedConfig = extractSerializedXrayConfig(configDoc.object());
    if (!serializedConfig || serializedConfig->isEmpty()) {
        const QString message = QStringLiteral("Gateway response lacks Xray last_config payload");
        ProxyLogger::getInstance().error(message);
        errorDescription = message;
        return std::nullopt;
    }

    ProxyLogger::getInstance().info("Fetched Xray config from gateway");
    return serializedConfig;
}

QString ConfigManager::tempDirectory() const
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (baseDir.isEmpty()) {
        return QDir::temp().filePath(QStringLiteral("amnezia_local_proxy"));
    }
    return QDir(baseDir).filePath(QStringLiteral("local_proxy"));
}
