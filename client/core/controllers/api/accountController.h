#ifndef ACCOUNTCONTROLLER_H
#define ACCOUNTCONTROLLER_H

#include <QJsonObject>
#include <QString>

#include "core/utils/errorCodes.h"

class SecureAppSettingsRepository;

class AccountController
{
public:
    enum class Error {
        None,
        NoNetwork,
        RateLimited,
        CaptchaRequired,
        InvalidCode,
        Conflict,
        Failed
    };

    struct Reply
    {
        amnezia::ErrorCode errorCode = amnezia::ErrorCode::NoError;
        int httpStatus = 0;
        int retryAfterSec = 0;
        QJsonObject body;
    };

    struct Session
    {
        QString userId;
        QString accessToken;
        QString refreshToken;
        int expiresInSec = 0;
        bool passwordMustChange = false;

        bool isValid() const { return !accessToken.isEmpty(); }
    };

    struct Captcha
    {
        QString id;
        QString imageBase64;
        QString hint;

        bool isRequired() const { return !id.isEmpty(); }
    };

    struct EmailCodeRequest
    {
        QString email;
        QString userCountryCode;
        QString idempotencyKey;
        QString captchaId;
        QString captchaSolution;
        bool refreshCaptcha = false;
    };

    explicit AccountController(SecureAppSettingsRepository *appSettingsRepository);

    amnezia::ErrorCode requestEmailCode(const EmailCodeRequest &request, Captcha &captcha, Error &error, int &retryAfterSec);
    amnezia::ErrorCode consumeEmailCode(const QString &email, const QString &code, const QString &idempotencyKey,
                                        Session &session, Error &error, int &retryAfterSec);

    static Error classifyReply(const Reply &reply);
    static Session sessionFromResponse(const QJsonObject &response);
    static Captcha captchaFromResponse(const QJsonObject &response);

private:
    Reply post(const QString &endpoint, const QJsonObject &apiPayload);

    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // ACCOUNTCONTROLLER_H
