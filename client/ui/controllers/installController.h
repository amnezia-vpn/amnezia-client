#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>

#include "core/defs.h"
#include "containers/containers_defs.h"
#include "ui/models/servers_model.h"
#include "ui/models/containers_model.h"

class InstallController : public QObject
{
    Q_OBJECT
public:
    explicit InstallController(const QSharedPointer<ServersModel> &serversModel,
                               const QSharedPointer<ContainersModel> &containersModel,
                               const std::shared_ptr<Settings> &settings,
                               QObject *parent = nullptr);

public slots:
    ErrorCode install(DockerContainer container, int port, TransportProto transportProto);
    void setCurrentlyInstalledServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);
    void setShouldCreateServer(bool shouldCreateServer);

signals:
    void installContainerFinished();
private:
    ErrorCode installServer(DockerContainer container, QJsonObject& config);
    ErrorCode installContainer(DockerContainer container, QJsonObject& config);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

    ServerCredentials m_currentlyInstalledServerCredentials;

    bool m_shouldCreateServer;
};

#endif // INSTALLCONTROLLER_H
