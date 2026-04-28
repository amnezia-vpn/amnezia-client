#ifndef HTTPGATEWAYTRANSPORT_H
#define HTTPGATEWAYTRANSPORT_H

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QNetworkReply>
#include <QSslError>
#include <QString>
#include <QStringList>

#include "igatewaytransport.h"

namespace amnezia::transport
{

class HttpGatewayTransport : public IGatewayTransport
{
public:
    HttpGatewayTransport(const QString &endpoint,
                         bool isDevEnvironment,
                         int requestTimeoutMsecs,
                         bool isStrictKillSwitchEnabled);

    QString name() const override { return QStringLiteral("HTTP"); }

    amnezia::ErrorCode send(const QString &endpointTemplate,
                            const QByteArray &requestBody,
                            QByteArray &decryptedResponse,
                            const DecryptionHook &decryptionHook) override;

private:
    struct ReplyOutcome
    {
        QByteArray encryptedBody;
        QList<QSslError> sslErrors;
        QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
        QString errorString;
        int httpStatusCode = 0;
    };

    ReplyOutcome doPost(const QString &fullUrl, const QByteArray &requestBody);
    void applyKillSwitchAllowlist(const QString &host);
    QStringList fetchProxyUrls(const QByteArray &serviceHint);
    bool shouldBypass(const ReplyOutcome &outcome, const DecryptionResult &decrypted) const;

    QString m_endpoint;
    bool m_isDevEnvironment;
    int m_requestTimeoutMsecs;
    bool m_isStrictKillSwitchEnabled;

    static QMutex s_proxyMutex;
    static QString s_proxyUrl;
};

} // namespace amnezia::transport

#endif // HTTPGATEWAYTRANSPORT_H
