#include "proxyConfigManager.h"

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/containers/containerUtils.h"
#include "localProxyDefs.h"
#include "portAvailabilityHelper.h"
#include "version.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSysInfo>
#include <QUuid>

using namespace amnezia;

namespace
{
    std::nullopt_t fail(QString &errorDescription, const QString &message)
    {
        qWarning() << message;
        errorDescription = message;
        return std::nullopt;
    }
}

ProxyConfigManager::ProxyConfigManager(SecureServersRepository *serversRepository, SecureAppSettingsRepository *appSettingsRepository)
    : m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

std::optional<ProxyConfigManager::ConfigData> ProxyConfigManager::buildConfig(QString &errorDescription) const
{
    errorDescription.clear();

    if (!m_serversRepository || !m_appSettingsRepository) {
        return fail(errorDescription, QStringLiteral("Local proxy repositories are not available"));
    }

    const QString ownerId = m_appSettingsRepository->localProxyOwnerId();
    if (ownerId.isEmpty()) {
        return fail(errorDescription, QStringLiteral("Local proxy owner server id is not configured"));
    }

    const auto ownerServer = m_serversRepository->serverJsonById(ownerId);
    if (!ownerServer) {
        return fail(errorDescription, QStringLiteral("Owner server with id %1 not found").arg(ownerId));
    }

    if (!apiUtils::isPremiumServer(*ownerServer)) {
        return fail(errorDescription,
                    QStringLiteral("Server %1 is not premium, local proxy is unavailable")
                            .arg(ownerServer->value(configKey::name).toString()));
    }

    auto serializedConfig = extractSerializedXrayConfig(*ownerServer);
    if (!serializedConfig || serializedConfig->isEmpty()) {
        serializedConfig = fetchSerializedXrayConfigFromGateway(*ownerServer, errorDescription);
        if (!serializedConfig || serializedConfig->isEmpty()) {
            return std::nullopt;
        }
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(serializedConfig->toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return fail(errorDescription, QStringLiteral("Failed to parse Xray config JSON: %1").arg(parseError.errorString()));
    }

    const auto proxyPort = selectProxyPort(errorDescription);
    if (!proxyPort) {
        return std::nullopt;
    }

    ConfigData data;
    data.parsedConfig = doc.object();
    if (applyProxyPortToConfig(data.parsedConfig, *proxyPort)) {
        data.serializedConfig = QString::fromUtf8(QJsonDocument(data.parsedConfig).toJson(QJsonDocument::Compact));
        data.proxyPort = *proxyPort;
    } else {
        qWarning() << "Failed to override local proxy inbound port, using original config";
        data.serializedConfig = *serializedConfig;
        data.proxyPort = data.parsedConfig.value(QLatin1String("inbounds")).toArray().at(0).toObject().value(QLatin1String("port")).toInt();
    }

    return data;
}

std::optional<int> ProxyConfigManager::selectProxyPort(QString &errorDescription) const
{
    int port = m_appSettingsRepository->localProxyPort();
    if (port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        port = localProxy::defaultProxyPort;
    }

    if (PortAvailabilityHelper::waitForPort(port, localProxy::portReleaseWaitMs)) {
        return port;
    }

    if (m_appSettingsRepository->isLocalProxyPortUserDefined()) {
        return fail(errorDescription, QStringLiteral("Local proxy port %1 is already in use").arg(port));
    }

    std::optional<int> freePort;
    if (port != localProxy::defaultProxyPort && PortAvailabilityHelper::isPortAvailable(localProxy::defaultProxyPort)) {
        freePort = localProxy::defaultProxyPort;
    } else {
        freePort = PortAvailabilityHelper::findFirstAvailablePort(localProxy::defaultProxyPort + 1, localProxy::proxyPortMax);
    }

    if (!freePort) {
        return fail(errorDescription,
                    QStringLiteral("No available local proxy port in range %1-%2")
                            .arg(localProxy::defaultProxyPort + 1)
                            .arg(localProxy::proxyPortMax));
    }

    qDebug() << "Local proxy port" << port << "is busy, using" << *freePort;
    return freePort;
}

bool ProxyConfigManager::applyProxyPortToConfig(QJsonObject &config, int port) const
{
    if (!config.value(QLatin1String("inbounds")).isArray()) {
        return false;
    }

    QJsonArray inbounds = config.value(QLatin1String("inbounds")).toArray();
    if (inbounds.isEmpty() || !inbounds.at(0).isObject()) {
        return false;
    }

    QJsonObject firstInbound = inbounds.at(0).toObject();
    firstInbound.insert(QLatin1String("port"), port);
    inbounds[0] = firstInbound;
    config.insert(QLatin1String("inbounds"), inbounds);
    return true;
}

std::optional<QString> ProxyConfigManager::extractSerializedXrayConfig(const QJsonObject &server) const
{
    const QJsonArray containers = server.value(configKey::containers).toArray();
    const QString targetContainer = ContainerUtils::containerToString(DockerContainer::Xray);

    for (const QJsonValue &value : containers) {
        const QJsonObject container = value.toObject();
        if (container.value(configKey::container).toString() != targetContainer) {
            continue;
        }

        const QJsonObject proto = container.value(QString(configKey::xray)).toObject();
        const QString serialized = proto.value(configKey::lastConfig).toString();
        if (!serialized.isEmpty()) {
            return serialized;
        }
    }

    return std::nullopt;
}

std::optional<QString> ProxyConfigManager::fetchSerializedXrayConfigFromGateway(const QJsonObject &server, QString &errorDescription) const
{
    const QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
    if (apiConfig.isEmpty()) {
        return fail(errorDescription, QStringLiteral("Server API config is missing"));
    }

    const QString userCountryCode = apiConfig.value(apiDefs::key::userCountryCode).toString();
    const QString serviceType = apiConfig.value(apiDefs::key::serviceType).toString();
    if (userCountryCode.isEmpty() || serviceType.isEmpty()) {
        return fail(errorDescription, QStringLiteral("Server API config lacks service identifiers"));
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
    apiPayload[apiDefs::key::serviceProtocol] = QStringLiteral("vless");
    apiPayload[apiDefs::key::publicKey] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QJsonObject authData = server.value(apiDefs::key::authData).toObject();
    if (!authData.isEmpty()) {
        apiPayload[apiDefs::key::authData] = authData;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled(),
                                        m_appSettingsRepository);

    QByteArray responseBody;
    const ErrorCode errorCode = gatewayController.post(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return fail(errorDescription, QStringLiteral("Gateway request failed with error code %1").arg(static_cast<int>(errorCode)));
    }

    QJsonParseError responseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseBody, &responseError);
    if (responseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        return fail(errorDescription, QStringLiteral("Failed to parse gateway response: %1").arg(responseError.errorString()));
    }

    QString data = responseDoc.object().value(configKey::config).toString();
    if (data.isEmpty()) {
        return fail(errorDescription, QStringLiteral("Gateway response lacks config payload"));
    }

    data.replace("vpn://", "");
    QByteArray decoded = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    if (decoded.isEmpty()) {
        return fail(errorDescription, QStringLiteral("Gateway config payload is empty"));
    }

    const QByteArray uncompressed = qUncompress(decoded);
    if (!uncompressed.isEmpty()) {
        decoded = uncompressed;
    }

    QJsonParseError configError;
    const QJsonDocument configDoc = QJsonDocument::fromJson(decoded, &configError);
    if (configError.error != QJsonParseError::NoError || !configDoc.isObject()) {
        return fail(errorDescription, QStringLiteral("Failed to parse gateway config JSON: %1").arg(configError.errorString()));
    }

    const auto serializedConfig = extractSerializedXrayConfig(configDoc.object());
    if (!serializedConfig || serializedConfig->isEmpty()) {
        return fail(errorDescription, QStringLiteral("Gateway response lacks Xray last_config payload"));
    }

    qDebug() << "Fetched Xray config from gateway";
    return serializedConfig;
}
