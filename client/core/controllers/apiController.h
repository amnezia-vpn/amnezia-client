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
    explicit ApiController(QObject *parent = nullptr);

public slots:
    void updateServerConfigFromApi(const QString &installationUuid, const int serverIndex, QJsonObject serverConfig);

signals:
    void errorOccurred(const QString &errorMessage);
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
    void processApiConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, QString &config);
};

#endif // APICONTROLLER_H
