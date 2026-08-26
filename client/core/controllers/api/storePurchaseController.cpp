#include "storePurchaseController.h"

#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QVariantMap>

#include "core/controllers/gatewayController.h"
#include "core/models/api/apiConfig.h"
#include "core/utils/api/gatewayPayloadBuilder.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/serverConfigUtils.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    #include "platforms/ios/ios_controller.h"
#elif defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
    #include "platforms/android/android_utils.h"
#endif

using namespace amnezia;

#if defined(Q_OS_ANDROID)
namespace
{
constexpr int purchaseStatePurchased = 1;
constexpr int purchaseStatePending = 2;

ErrorCode billingErrorFromResponse(int responseCode)
{
    if (responseCode >= ErrorCode::BillingCanceled && responseCode <= ErrorCode::BillingNetworkError) {
        return static_cast<ErrorCode>(responseCode);
    }
    return ErrorCode::ApiPurchaseError;
}

struct PlayPurchaseOutcome
{
    ErrorCode errorCode = ErrorCode::NoError;
    QString purchaseToken;
    bool isUpgrade = false;
    bool isAcknowledged = false;
};

void acknowledgePlayPurchase(const QString &purchaseToken, bool isAcknowledged)
{
    if (isAcknowledged) {
        return;
    }
    auto androidController = AndroidController::instance();
    const QJsonObject ackResult = AndroidUtils::runOnWorkerThread([androidController, purchaseToken]() {
        return androidController->acknowledgePurchase(purchaseToken);
    });
    if (ackResult.value("responseCode").toInt(-1) != 0) {
        qWarning() << "[Billing] Acknowledge failed, responseCode:" << ackResult.value("responseCode").toInt(-1);
    } else {
        qInfo() << "[Billing] Purchase acknowledged successfully";
    }
}

QJsonArray queryPlayPurchases()
{
    auto androidController = AndroidController::instance();
    const QJsonObject purchasesResult = AndroidUtils::runOnWorkerThread([androidController]() {
        return androidController->queryPurchases();
    });
    if (purchasesResult.value("responseCode").toInt(-1) != 0) {
        qWarning().noquote() << "[Billing] queryPurchases failed, responseCode ="
                             << purchasesResult.value("responseCode").toInt(-1);
        return {};
    }
    return purchasesResult.value("purchases").toArray();
}

bool isPaidButUnacknowledged(const QJsonObject &purchaseObj)
{
    return purchaseObj.value("purchaseState").toInt(-1) == purchaseStatePurchased
            && !purchaseObj.value("isAcknowledged").toBool(true)
            && !purchaseObj.value("purchaseToken").toString().isEmpty();
}
}
#endif

StorePurchaseController::StorePurchaseController(SecureServersRepository *serversRepository,
                                                 SecureAppSettingsRepository *appSettingsRepository)
    : m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode StorePurchaseController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload,
                                                  QByteArray &responseBody, bool isTestPurchase)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(isTestPurchase),
                                        m_appSettingsRepository->isDevGatewayEnv(isTestPurchase), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled(), m_appSettingsRepository);
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

ErrorCode StorePurchaseController::getSubscriptionInfo(const QString &userCountryCode, const QString &serviceType,
                                                       const QString &serviceProtocol, const QString &purchaseToken,
                                                       QByteArray &responseBody)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::transactionId, purchaseToken)
                                     .build();
    return executeRequest(QString("%1v1/get_subscription_info"), apiPayload, responseBody, false);
}

ErrorCode StorePurchaseController::importServiceFromMarket(const QString &userCountryCode, const QString &serviceType,
                                                           const QString &serviceProtocol,
                                                           const SubscriptionController::ProtocolData &protocolData,
                                                           const QString &transactionId, bool isTestPurchase,
                                                           int *duplicateServerIndex, const QString &endpoint)
{
    QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                     .addField(apiDefs::key::userCountryCode, userCountryCode)
                                     .addField(apiDefs::key::serviceType, serviceType)
                                     .addField(apiDefs::key::serviceProtocol, serviceProtocol)
                                     .addField(apiDefs::key::publicKey, SubscriptionController::publicKeyForProtocol(serviceProtocol, protocolData))
                                     .addField(apiDefs::key::transactionId, transactionId)
                                     .build();

    QByteArray responseBody;
    qInfo() << "[Billing][importServiceFromMarket] endpoint:" << endpoint << "isTestPurchase:" << isTestPurchase;
    ErrorCode errorCode = executeRequest(QString("%1") + endpoint, apiPayload, responseBody, isTestPurchase);
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
    apiV2ServerConfig.apiConfig.vpnKey = normalizedKey;
    apiV2ServerConfig.apiConfig.subscriptionExpiredByServer = false;
    apiV2ServerConfig.crc = crc;

    m_serversRepository->addServer(QString(), apiV2ServerConfig.toJson(),
                                   serverConfigUtils::configTypeFromJson(apiV2ServerConfig.toJson()));

    return ErrorCode::NoError;
}

ErrorCode StorePurchaseController::processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                                           const QString &serviceProtocol, const QString &productId,
                                                           int *duplicateServerIndex, bool *wasUpgrade)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    const bool hadActivePlanBeforePurchase = !resolveActiveStoreProductIds().isEmpty();
    if (wasUpgrade) {
        *wasUpgrade = hadActivePlanBeforePurchase;
    }

    bool purchaseOk = false;
    QString originalTransactionId;
    QString storeTransactionId;
    QString storeProductId;
    QString purchaseError;
    IosController::StorePurchaseFailure failureReason = IosController::StorePurchaseFailure::Other;
    QEventLoop waitPurchase;

    IosController::Instance()->purchaseProduct(productId,
                                               [&](bool success, const QString &txId, const QString &purchasedProductId,
                                                   const QString &originalTxId, const QString &errorString,
                                                   IosController::StorePurchaseFailure reason) {
                                                   purchaseOk = success;
                                                   originalTransactionId = originalTxId;
                                                   storeTransactionId = txId;
                                                   storeProductId = purchasedProductId;
                                                   purchaseError = errorString;
                                                   failureReason = reason;
                                                   waitPurchase.quit();
                                               });
    waitPurchase.exec();

    if (!purchaseOk || originalTransactionId.isEmpty()) {
        qDebug() << "IAP purchase failed:" << purchaseError;
        switch (failureReason) {
        case IosController::StorePurchaseFailure::Cancelled: return ErrorCode::BillingCanceled;
        case IosController::StorePurchaseFailure::Pending: return ErrorCode::ApiPurchasePendingError;
        default: return ErrorCode::ApiPurchaseError;
        }
    }
    qInfo().noquote() << "[IAP] Purchase success. transactionId =" << storeTransactionId
                      << "originalTransactionId =" << originalTransactionId << "productId =" << storeProductId;

    bool isTestPurchase = IosController::Instance()->isTestFlight();

    SubscriptionController::ProtocolData protocolData = SubscriptionController::generateProtocolData(serviceProtocol);
    ErrorCode importError = importServiceFromMarket(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                    originalTransactionId, isTestPurchase, duplicateServerIndex,
                                                    QStringLiteral("v1/subscriptions"));

    // Finish the StoreKit transaction only after the gateway has validated the purchase;
    // otherwise it stays in the unfinished queue and is retried via Transaction.updates
    if (importError == ErrorCode::NoError || importError == ErrorCode::ApiConfigAlreadyAdded) {
        IosController::Instance()->finishStoreTransaction(storeTransactionId);
    }

    return importError;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(productId);
    Q_UNUSED(duplicateServerIndex);
    Q_UNUSED(wasUpgrade);
    return ErrorCode::ApiPurchaseError;
#endif
}

ErrorCode StorePurchaseController::processPlayMarketPurchase(const QString &userCountryCode, const QString &serviceType,
                                                             const QString &serviceProtocol, const QString &productId,
                                                             int *duplicateServerIndex, bool *wasUpgrade)
{
#if defined(Q_OS_ANDROID)
    auto androidController = AndroidController::instance();

    const PlayPurchaseOutcome outcome = AndroidUtils::runOnWorkerThread([androidController, productId]() {
        PlayPurchaseOutcome outcome;

        QString oldPurchaseToken;
        QJsonObject existingPurchasesResult = androidController->queryPurchases();
        if (existingPurchasesResult.value("responseCode").toInt(-1) == 0) {
            const QJsonArray existingPurchases = existingPurchasesResult.value("purchases").toArray();
            for (const QJsonValue &purchaseValue : existingPurchases) {
                const QJsonObject existingPurchase = purchaseValue.toObject();
                if (existingPurchase.value("purchaseState").toInt(-1) == purchaseStatePurchased) {
                    oldPurchaseToken = existingPurchase.value("purchaseToken").toString();
                    qInfo().noquote() << "[Billing] Found existing active subscription, will upgrade instead of purchasing a new one. oldPurchaseToken =" << oldPurchaseToken;
                    break;
                }
            }
        }
        outcome.isUpgrade = !oldPurchaseToken.isEmpty();

        QJsonObject plansResult = androidController->getSubscriptionPlans();
        int responseCode = plansResult.value("responseCode").toInt(-1);
        if (responseCode != 0) {
            qWarning() << "[Billing] Failed to get subscription plans, responseCode:" << responseCode;
            outcome.errorCode = billingErrorFromResponse(responseCode);
            return outcome;
        }
        QJsonArray products = plansResult.value("products").toArray();
        QString offerToken;
        QString fallbackOfferToken;
        for (const QJsonValue &productValue : products) {
            QJsonObject product = productValue.toObject();
            QJsonArray offers = product.value("offers").toArray();
            for (const QJsonValue &offerValue : offers) {
                QJsonObject offer = offerValue.toObject();
                if (offer.value("basePlanId").toString() != productId) continue;

                const QString token = offer.value("offerToken").toString();
                QJsonArray pricingPhases = offer.value("pricingPhases").toArray();
                const bool hasFreeTrial = !pricingPhases.isEmpty()
                        && pricingPhases.first().toObject().value("priceAmountMicros").toDouble() == 0;

                if (hasFreeTrial && !oldPurchaseToken.isEmpty()) continue;

                if (fallbackOfferToken.isEmpty()) fallbackOfferToken = token;

                if (hasFreeTrial) {
                    offerToken = token;
                    qInfo() << "[Billing] Found free trial offer for basePlanId:" << productId;
                    break;
                }
            }
            if (!offerToken.isEmpty()) break;
        }
        if (offerToken.isEmpty()) offerToken = fallbackOfferToken;
        if (offerToken.isEmpty()) {
            qWarning() << "[Billing] No offer token found for basePlanId:" << productId;
            outcome.errorCode = ErrorCode::SubscriptionUnavailable;
            return outcome;
        }
        QJsonObject purchaseResult = oldPurchaseToken.isEmpty()
                ? androidController->purchaseSubscription(offerToken)
                : androidController->upgradeSubscription(offerToken, oldPurchaseToken);
        responseCode = purchaseResult.value("responseCode").toInt(-1);
        if (responseCode != 0) {
            qWarning() << "[Billing] Purchase failed, responseCode:" << responseCode;
            outcome.errorCode = billingErrorFromResponse(responseCode);
            return outcome;
        }
        QJsonArray purchases = purchaseResult.value("purchases").toArray();
        if (purchases.isEmpty()) {
            qWarning() << "[Billing] Purchase succeeded but no purchases returned";
            outcome.errorCode = ErrorCode::ApiPurchaseError;
            return outcome;
        }
        QJsonObject purchase = purchases.at(0).toObject();
        outcome.purchaseToken = purchase.value("purchaseToken").toString();
        outcome.isAcknowledged = purchase.value("isAcknowledged").toBool();
        int purchaseState = purchase.value("purchaseState").toInt(-1);
        qInfo().noquote() << "[Billing] Purchase success. purchaseToken =" << outcome.purchaseToken
                          << "isAcknowledged:" << outcome.isAcknowledged << "purchaseState:" << purchaseState;
        // purchaseState: 1 = PURCHASED, 2 = PENDING (user must confirm payment in Google Play), 0 = UNSPECIFIED
        if (purchaseState == purchaseStatePending) {
            qWarning() << "[Billing] Purchase is in PENDING state, waiting for user to confirm payment";
            outcome.errorCode = ErrorCode::ApiPurchasePendingError;
            return outcome;
        }
        if (purchaseState != purchaseStatePurchased || outcome.purchaseToken.isEmpty()) {
            outcome.errorCode = ErrorCode::ApiPurchaseError;
            return outcome;
        }
        return outcome;
    });

    if (outcome.errorCode != ErrorCode::NoError) {
        return outcome.errorCode;
    }

    if (wasUpgrade) {
        *wasUpgrade = outcome.isUpgrade;
    }

    if (outcome.purchaseToken != m_lastPlayPurchaseToken) {
        m_lastPlayPurchaseToken = outcome.purchaseToken;
        m_lastPlayBasePlanId = productId;
    }

    return finalizePlayPurchase(userCountryCode, serviceType, serviceProtocol, outcome.purchaseToken,
                                outcome.isAcknowledged, duplicateServerIndex, QStringLiteral("v1/subscriptions"));
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(productId);
    Q_UNUSED(duplicateServerIndex);
    Q_UNUSED(wasUpgrade);
    return ErrorCode::ApiPurchaseError;
#endif
}

StorePurchaseController::StoreRestoreResult StorePurchaseController::processAppStoreRestore(const QString &userCountryCode,
                                                                                            const QString &serviceType,
                                                                                            const QString &serviceProtocol)
{
    StoreRestoreResult result;

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

    qInfo().noquote() << "[IAP][processAppStoreRestore] restorePurchases result: success=" << restoreSuccess
                      << "transactions count=" << restoredTransactions.size()
                      << "error=" << restoreError;

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

        qInfo().noquote() << "[IAP][processAppStoreRestore] Processing transaction: transactionId=" << transactionId
                          << "originalTransactionId=" << originalTransactionId << "productId=" << transactionProductId;

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

        SubscriptionController::ProtocolData protocolData = SubscriptionController::generateProtocolData(serviceProtocol);
        int currentDuplicateServerIndex = -1;
        ErrorCode errorCode = importServiceFromMarket(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                      originalTransactionId, isTestPurchase,
                                                      &currentDuplicateServerIndex,
                                                      QStringLiteral("v1/restore_subscription"));

        qInfo().noquote() << "[IAP][processAppStoreRestore] importServiceFromMarket errorCode=" << static_cast<int>(errorCode)
                          << "for originalTransactionId=" << originalTransactionId;

        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            result.duplicateConfigAlreadyPresent = true;
            if (result.duplicateServerIndex < 0) {
                result.duplicateServerIndex = currentDuplicateServerIndex;
            }
            qInfo().noquote() << "[IAP] Skipping restored transaction" << originalTransactionId
                              << "because subscription config with the same vpn_key already exists";
        } else if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[IAP] Failed to process restored subscription response for transaction" << originalTransactionId
                                 << "errorCode=" << static_cast<int>(errorCode);
            result.errorCode = errorCode;
        } else {
            result.hasInstalledConfig = true;
        }
    }

    if (!result.hasInstalledConfig) {
        result.errorCode = result.duplicateConfigAlreadyPresent ? ErrorCode::ApiConfigAlreadyAdded : ErrorCode::ApiPurchaseError;
    }

    qInfo().noquote() << "[IAP][processAppStoreRestore] Done. hasInstalledConfig=" << result.hasInstalledConfig
                      << "duplicateConfigAlreadyPresent=" << result.duplicateConfigAlreadyPresent
                      << "duplicateCount=" << result.duplicateCount
                      << "errorCode=" << static_cast<int>(result.errorCode);

    return result;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    result.errorCode = ErrorCode::ApiPurchaseError;
    return result;
#endif
}

ErrorCode StorePurchaseController::processAppStoreTransactionUpdate(const QString &userCountryCode, const QString &serviceType,
                                                                    const QString &serviceProtocol, const QString &originalTransactionId,
                                                                    const QString &transactionId, int *duplicateServerIndex)
{
#if defined(Q_OS_IOS) || defined(MACOS_NE)
    if (originalTransactionId.isEmpty()) {
        return ErrorCode::ApiPurchaseError;
    }

    qInfo().noquote() << "[IAP] Processing transaction update. transactionId =" << transactionId
                      << "originalTransactionId =" << originalTransactionId;

    bool isTestPurchase = IosController::Instance()->isTestFlight();
    SubscriptionController::ProtocolData protocolData = SubscriptionController::generateProtocolData(serviceProtocol);
    ErrorCode errorCode = importServiceFromMarket(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                  originalTransactionId, isTestPurchase, duplicateServerIndex,
                                                  QStringLiteral("v1/restore_subscription"));

    // On validation failure the transaction is left unfinished so it is redelivered
    // by the Transaction.updates listener on the next app launch
    if (errorCode == ErrorCode::NoError || errorCode == ErrorCode::ApiConfigAlreadyAdded) {
        IosController::Instance()->finishStoreTransaction(transactionId);
    }

    return errorCode;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    Q_UNUSED(originalTransactionId);
    Q_UNUSED(transactionId);
    Q_UNUSED(duplicateServerIndex);
    return ErrorCode::ApiPurchaseError;
#endif
}

StorePurchaseController::StoreRestoreResult StorePurchaseController::processPlayMarketRestore(const QString &userCountryCode,
                                                                                              const QString &serviceType,
                                                                                              const QString &serviceProtocol)
{
    StoreRestoreResult result;

#if defined(Q_OS_ANDROID)
    auto androidController = AndroidController::instance();

    const QJsonObject purchasesResult = AndroidUtils::runOnWorkerThread([androidController]() {
        return androidController->queryPurchases();
    });

    int responseCode = purchasesResult.value("responseCode").toInt(-1);
    if (responseCode != 0) {
        qWarning().noquote() << "[Billing] queryPurchases failed, responseCode =" << responseCode;
        result.errorCode = billingErrorFromResponse(responseCode);
        return result;
    }

    QJsonArray purchases = purchasesResult.value("purchases").toArray();
    if (purchases.isEmpty()) {
        qInfo().noquote() << "[Billing] Restore completed, but no purchases were found";
        result.errorCode = ErrorCode::ApiNoPurchasesToRestore;
        return result;
    }

    QSet<QString> processedTokens;
    for (const QJsonValue &purchaseValue : std::as_const(purchases)) {
        const QJsonObject purchaseObj = purchaseValue.toObject();
        const QString purchaseToken = purchaseObj.value("purchaseToken").toString();

        if (purchaseToken.isEmpty()) {
            qWarning().noquote() << "[Billing] Skipping purchase without purchaseToken";
            continue;
        }

        if (purchaseObj.value("purchaseState").toInt(-1) != purchaseStatePurchased) {
            qInfo().noquote() << "[Billing] Skipping purchase in state" << purchaseObj.value("purchaseState").toInt(-1);
            continue;
        }

        if (processedTokens.contains(purchaseToken)) {
            result.duplicateCount++;
            continue;
        }
        processedTokens.insert(purchaseToken);

        qInfo().noquote() << "[Billing] Restoring subscription";

        int currentDuplicateServerIndex = -1;
        ErrorCode errorCode = finalizePlayPurchase(userCountryCode, serviceType, serviceProtocol,
                                                   purchaseToken, purchaseObj.value("isAcknowledged").toBool(),
                                                   &currentDuplicateServerIndex, QStringLiteral("v1/restore_subscription"));

        if (errorCode == ErrorCode::ApiConfigAlreadyAdded) {
            result.duplicateConfigAlreadyPresent = true;
            if (result.duplicateServerIndex < 0) {
                result.duplicateServerIndex = currentDuplicateServerIndex;
            }
            qInfo().noquote() << "[Billing] Skipping purchase because subscription config with the same vpn_key already exists";
        } else if (errorCode != ErrorCode::NoError) {
            qWarning().noquote() << "[Billing] Failed to process restored subscription, errorCode =" << static_cast<int>(errorCode);
            result.errorCode = errorCode;
        } else {
            result.hasInstalledConfig = true;
        }
    }

    if (!result.hasInstalledConfig && !result.duplicateConfigAlreadyPresent && result.errorCode == ErrorCode::NoError) {
        result.errorCode = ErrorCode::ApiNoPurchasesToRestore;
    } else if (!result.hasInstalledConfig && result.duplicateConfigAlreadyPresent) {
        result.errorCode = ErrorCode::ApiConfigAlreadyAdded;
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

#if defined(Q_OS_ANDROID)
ErrorCode StorePurchaseController::finalizePlayPurchase(const QString &userCountryCode, const QString &serviceType,
                                                        const QString &serviceProtocol, const QString &purchaseToken,
                                                        bool isAcknowledged, int *duplicateServerIndex, const QString &endpoint)
{
    QByteArray checkResponse;
    ErrorCode checkError = getSubscriptionInfo(userCountryCode, serviceType, serviceProtocol, purchaseToken, checkResponse);
    if (checkError != ErrorCode::NoError) {
        qWarning().noquote() << "[Billing] Initial subscriptions check failed:" << static_cast<int>(checkError);
        return checkError;
    }

    QJsonObject checkObject = QJsonDocument::fromJson(checkResponse).object();
    bool isTestPurchase = checkObject.value(apiDefs::key::isTestPurchase).toBool(false);
    qInfo().noquote() << "[Billing] Purchase isTestPurchase =" << isTestPurchase;

    SubscriptionController::ProtocolData protocolData = SubscriptionController::generateProtocolData(serviceProtocol);
    ErrorCode errorCode = importServiceFromMarket(userCountryCode, serviceType, serviceProtocol, protocolData,
                                                  purchaseToken, isTestPurchase, duplicateServerIndex, endpoint);

    // Acknowledge only after the gateway has accepted the purchase; otherwise it stays
    // unacknowledged and is retried on the next startup check or manual restore
    if (errorCode == ErrorCode::NoError || errorCode == ErrorCode::ApiConfigAlreadyAdded) {
        acknowledgePlayPurchase(purchaseToken, isAcknowledged);
    }
    return errorCode;
}
#endif

QJsonArray StorePurchaseController::findUnacknowledgedPlayPurchases()
{
#if defined(Q_OS_ANDROID)
    QJsonArray unacknowledged;
    for (const QJsonValue &purchaseValue : queryPlayPurchases()) {
        if (isPaidButUnacknowledged(purchaseValue.toObject())) {
            unacknowledged.append(purchaseValue);
        }
    }
    return unacknowledged;
#else
    return {};
#endif
}

bool StorePurchaseController::processUnacknowledgedPlayPurchases(const QJsonArray &purchases, const QString &userCountryCode,
                                                                 const QString &serviceType, const QString &serviceProtocol)
{
#if defined(Q_OS_ANDROID)
    bool installedNewConfig = false;
    QSet<QString> processedTokens;

    for (const QJsonValue &purchaseValue : purchases) {
        const QJsonObject purchaseObj = purchaseValue.toObject();
        const QString purchaseToken = purchaseObj.value("purchaseToken").toString();
        if (processedTokens.contains(purchaseToken)) {
            continue;
        }
        processedTokens.insert(purchaseToken);

        qInfo().noquote() << "[Billing] Found paid but unacknowledged purchase, validating";

        ErrorCode errorCode = finalizePlayPurchase(userCountryCode, serviceType, serviceProtocol,
                                                   purchaseToken, false, nullptr, QStringLiteral("v1/restore_subscription"));
        if (errorCode == ErrorCode::NoError) {
            installedNewConfig = true;
        } else if (errorCode != ErrorCode::ApiConfigAlreadyAdded) {
            qWarning().noquote() << "[Billing] Purchase validation failed, errorCode =" << static_cast<int>(errorCode)
                                 << "- will retry on next launch";
        }
    }
    return installedNewConfig;
#else
    Q_UNUSED(userCountryCode);
    Q_UNUSED(serviceType);
    Q_UNUSED(serviceProtocol);
    return false;
#endif
}

QStringList StorePurchaseController::resolveActiveStoreProductIds()
{
    QStringList activeProductIds;

#if defined(Q_OS_ANDROID)
    auto androidController = AndroidController::instance();

    const QJsonObject purchasesResult = AndroidUtils::runOnWorkerThread([androidController]() {
        return androidController->queryPurchases();
    });

    if (purchasesResult.value("responseCode").toInt(-1) != 0) {
        qWarning().noquote() << "[Billing][resolveActiveStoreProductIds] queryPurchases failed";
        return activeProductIds;
    }

    const QJsonArray purchases = purchasesResult.value("purchases").toArray();
    for (const QJsonValue &purchaseValue : purchases) {
        const QJsonObject purchaseObj = purchaseValue.toObject();
        const QString purchaseToken = purchaseObj.value("purchaseToken").toString();
        qInfo().noquote() << "[Billing][resolveActiveStoreProductIds] purchase found. purchaseToken =" << purchaseToken
                          << "purchaseState:" << purchaseObj.value("purchaseState").toInt()
                          << "productIds:" << purchaseObj.value("productIds").toArray();
        if (purchaseObj.value("purchaseState").toInt() != purchaseStatePurchased) {
            continue;
        }

        // Play's Purchase object only carries the product id, not the base plan/offer that was
        // actually bought; prefer the base plan id remembered from this session's own purchase.
        if (!m_lastPlayBasePlanId.isEmpty() && !purchaseToken.isEmpty() && purchaseToken == m_lastPlayPurchaseToken) {
            if (!activeProductIds.contains(m_lastPlayBasePlanId)) {
                activeProductIds.append(m_lastPlayBasePlanId);
            }
            continue;
        }

        const QJsonArray productIds = purchaseObj.value("productIds").toArray();
        for (const QJsonValue &productIdValue : productIds) {
            const QString productId = productIdValue.toString();
            if (!productId.isEmpty() && !activeProductIds.contains(productId)) {
                activeProductIds.append(productId);
            }
        }
    }
#elif defined(Q_OS_IOS) || defined(MACOS_NE)
    bool restoreSuccess = false;
    QList<QVariantMap> transactions;
    QString restoreError;
    QEventLoop waitRestore;

    IosController::Instance()->fetchLocalEntitlements([&](bool success, const QList<QVariantMap> &localTransactions, const QString &errorString) {
        restoreSuccess = success;
        transactions = localTransactions;
        restoreError = errorString;
        waitRestore.quit();
    });
    waitRestore.exec();

    if (!restoreSuccess) {
        qWarning().noquote() << "[Billing][resolveActiveStoreProductIds] fetchLocalEntitlements failed:" << restoreError;
        return activeProductIds;
    }

    for (const QVariantMap &transaction : std::as_const(transactions)) {
        const QString productId = transaction.value("productId").toString();
        if (!productId.isEmpty() && !activeProductIds.contains(productId)) {
            activeProductIds.append(productId);
        }
    }
#endif

    return activeProductIds;
}
