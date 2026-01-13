#ifndef SUBSCRIPTIONCONTROLLER_H
#define SUBSCRIPTIONCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>
#include <QList>
#include <QVariantMap>

#include "core/utils/defs.h"
#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"

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

    explicit SubscriptionController(ServersRepository* serversRepository,
                                     AppSettingsRepository* appSettingsRepository);

    ProtocolData generateProtocolData(const QString &protocol);
    void appendProtocolDataToApiPayload(const QString &protocol, const ProtocolData &protocolData, QJsonObject &apiPayload);
    ErrorCode fillServerConfig(const QString &protocol, const ProtocolData &protocolData, const QByteArray &apiResponseBody,
                               QJsonObject &serverConfig);

    ErrorCode importServiceFromGateway(const QString &userCountryCode, const QString &serviceType,
                                      const QString &serviceProtocol, const ProtocolData &protocolData,
                                      QJsonObject &serverConfig);

    ErrorCode importServiceFromAppStore(const QString &userCountryCode, const QString &serviceType,
                                        const QString &serviceProtocol, const ProtocolData &protocolData,
                                        const QString &transactionId, bool isTestPurchase,
                                        QJsonObject &serverConfig);

    ErrorCode updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent);

    ErrorCode revokeServiceFromGateway(int serverIndex, bool isRemoveEvent);

    ErrorCode revokeExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode);

    ErrorCode exportNativeConfig(int serverIndex, const QString &serverCountryCode, QString &nativeConfig);

    ErrorCode revokeNativeConfig(int serverIndex, const QString &serverCountryCode);

    ErrorCode updateServiceFromTelegram(int serverIndex);

    ErrorCode prepareVpnKeyExport(int serverIndex, QString &vpnKey);

    ErrorCode validateAndUpdateConfig(int serverIndex, bool hasInstalledContainers);

    void removeApiConfig(int serverIndex);

    void setApiServiceProtocol(int serverIndex, const QString &protocolName);
    bool isApiServiceProtocolVless(int serverIndex) const;

    ErrorCode getAccountInfo(int serverIndex, QJsonObject &accountInfo);

    struct AppStoreRestoreResult
    {
        bool hasInstalledConfig = false;
        bool duplicateConfigAlreadyPresent = false;
        int duplicateCount = 0;
        ErrorCode errorCode = ErrorCode::NoError;
    };

    ErrorCode processAppStorePurchase(const QString &userCountryCode, const QString &serviceType,
                                     const QString &serviceProtocol, const QString &productId,
                                     QJsonObject &serverConfig);

    AppStoreRestoreResult processAppStoreRestore(const QString &userCountryCode, const QString &serviceType,
                                                  const QString &serviceProtocol);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase = false);
    bool isApiKeyExpired(int serverIndex) const;

    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
};

#endif // SUBSCRIPTIONCONTROLLER_H

