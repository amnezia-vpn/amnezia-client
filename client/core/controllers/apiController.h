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
    explicit ApiController(const QString &gatewayEndpoint, QObject *parent = nullptr);

public slots:
    void updateServerConfigFromApi(const QString &installationUuid, const int serverIndex, QJsonObject serverConfig);

    ErrorCode getServicesList(QByteArray &responseBody);
    ErrorCode getConfigForService(const QString &installationUuid, const QString &userCountryCode, const QString &serviceType,
                                  const QString &protocol, const QString &serverCountryCode, QJsonObject &serverConfig);

signals:
    void errorOccurred(ErrorCode errorCode);
    void configUpdated(const bool updateConfig, const QJsonObject &config, const int serverIndex);

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

    QString m_gatewayEndpoint;
};

#endif // APICONTROLLER_H
