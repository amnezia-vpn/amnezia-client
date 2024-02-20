#ifndef APICONTROLLER_H
#define APICONTROLLER_H

#include <QObject>

#include "configurators/openvpn_configurator.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class ApiController : public QObject
{
    Q_OBJECT

public:
    explicit ApiController(const QSharedPointer<ServersModel> &serversModel,
                           const QSharedPointer<ContainersModel> &containersModel, QObject *parent = nullptr);

public slots:
    void updateServerConfigFromApi();

    void clearApiConfig();

signals:
    void updateStarted();
    void updateFinished(bool isConfigUpdateStarted);
    void errorOccurred(const QString &errorMessage);

private:
    struct ApiPayloadData {
        OpenVpnConfigurator::ConnectionData certRequest;

        QString wireGuardClientPrivKey;
        QString wireGuardClientPubKey;
    };

    ApiPayloadData generateApiPayloadData(const QString &protocol);
    QJsonObject fillApiPayload(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData);
    void processApiConfig(const QString &protocol, const ApiController::ApiPayloadData &apiPayloadData, QString &config);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    bool m_isConfigUpdateStarted = false;
};

#endif // APICONTROLLER_H
