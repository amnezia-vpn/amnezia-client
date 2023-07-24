#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QProcess>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class InstallController : public QObject
{
    Q_OBJECT
public:
    explicit InstallController(const QSharedPointer<ServersModel> &serversModel,
                               const QSharedPointer<ContainersModel> &containersModel,
                               const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);
    ~InstallController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto);
    void setCurrentlyInstalledServerCredentials(const QString &hostName, const QString &userName,
                                                const QString &secretData);
    void setShouldCreateServer(bool shouldCreateServer);

    void scanServerForInstalledContainers();

    void updateContainer(QJsonObject config);

    QRegularExpression ipAddressPortRegExp();

    void mountSftpDrive(const QString &port, const QString &password, const QString &username);

signals:
    void installContainerFinished(bool isInstalledContainerFound);
    void installServerFinished(bool isInstalledContainerFound);

    void updateContainerFinished();

    void scanServerFinished(bool isInstalledContainerFound);

    void installationErrorOccurred(QString errorMessage);

    void serverAlreadyExists(int serverIndex);

private:
    void installServer(DockerContainer container, QJsonObject &config);
    void installContainer(DockerContainer container, QJsonObject &config);
    bool isServerAlreadyExists();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

    ServerCredentials m_currentlyInstalledServerCredentials;

    bool m_shouldCreateServer;

    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
};

#endif // INSTALLCONTROLLER_H
