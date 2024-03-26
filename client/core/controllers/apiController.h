#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"

class ApiController : public QObject
{
    Q_OBJECT

public:
    explicit ApiController(QObject *parent = nullptr);

public slots:
    ErrorCode updateServerConfigFromApi(QJsonObject &serverConfig);

private:
    struct ApiPayloadData {
        OpenVpnConfigurator::ConnectionData certRequest;

        QString wireGuardClientPrivKey;
        QString wireGuardClientPubKey;
    };

    ApiPayloadData generateApiPayloadData(const QString &protocol);
    QJsonObject fillApiPayload(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData);
    void processApiConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, QString &config);
};

#endif // APICONTROLLER_H
