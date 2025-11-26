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
        constexpr char awg[] = "awg";

        constexpr char apiEndpoint[] = "api_endpoint";
        constexpr char accessToken[] = "api_key";
        constexpr char protocol[] = "protocol";

        constexpr char uuid[] = "installation_uuid";
        constexpr char osVersion[] = "os_version";
        constexpr char appVersion[] = "app_version";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";
        constexpr char serviceProtocol[] = "service_protocol";
    }
}

ApiConfigsController::ApiConfigsController(ServersController* serversController,
                                           ServersModel* serversModel,
                                           ApiServicesModel* apiServicesModel,
                                           ServicesCatalogController* servicesCatalogController,
                                           SubscriptionController* subscriptionController,
                                           QObject *parent)
    : QObject(parent), m_serversController(serversController), m_serversModel(serversModel), m_apiServicesModel(apiServicesModel), m_servicesCatalogController(servicesCatalogController), m_subscriptionController(subscriptionController)
{
}

bool ApiConfigsController::exportVpnKey(const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    prepareVpnKeyExport();
    if (m_vpnKey.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    SystemController::saveFile(fileName, m_vpnKey);
    return true;
}

bool ApiConfigsController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    auto serverConfigObject = m_serversController->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (m_subscriptionController->isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    QString protocol = configKey::awg; // apiConfigObject.value(configKey::serviceProtocol).toString();
    SubscriptionController::ProtocolData protocolData = m_subscriptionController->generateProtocolData(protocol);

    QString nativeConfig;
    ErrorCode errorCode = m_subscriptionController->exportNativeConfig(apiConfigObject,
                                                                       serverConfigObject.value(configKey::authData).toObject(),
                                            serverCountryCode,
                                                                       protocol,
                                                                       protocolData,
                                                                       nativeConfig);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    nativeConfig.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", protocolData.wireGuardClientPrivKey);

    SystemController::saveFile(fileName, nativeConfig);
    return true;
}

bool ApiConfigsController::revokeNativeConfig(const QString &serverCountryCode)
{
    auto serverConfigObject = m_serversController->getServerConfig(m_serversModel->getProcessedServerIndex());
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (m_subscriptionController->isSubscriptionExpired(apiConfigObject)) {
        emit errorOccurred(ErrorCode::ApiSubscriptionExpiredError);
        return false;
    }

    ErrorCode errorCode = m_subscriptionController->revokeNativeConfig(apiConfigObject,
                                                                        serverConfigObject.value(configKey::authData).toObject(),
                                            serverCountryCode,
                                                                        configKey::awg); // apiConfigObject.value(configKey::serviceProtocol).toString()
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }
    return true;
}

void ApiConfigsController::prepareVpnKeyExport()
{
    QString vpnKey;
    ErrorCode errorCode = m_subscriptionController->prepareVpnKeyExport(m_serversModel->getProcessedServerIndex(), vpnKey);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return;
    }

    m_vpnKey = vpnKey;

    QString vpnKeyForQr = vpnKey;
    vpnKeyForQr.replace("vpn://", "");

    m_qrCodes = qrCodeUtils::generateQrCodeImageSeries(vpnKeyForQr.toUtf8());

    emit vpnKeyExportReady();
}

void ApiConfigsController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

bool ApiConfigsController::fillAvailableServices()
{
    QJsonObject servicesData;
    ErrorCode errorCode = m_servicesCatalogController->fillAvailableServices(servicesData);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    m_apiServicesModel->updateModel(servicesData);
    return true;
}

bool ApiConfigsController::importServiceFromGateway()
{
    QString userCountryCode = m_apiServicesModel->getCountryCode();
    QString serviceType = m_apiServicesModel->getSelectedServiceType();
    QString serviceProtocol = m_apiServicesModel->getSelectedServiceProtocol();

    if (m_serversController->isServerFromApiAlreadyExists(userCountryCode, serviceType, serviceProtocol)) {
        emit errorOccurred(ErrorCode::ApiConfigAlreadyAdded);
        return false;
    }

    SubscriptionController::ProtocolData protocolData = m_subscriptionController->generateProtocolData(serviceProtocol);

    QJsonObject serverConfig;
    ErrorCode errorCode = m_subscriptionController->importServiceFromGateway(userCountryCode, serviceType,
                                                                             serviceProtocol, protocolData,
                                                                             serverConfig);

    if (errorCode == ErrorCode::NoError) {
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
    auto serverConfig = m_serversController->getServerConfig(serverIndex);
    auto apiConfig = serverConfig.value(configKey::apiConfig).toObject();
    QString serviceProtocol = apiConfig.value(configKey::serviceProtocol).toString();

    SubscriptionController::ProtocolData protocolData = m_subscriptionController->generateProtocolData(serviceProtocol);

    bool isConnectEvent = newCountryCode.isEmpty() && newCountryName.isEmpty() && !reloadServiceConfig;

    ErrorCode errorCode = m_subscriptionController->updateServiceFromGateway(serverIndex,
                                            newCountryCode,
                                                                             isConnectEvent,
                                                                             protocolData);

    if (errorCode == ErrorCode::NoError) {
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
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

    auto serverConfig = m_serversController->getServerConfig(serverIndex);
    QString serviceProtocol = serverConfig.value(configKey::protocol).toString();
    SubscriptionController::ProtocolData protocolData = m_subscriptionController->generateProtocolData(serviceProtocol);

    ErrorCode errorCode = m_subscriptionController->updateServiceFromTelegram(serverIndex, protocolData);

    if (errorCode == ErrorCode::NoError) {
        emit updateServerFromApiFinished();
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool ApiConfigsController::deactivateDevice(const bool isRemoveEvent)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();

    ErrorCode errorCode = m_subscriptionController->revokeServiceFromGateway(serverIndex, isRemoveEvent);
    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError && isRemoveEvent) {
            return true;
        }
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}

bool ApiConfigsController::deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();

    ErrorCode errorCode = m_subscriptionController->revokeExternalDevice(serverIndex, uuid, serverCountryCode);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}

bool ApiConfigsController::isConfigValid()
{
    int serverIndex = m_serversController->getDefaultServerIndex();
    QJsonObject serverConfigObject = m_serversController->getServerConfig(serverIndex);
    auto configSource = apiUtils::getConfigSource(serverConfigObject);

    if (configSource == apiDefs::ConfigSource::Telegram
        && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        m_serversController->removeApiConfig(serverIndex);
        return updateServiceFromTelegram(serverIndex);
    } else if (configSource == apiDefs::ConfigSource::AmneziaGateway
               && !m_serversModel->data(serverIndex, ServersModel::Roles::HasInstalledContainers).toBool()) {
        return updateServiceFromGateway(serverIndex, "", "");
    } else if (configSource && m_serversController->isApiKeyExpired(serverIndex)) {
        qDebug() << "attempt to update api config by expires_at event";
        if (configSource == apiDefs::ConfigSource::AmneziaGateway) {
            return updateServiceFromGateway(serverIndex, "", "");
        } else {
            m_serversController->removeApiConfig(serverIndex);
            return updateServiceFromTelegram(serverIndex);
        }
    }
    return true;
}

void ApiConfigsController::setCurrentProtocol(const QString &protocolName)
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversController->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    apiConfigObject[configKey::serviceProtocol] = protocolName;

    serverConfigObject.insert(configKey::apiConfig, apiConfigObject);

    m_serversController->editServer(serverIndex, serverConfigObject);
}

bool ApiConfigsController::isVlessProtocol()
{
    auto serverIndex = m_serversModel->getProcessedServerIndex();
    auto serverConfigObject = m_serversController->getServerConfig(serverIndex);
    auto apiConfigObject = serverConfigObject.value(configKey::apiConfig).toObject();

    if (apiConfigObject[configKey::serviceProtocol].toString() == "vless") {
        return true;
    }
    return false;
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

