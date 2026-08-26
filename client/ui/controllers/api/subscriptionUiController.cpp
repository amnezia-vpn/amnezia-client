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

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#elif defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

namespace
{
constexpr char premiumServiceType[] = "amnezia-premium";
}

SubscriptionUiController::SubscriptionUiController(ServersController* serversController,
                                           ApiServicesModel* apiServicesModel,
                                           ServicesCatalogController* servicesCatalogController,
                                           SubscriptionController* subscriptionController,
                                           StorePurchaseController* storePurchaseController,
                                           ApiSubscriptionPlansModel* apiSubscriptionPlansModel,
                                           ApiBenefitsModel* apiBenefitsModel,
                                           ApiAccountInfoModel* apiAccountInfoModel,
                                           ApiCountryModel* apiCountryModel,
                                           ApiDevicesModel* apiDevicesModel,
                                           SettingsController* settingsController,
                                           ConnectionController* connectionController,
                                           QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_apiServicesModel(apiServicesModel),
      m_servicesCatalogController(servicesCatalogController),
      m_subscriptionController(subscriptionController),
      m_storePurchaseController(storePurchaseController),
      m_apiSubscriptionPlansModel(apiSubscriptionPlansModel),
      m_apiBenefitsModel(apiBenefitsModel),
      m_apiAccountInfoModel(apiAccountInfoModel),
      m_apiCountryModel(apiCountryModel),
      m_apiDevicesModel(apiDevicesModel),
      m_settingsController(settingsController),
      m_connectionController(connectionController)
{
    connect(m_apiServicesModel, &ApiServicesModel::serviceSelectionChanged, this, [this]() {
        ApiServicesModel::ApiServicesData selectedServiceData = m_apiServicesModel->selectedServiceData();
        m_apiSubscriptionPlansModel->updateModel(selectedServiceData.subscriptionPlansJson);
        m_apiBenefitsModel->updateModel(selectedServiceData.benefits);
    });

    connect(this, &SubscriptionUiController::installServerFromApiFinished, this,
            [this](const QString &, int preferredDefaultServerIndex) {
        if (m_connectionController->isConnected()) {
            return;
        }

        const int selectedServerIndex = preferredDefaultServerIndex >= 0
                ? preferredDefaultServerIndex
                : (m_serversController->getServersCount() - 1);
        const QString serverId = m_serversController->getServerId(selectedServerIndex);
        if (!serverId.isEmpty()) {
            m_serversController->setDefaultServer(serverId);
        }
    });

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    connect(IosController::Instance(), &IosController::storeTransactionUpdated, this,
            &SubscriptionUiController::onStoreTransactionUpdated, Qt::QueuedConnection);
    IosController::Instance()->startStoreTransactionObserver();
#elif defined(Q_OS_ANDROID)
    // Counterpart of the iOS Transaction.updates listener: once the event loop is up,
    // pick up purchases that were paid but never acknowledged (validation failed earlier
    // or a PENDING purchase was completed outside the app)
    QTimer::singleShot(0, this, [this]() { checkUnacknowledgedPlayPurchases(); });
#endif
}

bool SubscriptionUiController::isCaptchaAwaitingUser() const
{
    return m_captchaState.isPending;
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

    return SystemController::saveFile(fileName, m_vpnKey);
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

    const bool saved = SystemController::saveFile(fileName, nativeConfig);
    getAccountInfo(serverId, true);
    return saved;
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

QVariantMap SubscriptionUiController::currentActivePlanInfo()
{
    QVariantMap info;
    info.insert(QStringLiteral("hasActivePlan"), false);

    const QStringList activeProductIds = m_storePurchaseController->resolveActiveStoreProductIds();
    qInfo().noquote() << "[Billing][currentActivePlanInfo] active store product ids:" << activeProductIds;

    if (activeProductIds.isEmpty()) {
        return info;
    }

    info.insert(QStringLiteral("hasActivePlan"), true);

    for (const QString &productId : activeProductIds) {
        const int row = m_apiSubscriptionPlansModel->rowForStoreProductId(productId);
        if (row < 0) {
            continue;
        }
        const QVariantMap plan = m_apiSubscriptionPlansModel->planAt(row);
        info.insert(QStringLiteral("storeProductId"), productId);
        info.insert(QStringLiteral("priceAmount"), plan.value(QStringLiteral("priceAmount")));
        info.insert(QStringLiteral("billingPeriod"), plan.value(QStringLiteral("billingPeriod")));
        break;
    }

    qInfo().noquote() << "[Billing][currentActivePlanInfo] resolved to:" << info;
    return info;
}

bool SubscriptionUiController::importPremiumFromStore(const QString &storeProductId)
{
    QString productId = storeProductId.trimmed();
    int duplicateServerIndex = -1;
    bool wasUpgrade = false;
    ErrorCode errorCode = ErrorCode::ApiPurchaseError;

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (productId.isEmpty()) {
        productId = QStringLiteral("amnezia_premium_6_month");
    }

    errorCode = m_storePurchaseController->processAppStorePurchase(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol(),
        productId,
        &duplicateServerIndex, &wasUpgrade);
#elif defined(Q_OS_ANDROID)
    if (productId.isEmpty()) {
        productId = QStringLiteral("premium");
    }

    errorCode = m_storePurchaseController->processPlayMarketPurchase(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol(),
        productId,
        &duplicateServerIndex, &wasUpgrade);
#else
    Q_UNUSED(wasUpgrade);
    return false;
#endif

    if (errorCode != ErrorCode::NoError) {
        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            const QString message = wasUpgrade ? tr("Your subscription has been upgraded")
                                                 : tr("This subscription has already been added");
            emit installServerFromApiFinished(message, duplicateServerIndex);
            return true;
        }
        if (errorCode == ErrorCode::BillingCanceled) {
            qInfo().noquote() << "[IAP] Purchase cancelled by user";
            return false;
        }
        emit errorOccurred(errorCode);
        return false;
    }

    emit installServerFromApiFinished(tr("%1 has been added to the app").arg(m_apiServicesModel->getSelectedServiceName()));
    return true;
}

bool SubscriptionUiController::restoreServiceFromStore()
{
#if defined(Q_OS_IOS) || defined(MACOS_NE) || defined(Q_OS_ANDROID)
    // Ensure we have a valid premium selection for gateway requests
    if (!selectPremiumServiceQuietly()) {
        qWarning().noquote() << "[IAP] Unable to select premium service before restore";
        emit errorOccurred(ErrorCode::ApiServicesMissingError);
        return false;
    }

#if defined(Q_OS_ANDROID)
    StorePurchaseController::StoreRestoreResult result = m_storePurchaseController->processPlayMarketRestore(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol());
#else
    StorePurchaseController::StoreRestoreResult result = m_storePurchaseController->processAppStoreRestore(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol());
#endif

    if (!result.hasInstalledConfig) {
        if (result.duplicateConfigAlreadyPresent) {
            emit installServerFromApiFinished(tr("This subscription has already been added"), result.duplicateServerIndex);
            return true;
        }
        emit errorOccurred(result.errorCode);
        return false;
    }

    emit installServerFromApiFinished(tr("Subscription restored successfully"));
    if (result.duplicateCount > 0) {
        qInfo().noquote() << "[IAP] Skipped" << result.duplicateCount << "duplicate restored purchases";
    }
#endif
    return true;
}

#if defined(Q_OS_IOS) || defined(MACOS_NE)
void SubscriptionUiController::onStoreTransactionUpdated(const QVariantMap &transaction)
{
    m_pendingStoreUpdates.enqueue(transaction);
    if (m_storeUpdateInProgress) {
        return;
    }

    m_storeUpdateInProgress = true;
    while (!m_pendingStoreUpdates.isEmpty()) {
        processStoreTransactionUpdate(m_pendingStoreUpdates.dequeue());
    }
    m_storeUpdateInProgress = false;
}

void SubscriptionUiController::processStoreTransactionUpdate(const QVariantMap &transaction)
{
    const QString transactionId = transaction.value(QStringLiteral("transactionId")).toString();
    const QString originalTransactionId = transaction.value(QStringLiteral("originalTransactionId")).toString();

    if (transactionId.isEmpty() || originalTransactionId.isEmpty()
        || m_handledStoreUpdateTransactionIds.contains(transactionId)) {
        return;
    }

    qInfo().noquote() << "[IAP] Store transaction update received. transactionId =" << transactionId
                      << "originalTransactionId =" << originalTransactionId;

    // Failures here are intentionally silent: the transaction stays unfinished
    // and is redelivered by the Transaction.updates listener on the next launch
    if (!selectPremiumServiceQuietly()) {
        qWarning().noquote() << "[IAP] Unable to select premium service for transaction update, will retry on next launch";
        return;
    }

    int duplicateServerIndex = -1;
    ErrorCode errorCode = m_storePurchaseController->processAppStoreTransactionUpdate(
        m_apiServicesModel->getCountryCode(),
        m_apiServicesModel->getSelectedServiceType(),
        m_apiServicesModel->getSelectedServiceProtocol(),
        originalTransactionId,
        transactionId,
        &duplicateServerIndex);

    if (errorCode == ErrorCode::NoError) {
        m_handledStoreUpdateTransactionIds.insert(transactionId);
        emit installServerFromApiFinished(tr("Purchase confirmed. Subscription has been added to the app"));
    } else if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
        m_handledStoreUpdateTransactionIds.insert(transactionId);
        qInfo().noquote() << "[IAP] Transaction update for already added subscription, transaction finished";
    } else {
        qWarning().noquote() << "[IAP] Transaction update validation failed, errorCode =" << static_cast<int>(errorCode)
                             << "- will retry on next launch";
    }
}

#elif defined(Q_OS_ANDROID)
void SubscriptionUiController::checkUnacknowledgedPlayPurchases()
{
    if (!AndroidController::instance()->isPlay()) {
        return;
    }

    const QJsonArray unacknowledgedPurchases = m_storePurchaseController->findUnacknowledgedPlayPurchases();
    if (unacknowledgedPurchases.isEmpty()) {
        return;
    }

    qInfo().noquote() << "[Billing] Found unacknowledged purchases on startup, validating";

    // Failures here are intentionally silent: the purchase stays unacknowledged
    // and is retried on the next launch (or via manual restore)
    if (!selectPremiumServiceQuietly()) {
        qWarning().noquote() << "[Billing] Unable to select premium service for purchase validation, will retry on next launch";
        return;
    }

    if (m_storePurchaseController->processUnacknowledgedPlayPurchases(
            unacknowledgedPurchases,
            m_apiServicesModel->getCountryCode(),
            m_apiServicesModel->getSelectedServiceType(),
            m_apiServicesModel->getSelectedServiceProtocol())) {
        emit installServerFromApiFinished(tr("Purchase confirmed. Subscription has been added to the app"));
    }
}
#endif

bool SubscriptionUiController::selectPremiumServiceQuietly()
{
    QJsonObject servicesData;
    if (m_servicesCatalogController->fillAvailableServices(servicesData) != ErrorCode::NoError) {
        return false;
    }
    m_apiServicesModel->updateModel(servicesData);

    for (int i = 0; i < m_apiServicesModel->rowCount(); ++i) {
        m_apiServicesModel->setServiceIndex(i);
        if (m_apiServicesModel->getSelectedServiceType() == QLatin1String(premiumServiceType)) {
            return true;
        }
    }
    return false;
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
    SubscriptionController::CaptchaInfo captchaInfo;

    ErrorCode errorCode = m_subscriptionController->importServiceFromGateway(userCountryCode, serviceType,
                                                                             serviceProtocol, protocolData,
                                                                             captchaInfo);

    if (errorCode == ErrorCode::NoError) {
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
        return true;
    } else if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfo.isRequired) {
        m_captchaState = CaptchaState{};
        m_captchaState.flow = CaptchaFlow::Import;
        m_captchaState.userCountryCode = userCountryCode;
        m_captchaState.serviceType = serviceType;
        m_captchaState.serviceProtocol = serviceProtocol;
        m_captchaState.openvpnPrivKey = protocolData.certPrivKey;
        m_captchaState.wireguardClientPrivKey = protocolData.wireGuardClientPrivKey;
        m_captchaState.wireguardClientPubKey = protocolData.wireGuardClientPubKey;
        m_captchaState.xrayUuid = protocolData.xrayUuid;
        m_captchaState.isPending = true;

        emit captchaRequired(captchaInfo.captchaId, captchaInfo.captchaImageBase64,
                             captchaInfo.hint.isEmpty() ? tr("Enter the digits from the image to continue") : captchaInfo.hint);
        return false;
    } else {
        emit errorOccurred(errorCode);
        return false;
    }
}

void SubscriptionUiController::onCaptchaSolved(const QString &captchaId, const QString &solution)
{
    if (!m_captchaState.isPending) {
        return;
    }

    if (m_captchaState.flow == CaptchaFlow::Update) {
        resolveUpdateCaptcha(captchaId, solution);
        return;
    }

    SubscriptionController::ProtocolData protocolData;
    protocolData.certPrivKey = m_captchaState.openvpnPrivKey;
    protocolData.wireGuardClientPrivKey = m_captchaState.wireguardClientPrivKey;
    protocolData.wireGuardClientPubKey = m_captchaState.wireguardClientPubKey;
    protocolData.xrayUuid = m_captchaState.xrayUuid;

    SubscriptionController::CaptchaInfo retryCaptcha;
    ErrorCode errorCode = m_subscriptionController->resolveImportServiceCaptcha(
            m_captchaState.userCountryCode,
            m_captchaState.serviceType,
            m_captchaState.serviceProtocol,
            protocolData,
            captchaId,
            solution,
            &retryCaptcha);

    if (errorCode == ErrorCode::NoError) {
        m_captchaState.isPending = false;
        emit captchaFlowDismissRequested();
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
        return;
    }

    if ((errorCode == ErrorCode::ApiCaptchaInvalidError || errorCode == ErrorCode::ApiCaptchaRefreshError
         || errorCode == ErrorCode::ApiCaptchaRequiredError)
        && retryCaptcha.isRequired) {
        emit captchaRequired(retryCaptcha.captchaId, retryCaptcha.captchaImageBase64,
                             retryCaptcha.hint.isEmpty() ? tr("Enter the digits from the image to continue") : retryCaptcha.hint);
        return;
    }

    m_captchaState.isPending = false;
    emit errorOccurred(errorCode);
}

void SubscriptionUiController::onRefreshCaptchaRequested()
{
    if (!m_captchaState.isPending) {
        return;
    }

    if (m_captchaState.flow == CaptchaFlow::Update) {
        SubscriptionController::CaptchaInfo captchaInfo;
        SubscriptionController::ProtocolData usedProtocolData;
        ErrorCode errorCode = m_subscriptionController->updateServiceFromGateway(
                m_captchaState.serverId,
                m_captchaState.newCountryCode,
                m_captchaState.isConnectEvent,
                &captchaInfo,
                &usedProtocolData);

        if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfo.isRequired) {
            m_captchaState.updateProtocolData = usedProtocolData;
            emit captchaRequired(captchaInfo.captchaId, captchaInfo.captchaImageBase64,
                                 captchaInfo.hint.isEmpty() ? tr("Enter the digits from the image to continue") : captchaInfo.hint);
        } else if (errorCode == ErrorCode::NoError) {
            emitCaptchaUpdateSuccess();
        } else {
            m_captchaState.isPending = false;
            if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
                emit subscriptionExpiredOnServer();
            } else {
                emit errorOccurred(errorCode);
            }
        }
        return;
    }

    SubscriptionController::ProtocolData protocolData;
    protocolData.certPrivKey = m_captchaState.openvpnPrivKey;
    protocolData.wireGuardClientPrivKey = m_captchaState.wireguardClientPrivKey;
    protocolData.wireGuardClientPubKey = m_captchaState.wireguardClientPubKey;
    protocolData.xrayUuid = m_captchaState.xrayUuid;

    SubscriptionController::CaptchaInfo captchaInfo;

    ErrorCode errorCode = m_subscriptionController->importServiceFromGateway(
            m_captchaState.userCountryCode,
            m_captchaState.serviceType,
            m_captchaState.serviceProtocol,
            protocolData,
            captchaInfo);

    if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfo.isRequired) {
        emit captchaRequired(captchaInfo.captchaId, captchaInfo.captchaImageBase64,
                             captchaInfo.hint.isEmpty() ? tr("Enter the digits from the image to continue") : captchaInfo.hint);
    } else if (errorCode != ErrorCode::NoError) {
        m_captchaState.isPending = false;
        emit errorOccurred(errorCode);
    }
}

void SubscriptionUiController::resolveUpdateCaptcha(const QString &captchaId, const QString &solution)
{
    SubscriptionController::CaptchaInfo retryCaptcha;
    ErrorCode errorCode = m_subscriptionController->resolveUpdateServiceCaptcha(
            m_captchaState.serverId,
            m_captchaState.newCountryCode,
            m_captchaState.isConnectEvent,
            m_captchaState.updateProtocolData,
            captchaId,
            solution,
            &retryCaptcha);

    if (errorCode == ErrorCode::NoError) {
        emitCaptchaUpdateSuccess();
        return;
    }

    if ((errorCode == ErrorCode::ApiCaptchaInvalidError || errorCode == ErrorCode::ApiCaptchaRefreshError
         || errorCode == ErrorCode::ApiCaptchaRequiredError)
        && retryCaptcha.isRequired) {
        emit captchaRequired(retryCaptcha.captchaId, retryCaptcha.captchaImageBase64,
                             retryCaptcha.hint.isEmpty() ? tr("Enter the digits from the image to continue") : retryCaptcha.hint);
        return;
    }

    m_captchaState.isPending = false;
    if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
        emit subscriptionExpiredOnServer();
    } else {
        emit errorOccurred(errorCode);
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
                    tr("This email address has already been used to activate a trial. Like the service? Upgrade to Premium"));
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

    SubscriptionController::CaptchaInfo captchaInfo;
    SubscriptionController::ProtocolData usedProtocolData;
    ErrorCode errorCode = m_subscriptionController->updateServiceFromGateway(serverId, newCountryCode, isConnectEvent,
                                                                             &captchaInfo, &usedProtocolData);

    if (errorCode == ErrorCode::NoError) {
        emitUpdateSuccess(wasSubscriptionExpired, reloadServiceConfig, newCountryName);
        return true;
    } else if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfo.isRequired) {
        m_captchaState = CaptchaState{};
        m_captchaState.flow = CaptchaFlow::Update;
        m_captchaState.serverId = serverId;
        m_captchaState.newCountryCode = newCountryCode;
        m_captchaState.newCountryName = newCountryName;
        m_captchaState.isConnectEvent = isConnectEvent;
        m_captchaState.reloadServiceConfig = reloadServiceConfig;
        m_captchaState.wasSubscriptionExpired = wasSubscriptionExpired;
        m_captchaState.updateProtocolData = usedProtocolData;
        m_captchaState.isPending = true;

        emit captchaRequired(captchaInfo.captchaId, captchaInfo.captchaImageBase64,
                             captchaInfo.hint.isEmpty() ? tr("Enter the digits from the image to continue") : captchaInfo.hint);
        return false;
    } else {
        if (errorCode == ErrorCode::ApiSubscriptionExpiredError) {
            emit subscriptionExpiredOnServer();
        } else {
            emit errorOccurred(errorCode);
        }
        return false;
    }
}

void SubscriptionUiController::emitUpdateSuccess(bool wasSubscriptionExpired, bool reloadServiceConfig, const QString &newCountryName)
{
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
}

void SubscriptionUiController::emitCaptchaUpdateSuccess()
{
    const bool fromValidateConfig = m_captchaState.fromValidateConfig;
    const bool wasSubscriptionExpired = m_captchaState.wasSubscriptionExpired;
    const bool reloadServiceConfig = m_captchaState.reloadServiceConfig;
    const QString newCountryName = m_captchaState.newCountryName;

    m_captchaState.isPending = false;
    emit captchaFlowDismissRequested();

    if (fromValidateConfig) {
        emit configValidated(true);
        return;
    }
    emitUpdateSuccess(wasSubscriptionExpired, reloadServiceConfig, newCountryName);
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
    if (serverId.isEmpty()) {
        emit configValidated(false);
        return;
    }

    bool hasInstalledContainers = m_serversController->hasInstalledContainers(serverId);

    SubscriptionController::CaptchaInfo captchaInfo;
    SubscriptionController::ProtocolData usedProtocolData;
    ErrorCode errorCode = m_subscriptionController->validateAndUpdateConfig(serverId, hasInstalledContainers,
                                                                            &captchaInfo, &usedProtocolData);

    if (errorCode == ErrorCode::ApiCaptchaRequiredError && captchaInfo.isRequired) {
        m_captchaState = CaptchaState{};
        m_captchaState.flow = CaptchaFlow::Update;
        m_captchaState.fromValidateConfig = true;
        m_captchaState.serverId = serverId;
        m_captchaState.isConnectEvent = true;
        m_captchaState.updateProtocolData = usedProtocolData;
        m_captchaState.isPending = true;

        emit captchaRequired(captchaInfo.captchaId, captchaInfo.captchaImageBase64,
                             captchaInfo.hint.isEmpty() ? tr("Enter the digits from the image to continue") : captchaInfo.hint);
        emit configValidated(false);
        return;
    }

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


QString SubscriptionUiController::currentProtocol(const QString &serverId)
{
    return m_subscriptionController->currentProtocol(serverId);
}


QStringList SubscriptionUiController::availableProtocols(const QString &serverId)
{
    return m_subscriptionController->availableProtocols(serverId);
}


void SubscriptionUiController::removeApiConfig(const QString &serverId)
{
    m_subscriptionController->removeApiConfig(serverId);
    emit apiConfigRemoved(tr("API config removed"));
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

