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
#include "core/models/serverConfig.h"

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
    ErrorCode fillServerConfig(const QJsonObject &serverConfigJson, ServerConfig &serverConfig);

    ErrorCode importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                      const QString &serviceProtocol, const ProtocolData &protocolData,
                                      ServerConfig &serverConfig);
    ErrorCode importTrialFromGateway(const QString &userCountryCode, const QString &serviceType,
                                     const QString &serviceProtocol, const QString &email,
                                     ServerConfig &serverConfig);

    ErrorCode importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                        const QString &serviceProtocol, const ProtocolData &protocolData,
                                        const QString &transactionId, bool isTestPurchase,
                                        ServerConfig &serverConfig,
                                        int *duplicateServerIndex = nullptr);

    ErrorCode updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent);

    ErrorCode deactivateDevice(int serverIndex);

    ErrorCode deactivateExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode);

    ErrorCode exportNativeConfig(int serverIndex, const QString &serverCountryCode, QString &nativeConfig);

    ErrorCode revokeNativeConfig(int serverIndex, const QString &serverCountryCode);

    ErrorCode updateServiceFromTelegram(int serverIndex);

    ErrorCode prepareVpnKeyExport(int serverIndex, QString &vpnKey);

    ErrorCode validateAndUpdateConfig(int serverIndex, bool hasInstalledContainers);

    void removeApiConfig(int serverIndex);

    void setCurrentProtocol(int serverIndex, const QString &protocolName);
    bool isVlessProtocol(int serverIndex) const;

    ErrorCode getAccountInfo(int serverIndex, QJsonObject &accountInfo);
    QFuture<QPair<ErrorCode, QString>> getRenewalLink(int serverIndex);

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
                                     ServerConfig &serverConfig,
                                     int *duplicateServerIndex = nullptr);

    AppStoreRestoreResult processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                                  const QString &serviceProtocol);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase = false);
    bool isApiKeyExpired(int serverIndex) const;
    
    ErrorCode extractServerConfigJsonFromResponse(const QByteArray &apiResponseBody, const QString &protocol, 
                                                   const ProtocolData &protocolData, QJsonObject &serverConfigJson);
    void updateApiConfigInJson(QJsonObject &serverConfigJson, const QString &serviceType, 
                                const QString &serviceProtocol, const QString &userCountryCode,
                                const QByteArray &apiResponseBody);

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;
};

#endif // SUBSCRIPTIONCONTROLLER_H

