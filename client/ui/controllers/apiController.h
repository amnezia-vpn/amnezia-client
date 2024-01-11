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
    bool updateServerConfigFromApi();

signals:
    void errorOccurred(const QString &errorMessage);

private:
    QString genPublicKey(const QString &protocol);
    QString genCertificateRequest(const QString &protocol);

    void processCloudConfig(const QString &protocol, QString &config);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    OpenVpnConfigurator::ConnectionData m_certRequest;
};

#endif // APICONTROLLER_H
