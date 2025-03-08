#include "apiConfigsController.h"

#include <QClipboard>
#include <QEventLoop>

#include "amnezia_application.h"
#include "configurators/wireguard_configurator.h"
#include "core/api/apiDefs.h"
#include "core/api/apiUtils.h"
#include "core/controllers/gatewayController.h"
#include "core/qrCodeUtils.h"
#include "ui/controllers/systemController.h"
#include "version.h"

namespace
{
    namespace configKey
    {
        constexpr char cloak[] = "cloak";
        constexpr char awg[] = "awg";

        constexpr char apiEdnpoint[] = "api_endpoint";
        constexpr char accessToken[] = "api_key";
        constexpr char certificate[] = "certificate";
        constexpr char publicKey[] = "public_key";
        constexpr char protocol[] = "protocol";

        constexpr char uuid[] = "installation_uuid";
        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";

        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char aesKey[] = "aes_key";
        constexpr char aesIv[] = "aes_iv";
        constexpr char aesSalt[] = "aes_salt";

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";
    }
}

ApiConfigsController::ApiConfigsController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                           const std::shared_ptr<Settings> &settings, QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_apiServicesModel(apiServicesModel), m_settings(settings)
{
}

bool ApiConfigsController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    QString protocol = apiConfigObject.value(configKey::serviceProtocol).toString();
    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = apiConfigObject.value(configKey::userCountryCode);
    apiPayload[configKey::serverCountryCode] = serverCountryCode;
    apiPayload[configKey::serviceType] = apiConfigObject.value(configKey::serviceType);
    apiPayload[configKey::authData] = serverConfigObject.value(configKey::authData);

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject jsonConfig = QJsonDocument::fromJson(responseBody).object();
    QString nativeConfig = jsonConfig.value(configKey::config).toString();
    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGuardClientPrivKey);

    SystemController::saveFile(fileName, nativeConfig);
    return true;
}

bool ApiConfigsController::revokeNativeConfig(const QString &serverCountryCode)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    QString protocol = apiConfigObject.value(configKey::serviceProtocol).toString();
    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = apiConfigObject.value(configKey::userCountryCode);
    apiPayload[configKey::serverCountryCode] = serverCountryCode;
    apiPayload[configKey::serviceType] = apiConfigObject.value(configKey::serviceType);
    apiPayload[configKey::authData] = serverConfigObject.value(configKey::authData);

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/revoke_native_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }
    return true;
}

void ApiConfigsController::prepareVpnKeyExport()
{
    auto serverConfigObject = m_serversModel->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    auto vpnKey = apiConfigObject.value(apiDefs::key::vpnKey).toString();
    m_vpnKey = vpnKey;

    vpnKey.replace("vpn://", "");

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(vpnKey.toUtf8());

    emit vpnKeyExportReady();
}

void ApiConfigsController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

bool ApiConfigsController::fillAvailableServices()
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    QJsonObject apiPayload;
    apiPayload[configKey::osVersion] = QSysInfo::productType();

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/services"), apiPayload, responseBody);
    if (errorCode == ErrorCode::NoError) {
        if (!responseBody.contains("services")) {
            errorCode = ErrorCode::ApiServicesMissingError;
        }
    }

    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    QJsonObject data = QJsonDocument::fromJson(responseBody).object();
    m_apiServicesModel->updateModel(data);
    return true;
}

bool ApiConfigsController::importServiceFromGateway()
{
    if (m_serversModel->isServerFromApiAlreadyExists(m_apiServicesModel->getCountryCode(), m_apiServicesModel->getSelectedServiceType(),
                                                     m_apiServicesModel->getSelectedServiceProtocol())) {
        emit errorOccurred(ErrorCode::ApiConfigAlreadyAdded);
        return false;
    }

    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto installationUuid = m_settings->getInstallationUuid(true);
    auto userCountryCode = m_apiServicesModel->getCountryCode();
    auto serviceType = m_apiServicesModel->getSelectedServiceType();
    auto serviceProtocol = m_apiServicesModel->getSelectedServiceProtocol();

    ApiPayloadData apiPayloadData = generateApiPayloadData(serviceProtocol);

    QJsonObject apiPayload = fillApiPayload(serviceProtocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = userCountryCode;
    apiPayload[configKey::serviceType] = serviceType;
    apiPayload[configKey::uuid] = installationUuid;

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/config"), apiPayload, responseBody);

    QJsonObject serverConfig;
    if (errorCode == ErrorCode::NoError) {
        fillServerConfig(serviceProtocol, apiPayloadData, responseBody, serverConfig);

        QJsonObject apiConfig = serverConfig.value(configKey::apiConfig).toObject();
        apiConfig.insert(configKey::userCountryCode, m_apiServicesModel->getCountryCode());
        apiConfig.insert(configKey::serviceType, m_apiServicesModel->getSelectedServiceType());
        apiConfig.insert(configKey::serviceProtocol, m_apiServicesModel->getSelectedServiceProtocol());

        serverConfig.insert(configKey::apiConfig, apiConfig);

        m_serversModel->addServer(serverConfig);
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool ApiConfigsController::updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    auto authData = serverConfig.value(configKey::authData).toObject();

    auto installationUuid = m_settings->getInstallationUuid(true);
    auto userCountryCode = apiConfig.value(configKey::userCountryCode).toString();
    auto serviceType = apiConfig.value(configKey::serviceType).toString();
    auto serviceProtocol = apiConfig.value(configKey::serviceProtocol).toString();

    ApiPayloadData apiPayloadData = generateApiPayloadData(serviceProtocol);

    QJsonObject apiPayload = fillApiPayload(serviceProtocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = userCountryCode;
    apiPayload[configKey::serviceType] = serviceType;
    apiPayload[configKey::uuid] = installationUuid;

    if (!newCountryCode.isEmpty()) {
        apiPayload[configKey::serverCountryCode] = newCountryCode;
    }
    if (!authData.isEmpty()) {
        apiPayload[configKey::authData] = authData;
    }

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/config"), apiPayload, responseBody);

    QJsonObject newServerConfig;
    if (errorCode == ErrorCode::NoError) {
        fillServerConfig(serviceProtocol, apiPayloadData, responseBody, newServerConfig);

        QJsonObject newApiConfig = newServerConfig.value(configKey::apiConfig).toObject();
        newApiConfig.insert(configKey::userCountryCode, apiConfig.value(configKey::userCountryCode));
        newApiConfig.insert(configKey::serviceType, apiConfig.value(configKey::serviceType));
        newApiConfig.insert(configKey::serviceProtocol, apiConfig.value(configKey::serviceProtocol));
        newApiConfig.insert(apiDefs::key::vpnKey, apiConfig.value(apiDefs::key::vpnKey));

        newServerConfig.insert(configKey::apiConfig, newApiConfig);
        newServerConfig.insert(configKey::authData, authData);
        // newServerConfig.insert(

        m_serversModel->editServer(newServerConfig, serverIndex);
        if (reloadServiceConfig) {
            emit reloadServerFromApiFinished(tr("API config reloaded"));
        } else if (newCountryName.isEmpty()) {
            emit updateServerFromApiFinished();
        } else {
            emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
        }
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool ApiConfigsController::updateServiceFromTelegram(const int serverIndex)
{
    auto serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto installationUuid = m_settings->getInstallationUuid(true);

#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    if (serverConfig.value(config_key::configVersion).toInt()) {
        QNetworkRequest request;
        request.setTransferTimeout(apiDefs::requestTimeoutMsecs);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Api-Key " + serverConfig.value(configKey::accessToken).toString().toUtf8());
        QString endpoint = serverConfig.value(configKey::apiEdnpoint).toString();
        request.setUrl(endpoint);

        QString protocol = serverConfig.value(configKey::protocol).toString();

        ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

        QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
        apiPayload[configKey::uuid] = installationUuid;

        QByteArray requestBody = QJsonDocument(apiPayload).toJson();

        QNetworkReply *reply = amnApp->networkManager()->post(request, requestBody);

        QEventLoop wait;
        connect(reply, &QNetworkReply::finished, &wait, &QEventLoop::quit);

        QList<QSslError> sslErrors;
        connect(reply, &QNetworkReply::sslErrors, [this, &sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });
        wait.exec();

        auto errorCode = apiUtils::checkNetworkReplyErrors(sslErrors, reply);
        if (errorCode != ErrorCode::NoError) {
            reply->deleteLater();
            emit errorOccurred(errorCode);
            return false;
        }

        auto apiResponseBody = reply->readAll();
        reply->deleteLater();
        fillServerConfig(protocol, apiPayloadData, apiResponseBody, serverConfig);
        m_serversModel->editServer(serverConfig, serverIndex);
        emit updateServerFromApiFinished();
    }
    return true;
}

bool ApiConfigsController::deactivateDevice()
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (apiUtils::getConfigType(serverConfigObject) != apiDefs::ConfigType::AmneziaPremiumV2) {
        return true;
    }

    QString protocol = apiConfigObject.value(configKey::serviceProtocol).toString();
    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = apiConfigObject.value(configKey::userCountryCode);
    apiPayload[configKey::serverCountryCode] = apiConfigObject.value(configKey::serverCountryCode);
    apiPayload[configKey::serviceType] = apiConfigObject.value(configKey::serviceType);
    apiPayload[configKey::authData] = serverConfigObject.value(configKey::authData);
    apiPayload[configKey::uuid] = m_settings->getInstallationUuid(true);

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }

    serverConfigObject.remove(config_key::containers);
    m_serversModel->editServer(serverConfigObject, serverIndex);

    return true;
}

bool ApiConfigsController::deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs);

    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (apiUtils::getConfigType(serverConfigObject) != apiDefs::ConfigType::AmneziaPremiumV2) {
        return true;
    }

    QString protocol = apiConfigObject.value(configKey::serviceProtocol).toString();
    ApiPayloadData apiPayloadData = generateApiPayloadData(protocol);

    QJsonObject apiPayload = fillApiPayload(protocol, apiPayloadData);
    apiPayload[configKey::userCountryCode] = apiConfigObject.value(configKey::userCountryCode);
    apiPayload[configKey::serverCountryCode] = serverCountryCode;
    apiPayload[configKey::serviceType] = apiConfigObject.value(configKey::serviceType);
    apiPayload[configKey::authData] = serverConfigObject.value(configKey::authData);
    apiPayload[configKey::uuid] = uuid;

    QByteArray responseBody;
    ErrorCode errorCode = gatewayController.post(QString("%1v1/revoke_config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError && errorCode != ErrorCode::ApiNotFoundError) {
        emit errorOccurred(errorCode);
        return false;
    }

    if (uuid == m_settings->getInstallationUuid(true)) {
        serverConfigObject.remove(config_key::containers);
        m_serversModel->editServer(serverConfigObject, serverIndex);
    }

    return true;
}

bool ApiConfigsController::isConfigValid()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversModel->getServerConfig(serverIndex);
    auto configSource = apiUtils::getConfigSource(serverConfigObject);

    if (configSource == apiDefs::ConfigSource::Telegram
        && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        m_serversModel->removeApiConfig(serverIndex);
        return updateServiceFromTelegram(serverIndex);
    } else if (configSource == apiDefs::ConfigSource::AmneziaGateway
               && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        return updateServiceFromGateway(serverIndex, "", "");
    } else if (configSource && m_serversModel->isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == apiDefs::ConfigSource::AmneziaGateway) {
            return updateServiceFromGateway(serverIndex, "", "");
        } else {
            m_serversModel->removeApiConfig(serverIndex);
            return updateServiceFromTelegram(serverIndex);
        }
    }
    return true;
}

ApiConfigsController::ApiPayloadData ApiConfigsController::generateApiPayloadData(const QString &protocol)
{
    ApiConfigsController::ApiPayloadData apiPayload;
    if (protocol == configKey::cloak) {
        apiPayload.certRequest = OpenVpnConfigurator::createCertRequest();
    } else if (protocol == configKey::awg) {
        auto connData = WireguardConfigurator::genClientKeys();
        apiPayload.wireGuardClientPubKey = connData.clientPubKey;
        apiPayload.wireGuardClientPrivKey = connData.clientPrivKey;
    }
    return apiPayload;
}

QJsonObject ApiConfigsController::fillApiPayload(const QString &protocol, const ApiPayloadData &apiPayloadData)
{
    QJsonObject obj;
    if (protocol == configKey::cloak) {
        obj[configKey::certificate] = apiPayloadData.certRequest.request;
    } else if (protocol == configKey::awg) {
        obj[configKey::publicKey] = apiPayloadData.wireGuardClientPubKey;
    }

    obj[configKey::osVersion] = QSysInfo::productType();
    obj[configKey::appVersion] = QString(APP_VERSION);

    return obj;
}

void ApiConfigsController::fillServerConfig(const QString &protocol, const ApiPayloadData &apiPayloadData,
                                            const QByteArray &apiResponseBody, QJsonObject &serverConfig)
{
    QString data = QJsonDocument::fromJson(apiResponseBody).object().value(config_key::config).toString();

    data.replace("vpn://", "");
    QByteArray ba = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    if (ba.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return;
    }

    QByteArray ba_uncompressed = qUncompress(ba);
    if (!ba_uncompressed.isEmpty()) {
        ba = ba_uncompressed;
    }

    QString configStr = ba;
    if (protocol == configKey::cloak) {
        configStr.replace("<key>", "<key>\n");
        configStr.replace("$OPENVPN_PRIV_KEY", apiPayloadData.certRequest.privKey);
    } else if (protocol == configKey::awg) {
        configStr.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", apiPayloadData.wireGuardClientPrivKey);
        auto newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
        auto containers = newServerConfig.value(config_key::containers).toArray();
        if (containers.isEmpty()) {
            return; // todo process error
        }
        auto container = containers.at(0).toObject();
        QString containerName = ContainerProps::containerTypeToString(DockerContainer::Awg);
        auto containerConfig = container.value(containerName).toObject();
        auto protocolConfig = QJsonDocument::fromJson(containerConfig.value(config_key::last_config).toString().toUtf8()).object();
        containerConfig[config_key::junkPacketCount] = protocolConfig.value(config_key::junkPacketCount);
        containerConfig[config_key::junkPacketMinSize] = protocolConfig.value(config_key::junkPacketMinSize);
        containerConfig[config_key::junkPacketMaxSize] = protocolConfig.value(config_key::junkPacketMaxSize);
        containerConfig[config_key::initPacketJunkSize] = protocolConfig.value(config_key::initPacketJunkSize);
        containerConfig[config_key::responsePacketJunkSize] = protocolConfig.value(config_key::responsePacketJunkSize);
        containerConfig[config_key::initPacketMagicHeader] = protocolConfig.value(config_key::initPacketMagicHeader);
        containerConfig[config_key::responsePacketMagicHeader] = protocolConfig.value(config_key::responsePacketMagicHeader);
        containerConfig[config_key::underloadPacketMagicHeader] = protocolConfig.value(config_key::underloadPacketMagicHeader);
        containerConfig[config_key::transportPacketMagicHeader] = protocolConfig.value(config_key::transportPacketMagicHeader);
        container[containerName] = containerConfig;
        containers.replace(0, container);
        newServerConfig[config_key::containers] = containers;
        configStr = QString(QJsonDocument(newServerConfig).toJson());
    }

    QJsonObject newServerConfig = QJsonDocument::fromJson(configStr.toUtf8()).object();
    serverConfig[config_key::dns1] = newServerConfig.value(config_key::dns1);
    serverConfig[config_key::dns2] = newServerConfig.value(config_key::dns2);
    serverConfig[config_key::containers] = newServerConfig.value(config_key::containers);
    serverConfig[config_key::hostName] = newServerConfig.value(config_key::hostName);

    if (newServerConfig.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
        serverConfig[config_key::configVersion] = newServerConfig.value(config_key::configVersion);
        serverConfig[config_key::description] = newServerConfig.value(config_key::description);
        serverConfig[config_key::name] = newServerConfig.value(config_key::name);
    }

    auto defaultContainer = newServerConfig.value(config_key::defaultContainer).toString();
    serverConfig[config_key::defaultContainer] = defaultContainer;

    QVariantMap map = serverConfig.value(configKey::apiConfig).toObject().toVariantMap();
    map.insert(newServerConfig.value(configKey::apiConfig).toObject().toVariantMap());
    auto apiConfig = QJsonObject::fromVariantMap(map);

    if (newServerConfig.value(config_key::configVersion).toInt() == apiDefs::ConfigSource::AmneziaGateway) {
        apiConfig.insert(configKey::serviceInfo, QJsonDocument::fromJson(apiResponseBody).object().value(configKey::serviceInfo).toObject());
    }

    serverConfig[configKey::apiConfig] = apiConfig;

    return;
}

QList<QString> ApiConfigsController::getQrCodes()
{
    return m_qrCodes;
}

int ApiConfigsController::getQrCodesCount()
{
    return m_qrCodes.size();
}

QString ApiConfigsController::getVpnKey()
{
    return m_vpnKey;
}
