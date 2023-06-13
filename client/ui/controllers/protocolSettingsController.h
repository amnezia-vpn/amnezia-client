#ifndef PROTOCOLSETTINGSCONTROLLER_H
#define PROTOCOLSETTINGSCONTROLLER_H

#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class ProtocolSettingsController : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolSettingsController(const QSharedPointer<ServersModel> &serversModel,
                                        const QSharedPointer<ContainersModel> &containersModel,
                                        const std::shared_ptr<Settings> &settings,
                                        QObject *parent = nullptr);

public slots:
    QByteArray getOpenVpnConfig();

signals:

private:
    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;
};

#endif // PROTOCOLSETTINGSCONTROLLER_H
