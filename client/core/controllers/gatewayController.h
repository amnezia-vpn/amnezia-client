#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QFuture>
#include <QNetworkReply>
#include <QObject>
#include <QPair>
#include <QPromise>
#include <QSharedPointer>

#include "core/defs.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

class GatewayController : public QObject
{
    Q_OBJECT

public:
    explicit GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                               const bool isStrictKillSwitchEnabled, QObject *parent = nullptr);

    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> postAsync(const QString &endpoint, const QJsonObject apiPayload);

private:
    struct EncryptedRequestData
    {
        QNetworkRequest request;
        QByteArray requestBody;
        QByteArray key;
        QByteArray iv;
        QByteArray salt;
        amnezia::ErrorCode errorCode;
    };

    EncryptedRequestData prepareRequest(const QString &endpoint, const QJsonObject &apiPayload);

    QStringList getProxyUrls(const QString &serviceType, const QString &userCountryCode);
    bool shouldBypassProxy(const QNetworkReply::NetworkError &replyError, const QByteArray &responseBody, bool checkEncryption,
                           const QByteArray &key = "", const QByteArray &iv = "", const QByteArray &salt = "");
    void bypassProxy(const QString &endpoint, const QString &serviceType, const QString &userCountryCode,
                     std::function<QNetworkReply *(const QString &url)> requestFunction,
                     std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction);

    void getProxyUrlsAsync(const QStringList proxyStorageUrls, const int currentProxyStorageIndex,
                           std::function<void(const QStringList &)> onComplete);
    void getProxyUrlAsync(const QStringList proxyUrls, const int currentProxyIndex, std::function<void(const QString &)> onComplete);
    void bypassProxyAsync(
            const QString &endpoint, const QString &proxyUrl, EncryptedRequestData encRequestData,
            std::function<void(const QByteArray &, const QList<QSslError> &, QNetworkReply::NetworkError, const QString &, int)> onComplete);

    int m_requestTimeoutMsecs;
    QString m_gatewayEndpoint;
    bool m_isDevEnvironment = false;
    bool m_isStrictKillSwitchEnabled = false;

    inline static QString m_proxyUrl;
};

#endif // GATEWAYCONTROLLER_H
