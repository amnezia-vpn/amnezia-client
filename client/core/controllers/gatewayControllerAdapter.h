#ifndef GATEWAYCONTROLLERADAPTER_H
#define GATEWAYCONTROLLERADAPTER_H

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

namespace agw
{
class GatewayController;
}

class GatewayControllerAdapter : public QObject
{
    Q_OBJECT

public:
    // Qt-зеркало agw::api::GatewayRequest — app собирает поля (часть из apiV2/настроек), agw-типы наружу
    // не текут. Пустые поля SDK опускает (паритет с GatewayRequestData::toJsonObject).
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

    explicit GatewayControllerAdapter(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

    // --- Tier 2 (Шаг 6): типизированные методы поверх agw::api::* ---------------
    // sync-методы UI-поток не блокируют (работа на пуле + прокачка QEventLoop);
    // *Async — возвращают QFuture, исполняются целиком на пуле.
    amnezia::ErrorCode getServices(const QString &osVersion, const QString &appVersion, const QString &cliName,
                                   const QString &appLanguage, QJsonObject &servicesOut);

    // v1/trial. publicKey — WG pub key (awg) или xray uuid (vless), генерит app. На успехе serverConfigJsonOut —
    // распакованный конфиг (как qUncompress(config)); сборку ApiV2ServerConfig и персист делает app.
    amnezia::ErrorCode importTrial(const GatewayRequest &request, const QString &publicKey, const QString &email,
                                   QString &serverConfigJsonOut);

    // v1/revoke_config. Возвращает ErrorCode как есть (app трактует ApiNotFoundError как успех).
    amnezia::ErrorCode deactivateDevice(const GatewayRequest &request);

    // Результат import/resolveCaptcha. rawResponse — сырое тело (app сам распаковывает конфиг через
    // существующий extractServerConfigJsonFromResponse + updateApiConfigInJson). serverConfigJson — уже
    // распакованный конфиг от SDK (на этом пути не используется, но доступен).
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

    // v1/config. publicKey — WG pub key (awg) или xray uuid (vless). На капчу — captchaRequired + поля.
    ImportResult importService(const GatewayRequest &request, const QString &publicKey);

    // v1/config с captcha_id/solution (решение нормализуется в SDK). На повторную капчу — captchaRequired.
    ImportResult resolveImportCaptcha(const GatewayRequest &request, const QString &publicKey,
                                      const QString &captchaId, const QString &captchaSolution);

    // v1/config для существующего сервиса (authData в request). isConnectEvent → is_connect_event.
    ImportResult updateService(const GatewayRequest &request, const QString &publicKey, bool isConnectEvent);

    // v1/account_info → сырое тело JSON (app сам разбирает нужные поля).
    amnezia::ErrorCode getAccountInfoRaw(const GatewayRequest &request, const QString &cliVersion,
                                         const QString &subscriptionStatus, QByteArray &rawJsonOut);

    // v1/native_config → нативный конфиг текстом (подстановку WG-ключа делает app).
    amnezia::ErrorCode exportNativeConfig(const GatewayRequest &request, const QString &publicKey,
                                          QString &nativeConfigOut);

    // v1/revoke_native_config. ApiNotFoundError трактуется как успех в app.
    amnezia::ErrorCode revokeNativeConfig(const GatewayRequest &request);

    // Результат v1/subscriptions (App Store). vpnKey — без "vpn://"; crc — qChecksum/CRC-16.
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
    // Исполняет work на фоновом потоке, прокачивая локальный QEventLoop (как post). UI остаётся живым.
    void runBlocking(const std::function<void()> &work);

    std::shared_ptr<agw::GatewayController> m_controller;
};

#endif
