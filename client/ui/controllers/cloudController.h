#ifndef CLOUDCONTROLLER_H
#define CLOUDCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

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
    QString genPublicKey(ServiceTypeId serviceTypeId);
    QString genCertificateRequest(ServiceTypeId serviceTypeId);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
};

#endif // CLOUDCONTROLLER_H
