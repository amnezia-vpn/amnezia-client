#ifndef SUBSCRIPTIONCONTROLLER_H
#define SUBSCRIPTIONCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>
#include <QFuture>
#include <QList>
#include <QVariantMap>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"

class ServersController;

class SubscriptionController
{
public:
    struct ProtocolData
    {
        QString certRequest;
        QString certPrivKey;
        QString wireGuardClientPrivKey;
        QString wireGuardClientPubKey;
        QString xrayUuid;
    };

    struct GatewayRequestData
    {
        QString osVersion;
        QString appVersion;
        QString appLanguage;
        QString installationUuid;
        QString userCountryCode;
        QString serverCountryCode;
        QString serviceType;
        QString serviceProtocol;
        QJsonObject authData;

        QJsonObject toJsonObject() const;
    };

    explicit SubscriptionController(SecureServersRepository* serversRepository,
                                     SecureAppSettingsRepository* appSettingsRepository);

    ProtocolData generateProtocolData(const QString &protocol);
    void appendProtocolDataToApiPayload(const QString &protocol, const ProtocolData &protocolData, QJsonObject &apiPayload);

    ErrorCode importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                      const QString &serviceProtocol, const ProtocolData &protocolData);
    ErrorCode importTrialFromGateway(const QString &userCountryCode, const QString &serviceType,
                                     const QString &serviceProtocol, const QString &email);

    ErrorCode importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                        const QString &serviceProtocol, const ProtocolData &protocolData,
                                        const QString &transactionId, bool isTestPurchase,
                                        int *duplicateServerIndex = nullptr);

    ErrorCode updateServiceFromGateway(const QString &serverId, const QString &newCountryCode, bool isConnectEvent);

    ErrorCode deactivateDevice(const QString &serverId);

    ErrorCode deactivateExternalDevice(const QString &serverId, const QString &uuid, const QString &serverCountryCode);

    ErrorCode exportNativeConfig(const QString &serverId, const QString &serverCountryCode, QString &nativeConfig);

    ErrorCode revokeNativeConfig(const QString &serverId, const QString &serverCountryCode);

    ErrorCode prepareVpnKeyExport(const QString &serverId, QString &vpnKey);

    ErrorCode validateAndUpdateConfig(const QString &serverId, bool hasInstalledContainers);

    void removeApiConfig(const QString &serverId);

    bool removeServer(const QString &serverId);

    void setCurrentProtocol(const QString &serverId, const QString &protocolName);
    bool isVlessProtocol(const QString &serverId) const;

    ErrorCode getAccountInfo(const QString &serverId, QJsonObject &accountInfo);
    QFuture<QPair<ErrorCode, QString>> getRenewalLink(const QString &serverId);

    struct AppStoreRestoreResult
    {
        bool hasInstalledConfig = false;
        bool duplicateConfigAlreadyPresent = false;
        int duplicateCount = 0;
        int duplicateServerIndex = -1;
        ErrorCode errorCode = ErrorCode::NoError;
    };

    ErrorCode processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                     const QString &serviceProtocol, const QString &productId,
                                     int *duplicateServerIndex = nullptr);

    AppStoreRestoreResult processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                                  const QString &serviceProtocol);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase = false);
    bool isApiKeyExpired(const QString &serverId) const;
    
    ErrorCode extractServerConfigJsonFromResponse(const QByteArray &apiResponseBody, const QString &protocol, 
                                                   const ProtocolData &protocolData, QJsonObject &serverConfigJson);
    void updateApiConfigInJson(QJsonObject &serverConfigJson, const QString &serviceType, 
                                const QString &serviceProtocol, const QString &userCountryCode,
                                const QByteArray &apiResponseBody);

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
};

#endif // SUBSCRIPTIONCONTROLLER_H

