#include <QJsonObject>
#include <QTest>

#include "core/controllers/api/accountController.h"

class TestAccountReplies : public QObject
{
    Q_OBJECT

private slots:
    void sessionFromDocumentedResponse();
    void sessionWithoutAccessTokenIsInvalid();
    void sessionFromEmptyResponseIsInvalid();

    void classifyReply_data();
    void classifyReply();
};

void TestAccountReplies::sessionFromDocumentedResponse()
{
    QJsonObject response;
    response[QStringLiteral("user_id")] = QStringLiteral("3fa85f64-5717-4562-b3fc-2c963f66afa6");
    response[QStringLiteral("access_token")] = QStringLiteral("access");
    response[QStringLiteral("refresh_token")] = QStringLiteral("refresh");
    response[QStringLiteral("expires_in")] = 3600;
    response[QStringLiteral("password_must_change")] = true;

    const AccountController::Session session = AccountController::sessionFromResponse(response);

    QVERIFY(session.isValid());
    QCOMPARE(session.userId, QStringLiteral("3fa85f64-5717-4562-b3fc-2c963f66afa6"));
    QCOMPARE(session.accessToken, QStringLiteral("access"));
    QCOMPARE(session.refreshToken, QStringLiteral("refresh"));
    QCOMPARE(session.expiresInSec, 3600);
    QCOMPARE(session.passwordMustChange, true);
}

void TestAccountReplies::sessionWithoutAccessTokenIsInvalid()
{
    QJsonObject response;
    response[QStringLiteral("user_id")] = QStringLiteral("3fa85f64-5717-4562-b3fc-2c963f66afa6");
    response[QStringLiteral("access_token")] = QString();

    const AccountController::Session session = AccountController::sessionFromResponse(response);

    QVERIFY(!session.isValid());
    QCOMPARE(session.userId, QStringLiteral("3fa85f64-5717-4562-b3fc-2c963f66afa6"));
}

void TestAccountReplies::sessionFromEmptyResponseIsInvalid()
{
    const AccountController::Session session = AccountController::sessionFromResponse(QJsonObject());

    QVERIFY(!session.isValid());
    QVERIFY(session.userId.isEmpty());
    QCOMPARE(session.expiresInSec, 0);
    QCOMPARE(session.passwordMustChange, false);
}

void TestAccountReplies::classifyReply_data()
{
    QTest::addColumn<int>("errorCode");
    QTest::addColumn<int>("httpStatus");
    QTest::addColumn<int>("expected");

    using Error = AccountController::Error;
    using amnezia::ErrorCode;
    QTest::newRow("ok") << int(ErrorCode::NoError) << 200 << int(Error::None);
    QTest::newRow("ok without status") << int(ErrorCode::NoError) << 0 << int(Error::None);
    QTest::newRow("404 unknown code") << int(ErrorCode::ApiNotFoundError) << 404 << int(Error::InvalidCode);
    QTest::newRow("409 race") << int(ErrorCode::ApiConfigLimitError) << 409 << int(Error::Conflict);
    QTest::newRow("429 rate limited") << int(ErrorCode::ApiRateLimitError) << 429 << int(Error::RateLimited);
    QTest::newRow("400 validation") << int(ErrorCode::ApiConfigDownloadError) << 400 << int(Error::Failed);
    QTest::newRow("422 idempotency mismatch") << int(ErrorCode::ApiConfigDownloadError) << 422 << int(Error::Failed);
    QTest::newRow("500 server error") << int(ErrorCode::ApiConfigDownloadError) << 500 << int(Error::Failed);
    QTest::newRow("timeout") << int(ErrorCode::ApiConfigTimeoutError) << 0 << int(Error::NoNetwork);
    QTest::newRow("ssl") << int(ErrorCode::ApiConfigSslError) << 0 << int(Error::NoNetwork);
    QTest::newRow("429 in body only") << int(ErrorCode::NoError) << 429 << int(Error::RateLimited);
    QTest::newRow("404 in body only") << int(ErrorCode::NoError) << 404 << int(Error::InvalidCode);
    QTest::newRow("400 in body only") << int(ErrorCode::NoError) << 400 << int(Error::Failed);
}

void TestAccountReplies::classifyReply()
{
    QFETCH(int, errorCode);
    QFETCH(int, httpStatus);
    QFETCH(int, expected);

    AccountController::Reply reply;
    reply.errorCode = static_cast<amnezia::ErrorCode>(errorCode);
    reply.httpStatus = httpStatus;

    QCOMPARE(int(AccountController::classifyReply(reply)), expected);
}

QTEST_GUILESS_MAIN(TestAccountReplies)
#include "testAccountReplies.moc"
