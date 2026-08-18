#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <functional>

#include <QFuture>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QPromise>
#include <QSharedPointer>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

class SecureAppSettingsRepository;

class GatewayController : public QObject
{
    Q_OBJECT

public:
    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                               QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject &apiPayload,
                                                            QNetworkReply **activeReplyOut = nullptr,
                                                            const QSharedPointer<GatewayController> &keepAlive = {});

private:
    struct EncryptedRequestData
    {
        QNetworkRequest request;
        QByteArray requestBody;
        QByteArray key;
        QByteArray iv;
        QByteArray salt;
        amnezia::ErrorCode errorCode;
        bool isPlaintextLocalGateway = false;
    };

    struct DecryptionResult
    {
        QByteArray decryptedBody;
        bool isDecryptionSuccessful;
    };

    EncryptedRequestData prepareRequest(const QString &endpoint, const QJsonObject &apiPayload);
    DecryptionResult tryDecryptResponseBody(const QByteArray &encryptedResponseBody, QNetworkReply::NetworkError replyError,
                                            const QByteArray &key, const QByteArray &iv, const QByteArray &salt);
    DecryptionResult resolveResponseBody(const QByteArray &responseBody, QNetworkReply::NetworkError replyError, const QByteArray &key,
                                         const QByteArray &iv, const QByteArray &salt);

    QStringList getProxyUrls(const QString &serviceType, const QString &userCountryCode);
    bool shouldBypassProxy(const QNetworkReply::NetworkError &replyError, const QByteArray &decryptedResponseBody, bool isDecryptionSuccessful);
    void bypassProxy(const QString &endpoint, const QString &serviceType, const QString &userCountryCode,
                     std::function<QNetworkReply *(const QString &url)> requestFunction,
                     std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction);

    void getProxyUrlsAsync(const QSharedPointer<GatewayController> &life, const QStringList &proxyStorageUrls, int currentProxyStorageIndex,
                           const QString &proxyUrlsCacheKey, const std::function<void(const QStringList &)> &onComplete);
    void getProxyUrlAsync(const QSharedPointer<GatewayController> &life, const QStringList &proxyUrls, int currentProxyIndex,
                          const std::function<void(const QString &)> &onComplete);
    void bypassProxyAsync(
            const QSharedPointer<GatewayController> &life, const QString &endpoint, const QString &proxyUrl, const EncryptedRequestData &encRequestData,
            const std::function<void(const QByteArray &, bool, const QList<QSslError> &, QNetworkReply::NetworkError, const QString &, int)> &onComplete);

    int m_requestTimeoutMsecs;
    QString m_gatewayEndpoint;
    bool m_isDevEnvironment = false;
    bool m_isStrictKillSwitchEnabled = false;
    SecureAppSettingsRepository *m_appSettingsRepository = nullptr;

    inline static QString m_proxyUrl;
};

#endif // GATEWAYCONTROLLER_H
