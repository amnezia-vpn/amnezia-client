#ifndef STOREPURCHASECONTROLLER_H
#define STOREPURCHASECONTROLLER_H

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "core/controllers/api/subscriptionController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/errorCodes.h"

// In-app purchase flows for App Store and Play Market: purchase, restore,
// gateway validation and store transaction lifecycle (finish / acknowledge)
class StorePurchaseController
{
public:
    struct StoreRestoreResult
    {
        bool hasInstalledConfig = false;
        bool duplicateConfigAlreadyPresent = false;
        int duplicateCount = 0;
        int duplicateServerIndex = -1;
        ErrorCode errorCode = ErrorCode::NoError;
    };

    explicit StorePurchaseController(SecureServersRepository *serversRepository,
                                     SecureAppSettingsRepository *appSettingsRepository);

    ErrorCode processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                      const QString &serviceProtocol, const QString &productId,
                                      int *duplicateServerIndex = nullptr);

    ErrorCode processPlayMarketPurchase(const QString &userCountryCode, const QString &serviceType,
                                        const QString &serviceProtocol, const QString &productId,
                                        int *duplicateServerIndex = nullptr, bool *wasUpgrade = nullptr);

    StoreRestoreResult processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                              const QString &serviceProtocol);

    StoreRestoreResult processPlayMarketRestore(const QString &userCountryCode, const QString &serviceType,
                                                const QString &serviceProtocol);

    // Validates a transaction delivered by the StoreKit Transaction.updates listener
    // (Ask to Buy approval, interrupted purchase, renewal) and finishes it on success
    ErrorCode processAppStoreTransactionUpdate(const QString &userCountryCode, const QString &serviceType,
                                               const QString &serviceProtocol, const QString &originalTransactionId,
                                               const QString &transactionId, int *duplicateServerIndex = nullptr);

    QStringList resolveActiveStoreProductIds();

    // Android counterpart of the iOS Transaction.updates listener: purchases that were
    // paid but never acknowledged (validation failed earlier, or a PENDING purchase was
    // completed outside the app). Call the cheap check first; the process method
    // validates each purchase on the gateway and acknowledges it on success.
    bool hasUnacknowledgedPlayPurchases();
    bool processUnacknowledgedPlayPurchases(const QString &userCountryCode, const QString &serviceType,
                                            const QString &serviceProtocol);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody,
                             bool isTestPurchase = false);

    ErrorCode getSubscriptionInfo(const QString &userCountryCode, const QString &serviceType,
                                  const QString &serviceProtocol, const QString &purchaseToken,
                                  QByteArray &responseBody);

    ErrorCode importServiceFromMarket(const QString &userCountryCode, const QString &serviceType,
                                      const QString &serviceProtocol,
                                      const SubscriptionController::ProtocolData &protocolData,
                                      const QString &transactionId, bool isTestPurchase,
                                      int *duplicateServerIndex, const QString &endpoint);

    // Defined for Android builds only
    ErrorCode validateAndAcknowledgePlayPurchase(const QString &userCountryCode, const QString &serviceType,
                                                 const QString &serviceProtocol, const QString &purchaseToken,
                                                 bool isAcknowledged, int *duplicateServerIndex);

    SecureServersRepository *m_serversRepository;
    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // STOREPURCHASECONTROLLER_H
