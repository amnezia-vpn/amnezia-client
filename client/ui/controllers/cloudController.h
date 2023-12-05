#ifndef CLOUDCONTROLLER_H
#define CLOUDCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"
#include "configurators/openvpn_configurator.h"

class CloudController : public QObject
{
    Q_OBJECT

    enum ServiceTypeId
    {
        AmneziaFreeRuWG = 0,
        AmneziaFreeRuCloak,
        AmneziaFreeRuAWG,
        AmneziaFreeRuReverseWG,
        AmneziaFreeRuReverseCloak,
        AmneziaFreeRuReverseAWG

    };

public:
    explicit CloudController(const QSharedPointer<ServersModel> &serversModel,
                             const QSharedPointer<ContainersModel> &containersModel, QObject *parent = nullptr);

public slots:
    bool updateServerConfigFromCloud();

signals:
    void errorOccurred(const QString &errorMessage);
    void serverConfigUpdated();

private:
    QString genPublicKey(const QString &protocol);
    QString genCertificateRequest(const QString &protocol);

    void processCloudConfig(const QString &protocol, QString &config);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;

    OpenVpnConfigurator::ConnectionData m_certRequest;
};

#endif // CLOUDCONTROLLER_H
