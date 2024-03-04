#ifndef VPNCONFIGIRATIONSCONTROLLER_H
#define VPNCONFIGIRATIONSCONTROLLER_H

#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "settings.h"

class VpnConfigirationsController : public QObject
{
    Q_OBJECT
public:
    explicit VpnConfigirationsController(const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

public slots:
    ErrorCode createProtocolConfigForContainer(const ServerCredentials &credentials, const DockerContainer container,
                                               QJsonObject &containerConfig, QString &clientId);
    ErrorCode createProtocolConfigString(const int serverIndex, const ServerCredentials &credentials,
                                         const DockerContainer container, const QJsonObject &containerConfig,
                                         QString &clientId, QString &protocolConfigString);

signals:

private:
    std::shared_ptr<Settings> m_settings;
};

#endif // VPNCONFIGIRATIONSCONTROLLER_H
