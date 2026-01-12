#ifndef SUBSCRIPTIONCONTROLLER_H
#define SUBSCRIPTIONCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/defs.h"
#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"

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

    bool isSubscriptionExpired(const QJsonObject &apiConfig);

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

    ErrorCode updateServiceFromGateway(int serverIndex, const QString &newCountryCode, bool isConnectEvent,
                                      const ProtocolData &protocolData);

    ErrorCode revokeServiceFromGateway(int serverIndex, bool isRemoveEvent);

    ErrorCode revokeExternalDevice(int serverIndex, const QString &uuid, const QString &serverCountryCode);

    ErrorCode exportNativeConfig(const QJsonObject &apiConfig, const QJsonObject &authData,
                                 const QString &serverCountryCode, const QString &serviceProtocol,
                                 const ProtocolData &protocolData, QString &nativeConfig);

    ErrorCode revokeNativeConfig(const QJsonObject &apiConfig, const QJsonObject &authData,
                                 const QString &serverCountryCode, const QString &serviceProtocol);

    ErrorCode updateServiceFromTelegram(int serverIndex, const ProtocolData &protocolData);

    ErrorCode prepareVpnKeyExport(int serverIndex, QString &vpnKey);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody, bool isTestPurchase = false);

    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
};

#endif // SUBSCRIPTIONCONTROLLER_H

