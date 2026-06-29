#include "subscriptionController.h"

#include <QDebug>
#include <QDateTime>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPromise>
#include <QSet>
#include <QSysInfo>
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
#include "core/controllers/gatewayControllerAdapter.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "version.h"
#include "core/models/containerConfig.h"
#include "core/models/api/apiConfig.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
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

        serverProtocolConfig[configKey::junkPacketCount] = clientProtocolConfig.value(configKey::junkPacketCount);
        serverProtocolConfig[configKey::junkPacketMinSize] = clientProtocolConfig.value(configKey::junkPacketMinSize);
        serverProtocolConfig[configKey::junkPacketMaxSize] = clientProtocolConfig.value(configKey::junkPacketMaxSize);
        serverProtocolConfig[configKey::initPacketJunkSize] = clientProtocolConfig.value(configKey::initPacketJunkSize);
        serverProtocolConfig[configKey::responsePacketJunkSize] = clientProtocolConfig.value(configKey::responsePacketJunkSize);
        serverProtocolConfig[configKey::initPacketMagicHeader] = clientProtocolConfig.value(configKey::initPacketMagicHeader);
        serverProtocolConfig[configKey::responsePacketMagicHeader] = clientProtocolConfig.value(configKey::responsePacketMagicHeader);
        serverProtocolConfig[configKey::underloadPacketMagicHeader] = clientProtocolConfig.value(configKey::underloadPacketMagicHeader);
        serverProtocolConfig[configKey::transportPacketMagicHeader] = clientProtocolConfig.value(configKey::transportPacketMagicHeader);

        serverProtocolConfig[configKey::cookieReplyPacketJunkSize] = clientProtocolConfig.value(configKey::cookieReplyPacketJunkSize);
        serverProtocolConfig[configKey::transportPacketJunkSize] = clientProtocolConfig.value(configKey::transportPacketJunkSize);
        serverProtocolConfig[configKey::specialJunk1] = clientProtocolConfig.value(configKey::specialJunk1);
        serverProtocolConfig[configKey::specialJunk2] = clientProtocolConfig.value(configKey::specialJunk2);
        serverProtocolConfig[configKey::specialJunk3] = clientProtocolConfig.value(configKey::specialJunk3);
        serverProtocolConfig[configKey::specialJunk4] = clientProtocolConfig.value(configKey::specialJunk4);
        serverProtocolConfig[configKey::specialJunk5] = clientProtocolConfig.value(configKey::specialJunk5);

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
        if (responseObj.contains(apiDefs::key::supportedProtocols)) {
            apiConfig.insert(apiDefs::key::supportedProtocols, responseObj.value(apiDefs::key::supportedProtocols).toArray());
        }
        if (responseObj.contains(apiDefs::key::serviceInfo)) {
            apiConfig.insert(apiDefs::key::serviceInfo, responseObj.value(apiDefs::key::serviceInfo).toObject());
        }
    }
    
    serverConfigJson[apiDefs::key::apiConfig] = apiConfig;
}

ErrorCode SubscriptionController::importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                                            const QString &serviceProtocol, const ProtocolData &protocolData,
                                                            CaptchaInfo &captchaInfo)
{
    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = userCountryCode;
    request.serviceType = serviceType;
    request.serviceProtocol = serviceProtocol;

    // public_key для awg — WG pub key, для vless — xray uuid (паритет с appendProtocolDataToApiPayload).
    QString publicKey;
    if (serviceProtocol == configKey::awg) {
        publicKey = protocolData.wireGuardClientPubKey;
    } else if (serviceProtocol == configKey::vless) {
        publicKey = protocolData.xrayUuid;
    }

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(),
                                               m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // payload (+public_key) + post + разбор капчи — внутри SDK. Сырое тело отдаём в существующий
    // extractServerConfigJsonFromResponse/updateApiConfigInJson (app-логика без изменений).
    GatewayControllerAdapter::ImportResult res = gatewayController.importService(request, publicKey);

    if (res.error == ErrorCode::ApiCaptchaRequiredError) {
        captchaInfo.captchaId = res.captchaId;
        captchaInfo.captchaImageBase64 = res.captchaImageBase64;
        captchaInfo.hint = res.hint;
        captchaInfo.isRequired = true;
        return res.error;
    }

    if (res.error != ErrorCode::NoError) {
        return res.error;
    }

    const QByteArray responseBody = res.rawResponse;
    QJsonObject serverConfigJson;
    ErrorCode errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
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

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = userCountryCode;
    request.serviceType = serviceType;
    request.serviceProtocol = serviceProtocol;

    // public_key для awg — WG pub key, для vless — xray uuid (паритет с appendProtocolDataToApiPayload).
    QString publicKey;
    if (serviceProtocol == configKey::awg) {
        publicKey = protocolData.wireGuardClientPubKey;
    } else if (serviceProtocol == configKey::vless) {
        publicKey = protocolData.xrayUuid;
    }

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(),
                                               m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // payload (+public_key, +email) + post + распаковка config — внутри SDK.
    QString serverConfigJson;
    ErrorCode errorCode = gatewayController.importTrial(request, publicKey, trimmedEmail, serverConfigJson);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    if (serverConfigJson.isEmpty()) {
        return ErrorCode::ApiConfigEmptyError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(serverConfigJson.toUtf8()).object();
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
    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = userCountryCode;
    request.serviceType = serviceType;
    request.serviceProtocol = serviceProtocol;

    QString publicKey;
    if (serviceProtocol == configKey::awg) {
        publicKey = protocolData.wireGuardClientPubKey;
    } else if (serviceProtocol == configKey::vless) {
        publicKey = protocolData.xrayUuid;
    }

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // SDK: payload(+public_key,+transaction_id) + post + разбор key + распаковка config + crc(CRC-16).
    GatewayControllerAdapter::AppStoreResult res =
            gatewayController.importServiceFromAppStore(request, publicKey, transactionId);
    if (res.error != ErrorCode::NoError) {
        return res.error;
    }

    const QString normalizedKey = res.vpnKey;  // уже без "vpn://"

    // Check if server with this VPN key already exists (app-state)
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

    if (res.serverConfigJson.isEmpty()) {
        qWarning().noquote() << "[IAP] Subscription response config payload is empty";
        return ErrorCode::ApiPurchaseError;
    }

    QJsonObject configObject = QJsonDocument::fromJson(res.serverConfigJson.toUtf8()).object();

    ApiV2ServerConfig apiV2ServerConfig = ApiV2ServerConfig::fromJson(configObject);
    ApiV2ServerConfig* apiV2 = &apiV2ServerConfig;
    apiV2->apiConfig.vpnKey = normalizedKey;
    apiV2->apiConfig.isTestPurchase = isTestPurchase;
    apiV2->apiConfig.isInAppPurchase = true;
    apiV2->apiConfig.subscriptionExpiredByServer = false;
    apiV2->crc = res.crc;

    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));

    return ErrorCode::NoError;
}

ErrorCode SubscriptionController::updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, bool isConnectEvent)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::InternalError;
    }
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;
    QString serviceProtocol = apiV2->serviceProtocol();
    ProtocolData protocolData = generateProtocolData(serviceProtocol);
    
    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serverCountryCode = newCountryCode;
    request.serviceType = apiV2->serviceType();
    request.serviceProtocol = serviceProtocol;
    request.authData = apiV2->authData.toJson();

    QString publicKey;
    if (serviceProtocol == configKey::awg) {
        publicKey = protocolData.wireGuardClientPubKey;
    } else if (serviceProtocol == configKey::vless) {
        publicKey = protocolData.xrayUuid;
    }

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    GatewayControllerAdapter::ImportResult res = gatewayController.updateService(request, publicKey, isConnectEvent);
    ErrorCode errorCode = res.error;
    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError && !apiV2->apiConfig.isInAppPurchase) {
            ApiV2ServerConfig expiredApiV2 = *apiV2;
            expiredApiV2.apiConfig.subscriptionExpiredByServer = true;
            m_serversRepository->editServer(serverId, expiredApiV2.toJson(),
                                           serverConfigUtils::configTypeFromJson(expiredApiV2.toJson()));
        }
        return errorCode;
    }

    const QByteArray responseBody = res.rawResponse;
    QJsonObject serverConfigJson;
    errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
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
    newApiV2->apiConfig.isTestPurchase = apiV2->apiConfig.isTestPurchase;
    newApiV2->apiConfig.isInAppPurchase = apiV2->apiConfig.isInAppPurchase;
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

ErrorCode SubscriptionController::deactivateDevice(const QString &serverId)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return ErrorCode::NoError;
    }
    
    if (!apiV2->isPremium() && !apiV2->isExternalPremium()) {
        return ErrorCode::NoError;
    }

    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serverCountryCode = apiV2->apiConfig.serverCountryCode;
    request.serviceType = apiV2->serviceType();
    request.authData = apiV2->authData.toJson();

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());
    ErrorCode errorCode = gatewayController.deactivateDevice(request);
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

    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = uuid;
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serverCountryCode = serverCountryCode;
    request.serviceType = apiV2->serviceType();
    request.authData = apiV2->authData.toJson();

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());
    ErrorCode errorCode = gatewayController.deactivateDevice(request);
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

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serverCountryCode = serverCountryCode;
    request.serviceType = apiV2->serviceType();
    request.serviceProtocol = protocol;
    request.authData = apiV2->authData.toJson();

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // SDK отдаёт native config текстом; подстановку WG-приватника делает app.
    ErrorCode errorCode = gatewayController.exportNativeConfig(request, protocolData.wireGuardClientPubKey, nativeConfig);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

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

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serverCountryCode = serverCountryCode;
    request.serviceType = apiV2->serviceType();
    request.serviceProtocol = protocol;
    request.authData = apiV2->authData.toJson();

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());
    ErrorCode errorCode = gatewayController.revokeNativeConfig(request);
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

ErrorCode SubscriptionController::validateAndUpdateConfig(const QString &serverId, bool hasInstalledContainers)
{
    if (!m_serversRepository->apiV2Config(serverId).has_value()) {
        return ErrorCode::NoError;
    }

    if (!hasInstalledContainers) {
        return updateServiceFromGateway(serverId, "", true);
    }

    if (isApiKeyExpired(serverId)) {
        qDebug() << "attempt to update api config by expires_at event";
        return updateServiceFromGateway(serverId, "", true);
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

    AmneziaVPN::removeVPNC(vpncName.toStdString());
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
    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serviceType = apiV2->serviceType();
    request.authData = apiV2->authData.toJson();

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                               m_appSettingsRepository->isDevGatewayEnv(isTestPurchase),
                                               apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // payload (+cli_version, +subscription_status) + post внутри SDK; сырое тело разбирает app.
    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.getAccountInfoRaw(request, QString(APP_VERSION),
                                                             getSubscriptionStatusForRenewal(apiV2->apiConfig),
                                                             responseBody);
    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    accountInfo = QJsonDocument::fromJson(responseBody).object();
    return ErrorCode::NoError;
}

QFuture<QPair<ErrorCode, QString>> SubscriptionController::getRenewalLink(const QString &serverId)
{
    auto apiV2 = m_serversRepository->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        return QtFuture::makeReadyFuture(qMakePair(ErrorCode::InternalError, QString()));
    }

    const bool isTestPurchase = apiV2->apiConfig.isTestPurchase;

    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = apiV2->apiConfig.userCountryCode;
    request.serviceType = apiV2->serviceType();
    request.authData = apiV2->authData.toJson();

    auto gatewayController = QSharedPointer<GatewayControllerAdapter>::create(
            m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
            m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), apiDefs::requestTimeoutMsecs,
            m_appSettingsRepository->isStrictKillSwitchEnabled());

    // payload (+cli_version, +subscription_status) и извлечение renewal_url — внутри SDK.
    auto future = gatewayController->getRenewalLinkAsync(request, QString(APP_VERSION),
                                                         getSubscriptionStatusForRenewal(apiV2->apiConfig));
    return future.then([gatewayController](QPair<ErrorCode, QString> result) { return result; });
}

ErrorCode SubscriptionController::resolveImportServiceCaptcha(const QString &userCountryCode,
                                                              const QString &serviceType,
                                                              const QString &serviceProtocol,
                                                              const ProtocolData &protocolData,
                                                              const QString &captchaId,
                                                              const QString &captchaSolution,
                                                              CaptchaInfo *retryCaptchaOut)
{
    GatewayControllerAdapter::GatewayRequest request;
    request.osVersion = QSysInfo::productType();
    request.appVersion = QString(APP_VERSION);
    request.appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    request.installationUuid = m_appSettingsRepository->getInstallationUuid(true);
    request.userCountryCode = userCountryCode;
    request.serviceType = serviceType;
    request.serviceProtocol = serviceProtocol;

    QString publicKey;
    if (serviceProtocol == configKey::awg) {
        publicKey = protocolData.wireGuardClientPubKey;
    } else if (serviceProtocol == configKey::vless) {
        publicKey = protocolData.xrayUuid;
    }

    GatewayControllerAdapter gatewayController(m_appSettingsRepository->getGatewayEndpoint(),
                                               m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                               m_appSettingsRepository->isStrictKillSwitchEnabled());

    // captcha_solution нормализуется в SDK; на повторную капчу — captchaRequired + поля.
    GatewayControllerAdapter::ImportResult res =
            gatewayController.resolveImportCaptcha(request, publicKey, captchaId, captchaSolution);

    if (res.error != ErrorCode::NoError) {
        if (retryCaptchaOut && res.captchaRequired) {
            retryCaptchaOut->captchaId = res.captchaId;
            retryCaptchaOut->captchaImageBase64 = res.captchaImageBase64;
            retryCaptchaOut->hint = res.hint;
            retryCaptchaOut->isRequired = true;
        }
        return res.error;
    }

    const QByteArray responseBody = res.rawResponse;
    QJsonObject serverConfigJson;
    ErrorCode errorCode = extractServerConfigJsonFromResponse(responseBody, serviceProtocol, protocolData, serverConfigJson);
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
