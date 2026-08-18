#include "subscriptionController.h"

#include <QDebug>
#include <QDateTime>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPromise>
#include <QSet>
#include <QUuid>
#include <QVariantMap>

#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/api/gatewayPayloadBuilder.h"
#include "core/controllers/gatewayController.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "version.h"
#include "core/models/containerConfig.h"
#include "core/models/api/apiConfig.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include "core/utils/swiftBridge.h"
#endif

using namespace amnezia;

namespace
{
QString getSubscriptionStatusForRenewal(const ApiConfig &apiConfig)
{
    if (apiConfig.subscriptionExpiredByServer) {
        return QStringLiteral("expired");
    }

    if (!apiConfig.subscription.endDate.isEmpty()) {
        if (apiUtils::isSubscriptionExpired(apiConfig.subscription.endDate)) {
            return QStringLiteral("expired");
        }
        if (apiUtils::isSubscriptionExpiringSoon(apiConfig.subscription.endDate)) {
            return QStringLiteral("expire_soon");
        }
    }

    return QStringLiteral("active");
}

QString normalizeCaptchaSolution(const QString &captchaSolution)
{
    QString normalizedSolution;
    normalizedSolution.reserve(captchaSolution.size());
    for (const QChar &ch : captchaSolution) {
        const ushort u = ch.unicode();
        if (u >= '0' && u <= '9') {
            normalizedSolution += ch;
        } else if (u >= 0xFF10 && u <= 0xFF19) {
            normalizedSolution += QChar(static_cast<char16_t>(u - 0xFF10 + '0'));
        }
    }
    return normalizedSolution.isEmpty() ? captchaSolution.trimmed() : normalizedSolution;
}

bool fillCaptchaInfoFromResponse(const QByteArray &responseBody, SubscriptionController::CaptchaInfo &captchaInfo)
{
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(responseBody);
    if (!jsonDoc.isObject()) {
        return false;
    }
    const QJsonObject jsonObj = jsonDoc.object();
    if (!jsonObj.contains(QStringLiteral("captcha_id")) || !jsonObj.contains(QStringLiteral("captcha_image"))) {
        return false;
    }
    captchaInfo.captchaId = jsonObj.value(QStringLiteral("captcha_id")).toString();
    captchaInfo.captchaImageBase64 = jsonObj.value(QStringLiteral("captcha_image")).toString();
    captchaInfo.hint = jsonObj.value(QStringLiteral("hint")).toString();
    captchaInfo.isRequired = true;
    return true;
}
}


SubscriptionController::SubscriptionController(SecureServersRepository* serversRepository,
                                               SecureAppSettingsRepository* appSettingsRepository)
    : m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

SubscriptionController::ProtocolData SubscriptionController::generateProtocolData(const QString &protocol)
{
    ProtocolData protocolData;
    if (protocol == configKey::awg) {
        auto connData = WireguardConfigurator::genClientKeys();
        protocolData.wireGuardClientPubKey = connData.clientPubKey;
        protocolData.wireGuardClientPrivKey = connData.clientPrivKey;
    } else if (protocol == configKey::vless) {
        protocolData.xrayUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }

    return protocolData;
}

QString SubscriptionController::publicKeyForProtocol(const QString &protocol, const ProtocolData &protocolData)
{
    if (protocol == configKey::awg) {
        return protocolData.wireGuardClientPubKey;
    }
    if (protocol == configKey::vless) {
        return protocolData.xrayUuid;
    }
    return {};
}

ErrorCode SubscriptionController::extractServerConfigJsonFromResponse(const QByteArray &apiResponseBody, const QString &protocol, 
                                                                        const ProtocolData &protocolData, QJsonObject &serverConfigJson)
{
    QString data = QJsonDocument::fromJson(apiResponseBody).object().value(configKey::config).toString();

    data.replace("vpn://", "");
    QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    if (ba.isEmpty()) {
        qDebug() << "empty vpn key";
        return ErrorCode::ApiConfigEmptyError;
    }

    QByteArray ba_uncompressed = qUncompress(ba);
    if (!ba_uncompressed.isEmpty()) {
        ba = ba_uncompressed;
    }

    QString configStr = ba;
    if (protocol == configKey::awg) {
        configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);
        auto newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
        auto containers = newServerConfig.value(configKey::containers).toArray();
        if (containers.isEmpty()) {
            qDebug() << "missing containers field";
            return ErrorCode::ApiConfigEmptyError;
        }
        auto container = containers.at(0).toObject();
        QString containerName = ContainerUtils::containerTypeToString(DockerContainer::Awg);
        auto serverProtocolConfig = container.value(containerName).toObject();
        auto clientProtocolConfig =
                QJsonDocument::fromJson(serverProtocolConfig.value(configKey::lastConfig).toString().toUtf8()).object();

        // TODO looks like this block can be removed after v1 configs EOL

        const QStringList awgProtocolKeys = configKey::awgProtocolKeys();

        for (const QString &key : awgProtocolKeys) {
            const QJsonValue value = clientProtocolConfig.value(key);
            if (value.isString() && !value.toString().isEmpty()) {
                serverProtocolConfig[key] = value;
            }
        }

        //

        container[containerName] = serverProtocolConfig;
        containers.replace(0, container);
        newServerConfig[configKey::containers] = containers;
        configStr = QString(QJsonDocument(newServerConfig).toJson());
    }

    serverConfigJson = QJsonDocument::fromJson(configStr.toUtf8()).object();
    return ErrorCode::NoError;
}

void SubscriptionController::updateApiConfigInJson(QJsonObject &serverConfigJson, const QString &serviceType, 
                                                    const QString &serviceProtocol, const QString &userCountryCode,
                                                    const QByteArray &apiResponseBody)
{
    QJsonObject apiConfig = serverConfigJson.value(apiDefs::key::apiConfig).toObject();
    
    apiConfig[apiDefs::key::serviceType] = serviceType;
    apiConfig[apiDefs::key::serviceProtocol] = serviceProtocol;
    apiConfig[apiDefs::key::userCountryCode] = userCountryCode;
    
    if (serverConfigJson.value(configKey::configVersion).toInt() == serverConfigUtils::ConfigSource::AmneziaGateway) {
        QJsonObject responseObj = QJsonDocument::fromJson(apiResponseBody).object();
        if (responseObj.contains(apiDefs::key::serviceInfo)) {
            apiConfig.insert(apiDefs::key::serviceInfo, responseObj.value(apiDefs::key::serviceInfo).toObject());
        }
    }
    
    serverConfigJson[apiDefs::key::apiConfig] = apiConfig;
}

ErrorCode SubscriptionController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase), m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled(), m_appSettingsRepository);
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

ErrorCode SubscriptionController::importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            CaptchaInfo &captchaInfo)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);

    if (errorCode == ErrorCode::ApiCaptchaRequiredError) {
        fillCaptchaInfoFromResponse(responseBody, captchaInfo);
        return errorCode;
    }

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    updateApiConfigInJson(serverConfigJson, serviceType, serviceProtocol, userCountryCode, responseBody);

    if (serverConfigJson.value(configKey::configVersion).toInt() != serverConfigUtils::ConfigSource::AmneziaGateway) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig apiV2ServerConfig = ApiV2ServerConfig::fromJson(serverConfigJson);
    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::importTrialFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                         const QString &serviceProtocol, const QString &email)
{
    const QString trimmedEmail = email.trimmed();
    if (trimmedEmail.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    ProtocolData protocolData = generateProtocolData(serviceProtocol);

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::email, trimmedEmail)
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/trial"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject responseObject = QJsonDocument::fromJson(responseBody).object();
    QString key = responseObject.value(apiDefs::key::config).toString();
    if (key.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    key.replace(QStringLiteral("vpn://"), QString());
    QByteArray configBytes = QByteArray::fromBase64(key.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray uncompressed = qUncompress(configBytes);
    if (!uncompressed.isEmpty()) {
        configBytes = uncompressed;
    }

    if (configBytes.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(configBytes).object();
    if (configObject.value(configKey::configVersion).toInt() != serverConfigUtils::ConfigSource::AmneziaGateway) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig apiV2ServerConfig = ApiV2ServerConfig::fromJson(configObject);
    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            const QString &transactionId, bool isTestPurchase,
                                                            int *duplicateServerIndex)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::transactionId, transactionId)
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/subscriptions"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    // Parse the subscription response
    QJsonObject responseObject = QJsonDocument::fromJson(responseBody).object();
    QString key = responseObject.value(QStringLiteral("key")).toString();
    if (key.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response does not contain a key field";
        return ErrorCode::ApiPurchaseError;
    }

    QString normalizedKey = key;
    normalizedKey.replace(QStringLiteral("vpn://"), QString());

    // Check if server with this VPN key already exists
    for (int i = 0; i < m_serversRepository->serversCount(); ++i) {
        const auto apiV2 = m_serversRepository->apiV2Config(m_serversRepository->serverIdAt(i));
        QString existingVpnKey = apiV2.has_value() ? apiV2->vpnKey() : QString();
        existingVpnKey.replace(QStringLiteral("vpn://"), QString());
        if (!existingVpnKey.isEmpty() && existingVpnKey == normalizedKey) {
            if (duplicateServerIndex) {
                *duplicateServerIndex = i;
            }
            qInfo().noquote() << "[IAP] Subscription config with the same vpn_key already exists";
            return ErrorCode::ApiConfigAlreadyAdded;
        }
    }

    QByteArray configString = QByteArray::fromBase64(normalizedKey.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    QByteArray configUncompressed = qUncompress(configString);
    if (!configUncompressed.isEmpty()) {
        configString = configUncompressed;
    }

    if (configString.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response config payload is empty";
        return ErrorCode::ApiPurchaseError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(configString).object();

    quint16 crc = qChecksum(QJsonDocument(configObject).toJson());
    
    if (configObject.value(configKey::configVersion).toInt() != serverConfigUtils::ConfigSource::AmneziaGateway) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig apiV2ServerConfig = ApiV2ServerConfig::fromJson(configObject);
    ApiV2ServerConfig* apiV2 = &apiV2ServerConfig;
    apiV2->apiConfig.vpnKey = normalizedKey;
    apiV2->apiConfig.subscriptionExpiredByServer = false;
    apiV2->crc = crc;

    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, bool isConnectEvent,
                                                           CaptchaInfo *captchaInfoOut, ProtocolData *usedProtocolDataOut)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QString serviceProtocol = apiV2->serviceProtocol();

    if (!newCountryCode.isEmpty()) {
        const auto availableCountries = apiV2->apiConfig.availableCountries;
        for (const auto &country : availableCountries) {
            const auto countryObject = country.toObject();
            if (countryObject.value(apiDefs::key::serverCountryCode).toString() != newCountryCode) {
                continue;
            }

            const auto availableProtocols = countryObject.value(apiDefs::key::availableProtocols).toArray();
            if (!availableProtocols.isEmpty() && !availableProtocols.contains(serviceProtocol)) {
                serviceProtocol = availableProtocols.first().toString();
            }
            break;
        }
    }

    ProtocolData protocolData = generateProtocolData(serviceProtocol);

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, newCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .addField(apiDefs::key::isConnectEvent, isConnectEvent ? QJsonValue(true) : QJsonValue())
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfoOut) {
            if (fillCaptchaInfoFromResponse(responseBody, *captchaInfoOut) && usedProtocolDataOut) {
                *usedProtocolDataOut = protocolData;
            }
        }
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError && !apiV2->apiConfig.isInAppPurchase) {
            ApiV2ServerConfig expiredApiV2 = *apiV2;
            expiredApiV2.apiConfig.subscriptionExpiredByServer = true;
            m_serversRepository->editServer(serverId, expiredApiV2.toJson(),
                                           serverConfigUtils::configTypeFromJson(expiredApiV2.toJson()));
        }
        return errorCode;
    }

    return applyUpdatedServiceConfig(serverId, serviceProtocol, protocolData, responseBody);
}

ErrorCode SubscriptionController::applyUpdatedServiceConfig(const QString &serverId, const QString &serviceProtocol,
                                                           const ProtocolData &protocolData, const QByteArray &responseBody)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }

    QJsonObject serverConfigJson;
    ErrorCode errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    updateApiConfigInJson(serverConfigJson, apiV2->apiConfig.serviceType, serviceProtocol, apiV2->apiConfig.userCountryCode, responseBody);

    if (serverConfigJson.value(configKey::configVersion).toInt() != serverConfigUtils::ConfigSource::AmneziaGateway) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig newApiV2Config = ApiV2ServerConfig::fromJson(serverConfigJson);
    ApiV2ServerConfig* newApiV2 = &newApiV2Config;

    newApiV2->apiConfig.vpnKey = apiV2->apiConfig.vpnKey;
    newApiV2->apiConfig.subscriptionExpiredByServer = false;

    newApiV2->authData = apiV2->authData;
    newApiV2->crc = apiV2->crc;

    if (apiV2->nameOverriddenByUser) {
        newApiV2->name = apiV2->name;
        newApiV2->displayName = apiV2->displayName;
        newApiV2->nameOverriddenByUser = true;
    }

    m_serversRepository->editServer(serverId, newApiV2Config.toJson(),
                                   serverConfigUtils::configTypeFromJson(newApiV2Config.toJson()));
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::resolveUpdateServiceCaptcha(const QString &serverId, const QString &newCountryCode,
                                                              bool isConnectEvent, const ProtocolData &protocolData,
                                                              const QString &captchaId, const QString &captchaSolution,
                                                              CaptchaInfo *retryCaptchaOut)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QString serviceProtocol = apiV2->serviceProtocol();

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, newCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .addField(apiDefs::key::captchaId, captchaId)
                                     .addField(apiDefs::key::captchaSolution, normalizeCaptchaSolution(captchaSolution))
                                     .addField(apiDefs::key::isConnectEvent, isConnectEvent ? QJsonValue(true) : QJsonValue())
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        if (retryCaptchaOut
            && (errorCode == ErrorCode::ApiCaptchaInvalidError || errorCode == ErrorCode::ApiCaptchaRefreshError
                || errorCode == ErrorCode::ApiCaptchaRequiredError)) {
            fillCaptchaInfoFromResponse(responseBody, *retryCaptchaOut);
        }
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError && !apiV2->apiConfig.isInAppPurchase) {
            ApiV2ServerConfig expiredApiV2 = *apiV2;
            expiredApiV2.apiConfig.subscriptionExpiredByServer = true;
            m_serversRepository->editServer(serverId, expiredApiV2.toJson(),
                                           serverConfigUtils::configTypeFromJson(expiredApiV2.toJson()));
        }
        return errorCode;
    }

    return applyUpdatedServiceConfig(serverId, serviceProtocol, protocolData, responseBody);
}

ErrorCode SubscriptionController::deactivateDevice(const QString &serverId)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::NoError;
    }
    
    if (!apiV2->isPremium() && !apiV2->isExternalPremium()) {
        return ErrorCode::NoError;
    }

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, apiV2->apiConfig.serverCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .build();

    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    apiV2->containers.clear();
    m_serversRepository->editServer(serverId, apiV2->toJson(),
                                    serverConfigUtils::configTypeFromJson(apiV2->toJson()));
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::deactivateExternalDevice(const QString &serverId, const QString &uuid, const QString &serverCountryCode)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::NoError;
    }
    
    if (!apiV2->isPremium() && !apiV2->isExternalPremium()) {
        return ErrorCode::NoError;
    }

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::installationUuid, uuid)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, serverCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .build();

    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    if (uuid == m_appSettingsRepository->getInstallationUuid(true)) {
        apiV2->containers.clear();
        m_serversRepository->editServer(serverId, apiV2->toJson(),
                                        serverConfigUtils::configTypeFromJson(apiV2->toJson()));
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::exportNativeConfig(const QString &serverId, const QString &serverCountryCode, QString &nativeConfig)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QString protocol = configKey::awg;
    ProtocolData protocolData = generateProtocolData(protocol);

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, serverCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::serviceProtocol, protocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(protocol, protocolData))
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/native_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    nativeConfig = jsonConfig.value(configKey::config).toString();
    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);
    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::revokeNativeConfig(const QString &serverId, const QString &serverCountryCode)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QString protocol = configKey::awg;

    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serverCountryCode, serverCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::serviceProtocol, protocol)
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/revoke_native_config"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        return errorCode;
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::prepareVpnKeyExport(const QString &serverId, QString &vpnKey)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::ApiConfigEmptyError;
    }
    vpnKey = apiV2->vpnKey();
    if (vpnKey.isEmpty()) {
        vpnKey = apiUtils::getPremiumV2VpnKey(apiV2->toJson());
        if (vpnKey.isEmpty()) {
            return ErrorCode::ApiConfigEmptyError;
        }
        apiV2->apiConfig.vpnKey = vpnKey;
        m_serversRepository->editServer(serverId, apiV2->toJson(),
                                         serverConfigUtils::configTypeFromJson(apiV2->toJson()));
    }

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::validateAndUpdateConfig(const QString &serverId, bool hasInstalledContainers,
                                                         CaptchaInfo *captchaInfoOut, ProtocolData *usedProtocolDataOut)
{
    if (!m_serversRepository->apiV2Config(serverId).has_value()) {
        return ErrorCode::NoError;
    }

    if (!hasInstalledContainers) {
        return updateServiceFromGateway(serverId, "", true, captchaInfoOut, usedProtocolDataOut);
    }

    if (isApiKeyExpired(serverId)) {
        qDebug() << "attempt to update api config by expires_at event";
        return updateServiceFromGateway(serverId, "", true, captchaInfoOut, usedProtocolDataOut);
    }

    return ErrorCode::NoError;
}

void SubscriptionController::removeApiConfig(const QString &serverId)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return;
    }

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QString description = apiV2->description;
    QString hostName = apiV2->hostName;
    QString vpncName = QString("%1 (%2) %3")
                               .arg(description)
                               .arg(hostName)
                               .arg("");

    SWIFT_BRIDGE_NAMESPACE::removeVPNC(vpncName.toStdString());
#endif

    apiV2->dns1.clear();
    apiV2->dns2.clear();
    apiV2->containers.clear();
    apiV2->hostName.clear();
    apiV2->defaultContainer = DockerContainer::None;
    apiV2->apiConfig.publicKey = ApiConfig::PublicKeyInfo{};

    m_serversRepository->editServer(serverId, apiV2->toJson(),
                                    serverConfigUtils::configTypeFromJson(apiV2->toJson()));
}

bool SubscriptionController::removeServer(const QString &serverId)
{
    if (serverId.isEmpty()) {
        return false;
    }

    if (!m_serversRepository->apiV2Config(serverId).has_value()) {
        qWarning().noquote() << "SubscriptionController::removeServer: not an Api V2 server, id" << serverId;
        return false;
    }

    const ErrorCode revokeError = deactivateDevice(serverId);
    if (revokeError != ErrorCode::NoError && revokeError != ErrorCode::ApiNotFoundError) {
        qWarning().noquote() << "SubscriptionController::removeServer: deactivateDevice failed (error"
                             << static_cast<int>(revokeError) << "); removing locally anyway.";
    }

    m_serversRepository->removeServer(serverId);
    return true;
}

bool SubscriptionController::isApiKeyExpired(const QString &serverId) const
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return false;
    }
    const QString expiresAt = apiV2->apiConfig.publicKey.expiresAt;
    
    if (expiresAt.isEmpty()) {
        return false;
    }

    auto expiresAtDateTime = QDateTime::fromString(expiresAt, Qt::ISODate).toUTC();
    if (expiresAtDateTime < QDateTime::currentDateTimeUtc()) {
        return true;
    }
    
    return false;
}

void SubscriptionController::setCurrentProtocol(const QString &serverId, const QString &protocolName)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (apiV2.has_value()) {
        apiV2->apiConfig.serviceProtocol = protocolName;
        m_serversRepository->editServer(serverId, apiV2->toJson(),
                                        serverConfigUtils::configTypeFromJson(apiV2->toJson()));
    }
}

bool SubscriptionController::isVlessProtocol(const QString &serverId) const
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    return apiV2.has_value() && apiV2->serviceProtocol() == "vless";
}

QString SubscriptionController::currentProtocol(const QString &serverId) const
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    return apiV2.has_value() ? apiV2->serviceProtocol() : QString();
}

QStringList SubscriptionController::availableProtocols(const QString &serverId) const
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return {};
    }

    const auto currentCountryCode = apiV2->apiConfig.serverCountryCode;
    const auto availableCountries = apiV2->apiConfig.availableCountries;

    QStringList protocols;
    for (const auto &country : availableCountries) {
        const auto countryObject = country.toObject();
        if (countryObject.value(apiDefs::key::serverCountryCode).toString() != currentCountryCode) {
            continue;
        }
        for (const auto &protocol : countryObject.value(apiDefs::key::availableProtocols).toArray()) {
            protocols.push_back(protocol.toString());
        }
        break;
    }
    return protocols;
}

ErrorCode SubscriptionController::processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                                          const QString &serviceProtocol, const QString &productId,
                                                          int *duplicateServerIndex)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    bool purchaseOk = false;
    QString originalTransactionId;
    QString storeTransactionId;
    QString storeProductId;
    QString purchaseError;
    QEventLoop waitPurchase;

    IosController::Instance()->purchaseProduct(productId,
                                               [&](bool success, const QString &txId, const QString &purchasedProductId,
                                                   const QString &originalTxId, const QString &errorString) {
                                                   purchaseOk = success;
                                                   originalTransactionId = originalTxId;
                                                   storeTransactionId = txId;
                                                   storeProductId = purchasedProductId;
                                                   purchaseError = errorString;
                                                   waitPurchase.quit();
                                               });
    waitPurchase.exec();

    if (!purchaseOk || originalTransactionId.isEmpty()) {
        qDebug() << "IAP purchase failed:" << purchaseError;
        return ErrorCode::ApiPurchaseError;
    }
    qInfo().noquote() << "[IAP] Purchase success. transactionId =" << storeTransactionId
                      << "originalTransactionId =" << originalTransactionId << "productId =" << storeProductId;

    bool isTestPurchase = IosController::Instance()->isTestFlight();

    ProtocolData protocolData = generateProtocolData(serviceProtocol);
    return importServiceFromAppStore(userCountryCode, serviceType, serviceProtocol, protocolData,
                                     originalTransactionId, isTestPurchase, duplicateServerIndex);
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(productId);
    return ErrorCode::ApiPurchaseError;
#endif
}

SubscriptionController::AppStoreRestoreResult SubscriptionController::processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                                                                             const QString &serviceProtocol)
{
    AppStoreRestoreResult result;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    bool restoreSuccess = false;
    QList<QVariantMap> restoredTransactions;
    QString restoreError;
    QEventLoop waitRestore;

    IosController::Instance()->restorePurchases([&](bool success, const QList<QVariantMap> &transactions, const QString &errorString) {
        restoreSuccess = success;
        restoredTransactions = transactions;
        restoreError = errorString;
        waitRestore.quit();
    });
    waitRestore.exec();

    if (!restoreSuccess) {
        qWarning().noquote() << "[IAP] Restore failed:" << restoreError;
        result.errorCode = ErrorCode::ApiPurchaseError;
        return result;
    }

    if (restoredTransactions.isEmpty()) {
        qInfo().noquote() << "[IAP] Restore completed, but no transactions were returned";
        result.errorCode = ErrorCode::ApiNoPurchasedSubscriptionsError;
        return result;
    }

    bool isTestPurchase = IosController::Instance()->isTestFlight();
    QSet<QString> processedTransactions;

    for (const QVariantMap &transaction : restoredTransactions) {
        const QString originalTransactionId = transaction.value(QStringLiteral("originalTransactionId")).toString();
        const QString transactionId = transaction.value(QStringLiteral("transactionId")).toString();
        const QString transactionProductId = transaction.value(QStringLiteral("productId")).toString();

        if (originalTransactionId.isEmpty()) {
            qWarning().noquote() << "[IAP] Skipping restored transaction without originalTransactionId" << transactionId;
            continue;
        }

        if (processedTransactions.contains(originalTransactionId)) {
            result.duplicateCount++;
            continue;
        }
        processedTransactions.insert(originalTransactionId);

        qInfo().noquote() << "[IAP] Restoring subscription. transactionId =" << transactionId
                          << "originalTransactionId =" << originalTransactionId << "productId =" << transactionProductId;

        ProtocolData protocolData = generateProtocolData(serviceProtocol);
        int currentDuplicateServerIndex = -1;
        ErrorCode errorCode = importServiceFromAppStore(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                        originalTransactionId, isTestPurchase,
                                                        &currentDuplicateServerIndex);

        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            result.duplicateConfigAlreadyPresent = true;
            if (result.duplicateServerIndex < 0) {
                result.duplicateServerIndex = currentDuplicateServerIndex;
            }
            qInfo().noquote() << "[IAP] Skipping restored transaction" << originalTransactionId
                              << "because subscription config with the same vpn_key already exists";
        } else if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[IAP] Failed to process restored subscription response for transaction" << originalTransactionId;
            result.errorCode = errorCode;
        } else {
            result.hasInstalledConfig = true;
        }
    }

    if (!result.hasInstalledConfig) {
        result.errorCode = result.duplicateConfigAlreadyPresent ? ErrorCode::ApiConfigAlreadyAdded : ErrorCode::ApiPurchaseError;
    }

    return result;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    result.errorCode = ErrorCode::ApiPurchaseError;
    return result;
#endif
}

ErrorCode SubscriptionController::getAccountInfo(const QString &serverId, QJsonObject &accountInfo)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .addField(apiDefs::key::cliVersion, QString(APP_VERSION))
                                     .addField(apiDefs::key::subscriptionStatus, getSubscriptionStatusForRenewal(apiV2->apiConfig))
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/account_info"), apiPayload, responseBody, isTestPurchase);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    accountInfo = QJsonDocument::fromJson(responseBody).object();    

    return ErrorCode::NoError;
}

QFuture<QPair<ErrorCode, QString>> SubscriptionController::getRenewalLink(const QString &serverId)
{
    auto promise = QSharedPointer<QPromise<QPair<ErrorCode, QString>>>::create();
    promise->start();

    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        promise->addResult(qMakePair(ErrorCode::InternalError, QString()));
        promise->finish();
        return promise->future();
    }

    bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, apiV2->apiConfig.userCountryCode)
                                     .addField(apiDefs::key::serviceType, apiV2->serviceType())
                                     .addField(apiDefs::key::authData, apiV2->authData.toJson())
                                     .addField(apiDefs::key::cliVersion, QString(APP_VERSION))
                                     .addField(apiDefs::key::subscriptionStatus, getSubscriptionStatusForRenewal(apiV2->apiConfig))
                                     .build();

    auto gatewayController = QSharedPointer<GatewayController>::create(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                                                       m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                                                       apiDefs::requestTimeoutMsecs,
                                                                       m_appSettingsRepository->isStrictKillSwitchEnabled(),
                                                                       m_appSettingsRepository);
    auto postFuture = gatewayController->postAsync(QString("%1v1/renewal_link"), apiPayload);
    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QByteArray>>();
    QObject::connect(watcher, &QFutureWatcher<QPair<ErrorCode, QByteArray>>::finished,
                     [promise, watcher, gatewayController]() {
                         const auto [errorCode, responseBody] = watcher->result();
                         watcher->deleteLater();
                         if (errorCode != ErrorCode::NoError) {
                             promise->addResult(qMakePair(errorCode, QString()));
                             promise->finish();
                             return;
                         }

                         QJsonObject responseJson = QJsonDocument::fromJson(responseBody).object();
                         const QString url = responseJson.value("renewal_url").toString();
                         promise->addResult(qMakePair(ErrorCode::NoError, url));
                         promise->finish();
                     });
    watcher->setFuture(postFuture);
    return promise->future();
}

ErrorCode SubscriptionController::resolveImportServiceCaptcha(const QString &userCountryCode,
                                                              const QString &serviceType,
                                                              const QString &serviceProtocol,
                                                              const ProtocolData &protocolData,
                                                              const QString &captchaId,
                                                              const QString &captchaSolution,
                                                              CaptchaInfo *retryCaptchaOut)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::captchaId, captchaId)
                                     .addField(apiDefs::key::captchaSolution, normalizeCaptchaSolution(captchaSolution))
                                     .build();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        if (retryCaptchaOut
            && (errorCode == ErrorCode::ApiCaptchaInvalidError || errorCode == ErrorCode::ApiCaptchaRefreshError
                || errorCode == ErrorCode::ApiCaptchaRequiredError)) {
            fillCaptchaInfoFromResponse(responseBody, *retryCaptchaOut);
        }
        return errorCode;
    }

    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    updateApiConfigInJson(serverConfigJson, serviceType, serviceProtocol, userCountryCode, responseBody);

    if (serverConfigJson.value(configKey::configVersion).toInt() != serverConfigUtils::ConfigSource::AmneziaGateway) {
        return ErrorCode::InternalError;
    }

    ApiV2ServerConfig apiV2ServerConfig = ApiV2ServerConfig::fromJson(serverConfigJson);
    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));
    return ErrorCode::NoError;
}
