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

    explicit AccountController(SecureAppSettingsRepository *appSettingsRepository);

    amnezia::ErrorCode requestEmailCode(const QString &email, const QString &idempotencyKey, Error &error, int &retryAfterSec);
    amnezia::ErrorCode consumeEmailCode(const QString &email, const QString &code, const QString &idempotencyKey,
                                        Session &session, Error &error, int &retryAfterSec);

    static Error classifyReply(const Reply &reply);
    static Session sessionFromResponse(const QJsonObject &response);

private:
    Reply post(const QString &endpoint, const QJsonObject &apiPayload);

    SecureAppSettingsRepository *m_appSettingsRepository;
};

#endif // ACCOUNTCONTROLLER_H
