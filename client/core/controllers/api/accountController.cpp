#include "accountController.h"

#include <QDebug>
#include <QJsonDocument>

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/gatewayPayloadBuilder.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"

using namespace amnezia;

namespace
{
    constexpr QLatin1String emailCodeEndpoint("%1v1/email_code");
    constexpr QLatin1String emailCodeConsumeEndpoint("%1v1/email_code_consume");

    constexpr int httpStatusNotFound = 404;
    constexpr int httpStatusConflict = 409;
    constexpr int httpStatusTooManyRequests = 429;
}

AccountController::AccountController(SecureAppSettingsRepository *appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode AccountController::requestEmailCode(const QString &email, const QString &idempotencyKey, Error &error, int &retryAfterSec)
{
    const QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                           .addField(apiDefs::key::email, email)
                                           .addField(apiDefs::key::idempotencyKey, idempotencyKey)
                                           .build();

    const Reply reply = post(emailCodeEndpoint, apiPayload);
    retryAfterSec = reply.retryAfterSec;
    error = classifyReply(reply);
    if (error == Error::InvalidCode || error == Error::Conflict) {
        error = Error::Failed;
    }
    if (error == Error::RateLimited) {
        return ErrorCode::ApiRateLimitError;
    }
    if (error != Error::None) {
        return ErrorCode::ApiEmailCodeSendError;
    }
    return ErrorCode::NoError;
}

ErrorCode AccountController::consumeEmailCode(const QString &email, const QString &code, const QString &idempotencyKey,
                                                  Session &session, Error &error, int &retryAfterSec)
{
    const QJsonObject apiPayload = GatewayPayloadBuilder(m_appSettingsRepository)
                                           .addField(apiDefs::key::email, email)
                                           .addField(apiDefs::key::code, code)
                                           .addField(apiDefs::key::idempotencyKey, idempotencyKey)
                                           .build();

    const Reply reply = post(emailCodeConsumeEndpoint, apiPayload);
    retryAfterSec = reply.retryAfterSec;
    error = classifyReply(reply);
    switch (error) {
    case Error::None: break;
    case Error::RateLimited: return ErrorCode::ApiRateLimitError;
    case Error::InvalidCode: return ErrorCode::ApiEmailCodeInvalidError;
    default: return ErrorCode::ApiAccountSessionError;
    }

    session = sessionFromResponse(reply.body);
    if (!session.isValid()) {
        qWarning().noquote() << "[Account] Session response has no access token, keys =" << reply.body.keys();
        error = Error::Failed;
        return ErrorCode::ApiAccountSessionError;
    }
    qDebug().noquote() << "[Account] Session issued, expires in =" << session.expiresInSec
                       << "password must change =" << session.passwordMustChange;
    return ErrorCode::NoError;
}

AccountController::Error AccountController::classifyReply(const Reply &reply)
{
    switch (reply.httpStatus) {
    case httpStatusNotFound: return Error::InvalidCode;
    case httpStatusConflict: return Error::Conflict;
    case httpStatusTooManyRequests: return Error::RateLimited;
    default: break;
    }
    if (reply.httpStatus >= 400) {
        return Error::Failed;
    }
    if (reply.errorCode == ErrorCode::NoError) {
        return Error::None;
    }
    return reply.httpStatus == 0 ? Error::NoNetwork : Error::Failed;
}

AccountController::Session AccountController::sessionFromResponse(const QJsonObject &response)
{
    Session session;
    session.userId = response.value(apiDefs::key::userId).toString();
    session.accessToken = response.value(apiDefs::key::accessToken).toString();
    session.refreshToken = response.value(apiDefs::key::refreshToken).toString();
    session.expiresInSec = response.value(apiDefs::key::expiresIn).toInt();
    session.passwordMustChange = response.value(apiDefs::key::passwordMustChange).toBool();
    return session;
}

AccountController::Reply AccountController::post(const QString &endpoint, const QJsonObject &apiPayload)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled(),
                                        m_appSettingsRepository);

    Reply reply;
    QByteArray responseBody;
    reply.errorCode = gatewayController.post(endpoint, apiPayload, responseBody);
    reply.body = QJsonDocument::fromJson(responseBody).object();
    reply.httpStatus = reply.body.value(apiDefs::key::httpStatus).toInt(reply.body.value(apiDefs::key::status).toInt(0));
    reply.retryAfterSec = reply.body.value(apiDefs::key::retryAfter).toInt();

    const QString requestId = reply.body.value(apiDefs::key::requestId).toString();
    if (reply.errorCode != ErrorCode::NoError || reply.httpStatus >= 400) {
        qWarning().noquote() << "[Account]" << endpoint << "failed, errorCode =" << static_cast<int>(reply.errorCode)
                             << "http status =" << reply.httpStatus << "retry after =" << reply.retryAfterSec
                             << "detail =" << reply.body.value(QLatin1String("title")).toString()
                             << reply.body.value(QLatin1String("detail")).toString()
                             << "request id =" << requestId;
    } else {
        qDebug().noquote() << "[Account]" << endpoint << "done, request id =" << requestId;
    }
    return reply;
}
