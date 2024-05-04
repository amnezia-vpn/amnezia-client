#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"

#ifdef Q_OS_IOS
#include "platforms/ios/MobileUtils.h"
#endif

class ConnectionController;

class ApiController : public QObject
{
    Q_OBJECT

public:
    explicit ApiController(ConnectionController *connectionController);

public slots:
    void updateServerConfigFromApi(const QString &installationUuid, const QJsonObject &serverConfig,
                                   const std::function<void(bool updateConfig, QJsonObject config)> &cb);

private:
    ConnectionController *m_connectionController;
    MobileUtils m_mobileUtils;

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
