#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QNetworkReply>
#include <QObject>

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

    amnezia::ErrorCode get(const QString &endpoint, QByteArray &responseBody);
    amnezia::ErrorCode post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody);

private:
    QStringList getProxyUrls();
    bool shouldBypassProxy(QNetworkReply *reply, const QByteArray &responseBody, bool checkEncryption, const QByteArray &key = "",
                           const QByteArray &iv = "", const QByteArray &salt = "");
    void bypassProxy(const QString &endpoint, QNetworkReply *reply, std::function<QNetworkReply *(const QString &url)> requestFunction,
                     std::function<bool(QNetworkReply *reply, const QList<QSslError> &sslErrors)> replyProcessingFunction);

    int m_requestTimeoutMsecs;
    QString m_gatewayEndpoint;
    bool m_isDevEnvironment = false;
    bool m_isStrictKillSwitchEnabled = false;
};

#endif // GATEWAYCONTROLLER_H
