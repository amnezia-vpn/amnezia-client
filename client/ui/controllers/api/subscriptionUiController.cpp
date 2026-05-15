#include "subscriptionUiController.h"

#include "amneziaApplication.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/qrCodeUtils.h"
#include "ui/controllers/systemController.h"
#include "version.h"
#include <QClipboard>
#include <QDebug>
#include <QSet>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QTimer>

namespace
{
    namespace configKey
    {
        constexpr char awg[] = "awg";
        constexpr char vless[] = "vless";

        constexpr char apiEndpoint[] = "api_endpoint";
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

        constexpr char apiPayload[] = "api_payload";
        constexpr char keyPayload[] = "key_payload";

        constexpr char apiConfig[] = "api_config";
        constexpr char authData[] = "auth_data";

        constexpr char config[] = "config";

        constexpr char subscription[] = "subscription";
        constexpr char endDate[] = "end_date";

        constexpr char isConnectEvent[] = "is_connect_event";
    }

}

SubscriptionUiController::SubscriptionUiController(ServersController* serversController,
                                           ApiServicesModel* apiServicesModel,
                                           ServicesCatalogController* servicesCatalogController,
                                           SubscriptionController* subscriptionController,
                                           ApiSubscriptionPlansModel* apiSubscriptionPlansModel,
                                           ApiBenefitsModel* apiBenefitsModel,
                                           ApiAccountInfoModel* apiAccountInfoModel,
                                           ApiCountryModel* apiCountryModel,
                                           ApiDevicesModel* apiDevicesModel,
                                           SettingsController* settingsController,
                                           QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_apiServicesModel(apiServicesModel),
      m_servicesCatalogController(servicesCatalogController),
      m_subscriptionController(subscriptionController),
      m_apiSubscriptionPlansModel(apiSubscriptionPlansModel),
      m_apiBenefitsModel(apiBenefitsModel),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
      m_settingsController(settingsController)
{
    connect(m_apiServicesModel, &ApiServicesModel::serviceSelectionChanged, this, [this]() {
        ApiServicesModel::ApiServicesData selectedServiceData = m_apiServicesModel->selectedServiceData();
        m_apiSubscriptionPlansModel->updateModel(selectedServiceData.subscriptionPlansJson);
        m_apiBenefitsModel->updateModel(selectedServiceData.benefits);
    });
}

bool SubscriptionUiController::exportVpnKey(const QString &serverId, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    prepareVpnKeyExport(serverId);
    if (m_vpnKey.isEmpty()) {
        emit errorOccurred(ErrorCode::ApiConfigEmptyError);
        return false;
    }

    SystemController::saveFile(fileName, m_vpnKey);
    return true;
}


bool SubscriptionUiController::exportNativeConfig(const QString &serverId, const QString &serverCountryCode, const QString &fileName)
{
    if (fileName.isEmpty()) {
        emit errorOccurred(ErrorCode::PermissionsError);
        return false;
    }

    QString nativeConfig;
    ErrorCode errorCode = m_subscriptionController->exportNativeConfig(serverId, serverCountryCode, nativeConfig);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    SystemController::saveFile(fileName, nativeConfig);
    return true;
}


bool SubscriptionUiController::revokeNativeConfig(const QString &serverId, const QString &serverCountryCode)
{
    ErrorCode errorCode = m_subscriptionController->revokeNativeConfig(serverId, serverCountryCode);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }
    return true;
}


void SubscriptionUiController::prepareVpnKeyExport(const QString &serverId)
{
    QString vpnKey;
    ErrorCode errorCode = m_subscriptionController->prepareVpnKeyExport(serverId, vpnKey);
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


void SubscriptionUiController::copyVpnKeyToClipboard()
{
    auto clipboard = amnApp->getClipboard();
    clipboard->setText(m_vpnKey);
}

bool SubscriptionUiController::fillAvailableServices()
{
    QJsonObject servicesData;
    ErrorCode errorCode = m_servicesCatalogController->fillAvailableServices(servicesData);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    m_apiServicesModel->updateModel(servicesData);
    if (m_apiServicesModel->rowCount() > 0) {
        m_apiServicesModel->setServiceIndex(0);
    }
    return true;
}

bool SubscriptionUiController::importPremiumFromAppStore(const QString &storeProductId)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QString productId = storeProductId.trimmed();
    if (productId.isEmpty()) {
        productId = QStringLiteral("amnezia_premium_6_month");
    }

    int duplicateServerIndex = -1;
    ErrorCode errorCode = m_subscriptionController->processAppStorePurchase(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol(),
        productId,
        &duplicateServerIndex);

    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            emit installServerFromApiFinished(tr("This subscription has already been added"), duplicateServerIndex);
            return true;
        }
        emit errorOccurred(errorCode);
        return false;
    }

    emit installServerFromApiFinished(tr("%1 has been added to the app").arg(m_apiServicesModel->getSelectedServiceName()));
#endif
    return true;
}

bool SubscriptionUiController::restoreServiceFromAppStore()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    const QString premiumServiceType = QStringLiteral("amnezia-premium");

    if (!fillAvailableServices()) {
        qWarning().noquote() << "[IAP] Unable to fetch services list before restore";
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

    if (m_apiServicesModel->rowCount() <= 0) {
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

    // Ensure we have a valid premium selection for gateway requests
    bool premiumSelected = false;
    for (int i = 0; i < m_apiServicesModel->rowCount(); ++i) {
        m_apiServicesModel->setServiceIndex(i);
        if (m_apiServicesModel->getSelectedServiceType() == premiumServiceType) {
            premiumSelected = true;
            break;
        }
    }

    if (!premiumSelected) {
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

    SubscriptionController::AppStoreRestoreResult result = m_subscriptionController->processAppStoreRestore(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol());

    if (!result.hasInstalledConfig) {
        if (result.duplicateConfigAlreadyPresent) {
            emit installServerFromApiFinished(tr("This subscription has already been added"), result.duplicateServerIndex);
            return true;
        }
        emit errorOccurred(result.errorCode);
        return false;
    }

    emit installServerFromApiFinished(tr("Subscription restored successfully."));
    if (result.duplicateCount > 0) {
        qInfo().noquote() << "[IAP] Skipped" << result.duplicateCount
                          << "duplicate restored transactions for original transaction IDs already processed";
    }
#endif
    return true;
}

bool SubscriptionUiController::importFreeFromGateway()
{
    QString userCountryCode = m_apiServicesModel->getCountryCode();
    QString serviceType = m_apiServicesModel->getSelectedServiceType();
    QString serviceProtocol = m_apiServicesModel->getSelectedServiceProtocol();

    if (m_serversController->isServerFromApiAlreadyExists(userCountryCode, serviceType, serviceProtocol)) {
        emit errorOccurred(ErrorCode::ApiConfigAlreadyAdded);
        return false;
    }

    SubscriptionController::ProtocolData protocolData = m_subscriptionController->generateProtocolData(serviceProtocol);
    ErrorCode errorCode = m_subscriptionController->importServiceFromGateway(userCountryCode, serviceType,
                                                                             serviceProtocol, protocolData);

    if (errorCode == ErrorCode::NoError) {
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
        return true;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

bool SubscriptionUiController::importTrialFromGateway(const QString &email)
{
    emit trialEmailError(QString());
    ErrorCode errorCode = m_subscriptionController->importTrialFromGateway(m_apiServicesModel->getCountryCode(),
                                                                            m_apiServicesModel->getSelectedServiceType(),
                                                                            m_apiServicesModel->getSelectedServiceProtocol(),
                                                                            email);
    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiTrialAlreadyUsedError) {
            emit trialEmailError(
                    tr("This email address has already been used to activate a trial. If you like the service, you can upgrade to Premium"));
        } else {
            emit errorOccurred(errorCode);
        }
        return false;
    }

    emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
    return true;
}

bool SubscriptionUiController::updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, const QString &newCountryName,
                                                    bool reloadServiceConfig)
{
    bool isConnectEvent = newCountryCode.isEmpty() && newCountryName.isEmpty() && !reloadServiceConfig;
    bool wasSubscriptionExpired = false;
    if (const auto oldApiV2 = m_serversController->apiV2Config(serverId)) {
        wasSubscriptionExpired = oldApiV2->apiConfig.subscriptionExpiredByServer
                || oldApiV2->apiConfig.isSubscriptionExpired();
    }

    ErrorCode errorCode = m_subscriptionController->updateServiceFromGateway(serverId, newCountryCode, isConnectEvent);

    if (errorCode == ErrorCode::NoError) {
        if (wasSubscriptionExpired) {
            emit subscriptionRefreshNeeded();
        }
        if (reloadServiceConfig) {
            emit reloadServerFromApiFinished(tr("API config reloaded"));
        } else if (newCountryName.isEmpty()) {
            emit updateServerFromApiFinished();
        } else {
            emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
        }
        return true;
    } else {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            emit subscriptionExpiredOnServer();
        } else {
            emit errorOccurred(errorCode);
        }
        return false;
    }
}


bool SubscriptionUiController::deactivateDevice(const QString &serverId)
{
    ErrorCode errorCode = m_subscriptionController->deactivateDevice(serverId);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}


bool SubscriptionUiController::deactivateExternalDevice(const QString &serverId, const QString &uuid, const QString &serverCountryCode)
{
    ErrorCode errorCode = m_subscriptionController->deactivateExternalDevice(serverId, uuid, serverCountryCode);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    return true;
}


void SubscriptionUiController::validateConfig()
{
    const QString serverId = m_serversController->getDefaultServerId();
    if (!serverId.isEmpty() && m_serversController->isLegacyApiV1Server(serverId)) {
        emit unsupportedConnectDrawerRequested();
        emit configValidated(false);
        return;
    }

    bool hasInstalledContainers = m_serversController->hasInstalledContainers(serverId);

    ErrorCode errorCode = m_subscriptionController->validateAndUpdateConfig(serverId, hasInstalledContainers);

    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            emit subscriptionExpiredOnServer();
        } else {
            emit errorOccurred(errorCode);
        }
        emit configValidated(false);
        return;
    }
    emit configValidated(true);
}

void SubscriptionUiController::setCurrentProtocol(const QString &serverId, const QString &protocolName)
{
    m_subscriptionController->setCurrentProtocol(serverId, protocolName);
}


bool SubscriptionUiController::isVlessProtocol(const QString &serverId)
{
    return m_subscriptionController->isVlessProtocol(serverId);
}


void SubscriptionUiController::removeApiConfig(const QString &serverId)
{
    m_subscriptionController->removeApiConfig(serverId);
    emit apiConfigRemoved(tr("Api config removed"));
}

void SubscriptionUiController::removeServer(const QString &serverId)
{
    const QString serverName = m_serversController->notificationDisplayName(serverId);
    if (!m_subscriptionController->removeServer(serverId)) {
        return;
    }
    emit apiServerRemoved(tr("Server '%1' was removed").arg(serverName));
}


QList<QString> SubscriptionUiController::getQrCodes()
{
    return m_qrCodes;
}

int SubscriptionUiController::getQrCodesCount()
{
    return static_cast<int>(m_qrCodes.size());
}

QString SubscriptionUiController::getVpnKey()
{
    return m_vpnKey;
}

bool SubscriptionUiController::getAccountInfo(const QString &serverId, bool reload)
{
    if (reload) {
        QEventLoop wait;
        QTimer::singleShot(1000, &wait, &QEventLoop::quit);
        wait.exec(QEventLoop::ExcludeUserInputEvents);
    }
    QJsonObject accountInfo;
    ErrorCode errorCode = m_subscriptionController->getAccountInfo(serverId, accountInfo);
    if (errorCode != ErrorCode::NoError) {
        emit errorOccurred(errorCode);
        return false;
    }

    const auto apiV2 = m_serversController->apiV2Config(serverId);
    if (!apiV2.has_value()) {
        emit errorOccurred(ErrorCode::InternalError);
        return false;
    }
    m_apiAccountInfoModel->updateModel(accountInfo, apiV2->toJson());

    if (reload) {
        updateApiCountryModel();
        updateApiDevicesModel();
    }

    return true;
}

void SubscriptionUiController::updateApiCountryModel()
{
    m_apiCountryModel->updateModel(m_apiAccountInfoModel->getAvailableCountries(), "");
    m_apiCountryModel->updateIssuedConfigsInfo(m_apiAccountInfoModel->getIssuedConfigsInfo());
}

void SubscriptionUiController::updateApiDevicesModel()
{
    m_apiDevicesModel->updateModel(m_apiAccountInfoModel->getIssuedConfigsInfo(), m_settingsController->getInstallationUuid(false));
}

void SubscriptionUiController::getRenewalLink(const QString &serverId)
{
    if (serverId.isEmpty()) {
        emit errorOccurred(ErrorCode::InternalError);
        return;
    }

    auto *watcher = new QFutureWatcher<QPair<ErrorCode, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<ErrorCode, QString>>::finished, this, [this, watcher]() {
        const auto [errorCode, url] = watcher->result();
        watcher->deleteLater();
        if (errorCode != ErrorCode::NoError) {
            emit errorOccurred(errorCode);
            return;
        }
        if (url.isEmpty()) {
            return;
        }
        emit renewalLinkReceived(url);
    });
    watcher->setFuture(m_subscriptionController->getRenewalLink(serverId));
}

