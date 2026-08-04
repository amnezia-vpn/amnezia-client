#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <functional>
#include <memory>

#include <QByteArray>
#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>

#include "core/utils/errorCodes.h"

struct amnezia_gateway_api_client;

class SecureAppSettingsRepository;

class GatewayController : public QObject
{
    Q_OBJECT

public:
    struct GatewayRequest
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
    };

    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                               QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);

    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

    amnezia::ErrorCode getServices(const QString &osVersion, const QString &appVersion, const QString &cliName,
                                   const QString &appLanguage, QJsonObject &servicesOut);

    amnezia::ErrorCode importTrial(const GatewayRequest &request, const QString &publicKey, const QString &email,
                                   QString &serverConfigJsonOut);

    amnezia::ErrorCode deactivateDevice(const GatewayRequest &request);

    struct ImportResult
    {
        amnezia::ErrorCode error = amnezia::ErrorCode::NoError;
        bool captchaRequired = false;
        QString captchaId;
        QString captchaImageBase64;
        QString hint;
        QString serverConfigJson;
        QByteArray rawResponse;
    };

    ImportResult importService(const GatewayRequest &request, const QString &publicKey);

    ImportResult resolveImportCaptcha(const GatewayRequest &request, const QString &publicKey,
                                      const QString &captchaId, const QString &captchaSolution);

    ImportResult updateService(const GatewayRequest &request, const QString &publicKey, bool isConnectEvent);

    amnezia::ErrorCode getAccountInfoRaw(const GatewayRequest &request, const QString &cliVersion,
                                         const QString &subscriptionStatus, QByteArray &rawJsonOut);

    amnezia::ErrorCode exportNativeConfig(const GatewayRequest &request, const QString &publicKey,
                                          QString &nativeConfigOut);

    amnezia::ErrorCode revokeNativeConfig(const GatewayRequest &request);

    struct AppStoreResult
    {
        amnezia::ErrorCode error = amnezia::ErrorCode::NoError;
        QString serverConfigJson;
        QString vpnKey;
        quint16 crc = 0;
    };

    AppStoreResult importServiceFromAppStore(const GatewayRequest &request, const QString &publicKey,
                                             const QString &transactionId);

    QFuture<QPair<amnezia::ErrorCode, QJsonArray>> getNewsAsync(const QString &locale,
                                                               const QStringList &userCountryCodes,
                                                               const QStringList &serviceTypes);

    QFuture<QPair<amnezia::ErrorCode, QString>> getUpdaterEndpointAsync(const QString &cliVersion,
                                                                       const QString &osVersion,
                                                                       const QString &installationUuid);

    QFuture<QPair<amnezia::ErrorCode, QString>> getRenewalLinkAsync(const GatewayRequest &request,
                                                                   const QString &cliVersion,
                                                                   const QString &subscriptionStatus);

private:
    void runBlocking(const std::function<void()> &work);

    std::shared_ptr<amnezia_gateway_api_client> m_controller;
};

#endif
