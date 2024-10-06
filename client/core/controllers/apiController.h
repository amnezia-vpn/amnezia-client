#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

class ApiController : public QObject
{
    Q_OBJECT

public:
    explicit ApiController(const QString &gatewayEndpoint, bool isDevEnvironment, QObject *parent = nullptr);

public slots:
    void updateServerConfigFromApi(const QString &installationUuid, const int serverIndex, QJsonObject serverConfig);

    ErrorCode getServicesList(QByteArray &responseBody);
    ErrorCode getConfigForService(const QString &installationUuid, const QString &userCountryCode, const QString &serviceType,
                                  const QString &protocol, const QString &serverCountryCode, const QJsonObject &authData, QJsonObject &serverConfig);

signals:
    void errorOccurred(ErrorCode errorCode);
    void finished(const QJsonObject &config, const int serverIndex);

private:
    struct ApiPayloadData
    {
        OpenVpnConfigurator::ConnectionData certRequest;

        QString wireGuardClientPrivKey;
        QString wireGuardClientPubKey;
    };

    ApiPayloadData generateApiPayloadData(const QString &protocol);
    QJsonObject fillApiPayload(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData);
    void fillServerConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, const QByteArray &apiResponseBody,
                          QJsonObject &serverConfig);
    QStringList getProxyUrls();

    QString m_gatewayEndpoint;
    QStringList m_proxyUrls;
    bool m_isDevEnvironment = false;
};

#endif // APICONTROLLER_H
