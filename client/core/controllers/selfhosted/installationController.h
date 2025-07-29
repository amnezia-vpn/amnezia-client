#ifndef INSTALLATIONCONTROLLER_H
#define INSTALLATIONCONTROLLER_H

#include <QObject>
#include <QFuture>
#include <QSharedPointer>

#include "core/defs.h"
#include "core/models/containers/containers_defs.h"
#include "core/models/servers/serverConfig.h"
#include "serverController.h"

class Settings;

using namespace amnezia;

class InstallationController : public QObject
{
    Q_OBJECT

public:
    explicit InstallationController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QFuture<ErrorCode> installContainer(const QSharedPointer<ServerConfig> &serverConfig, 
                                      DockerContainer container, 
                                      const QJsonObject &containerConfig);

    QFuture<ErrorCode> scanServerForInstalledContainers(const QSharedPointer<ServerConfig> &serverConfig);

    QFuture<ErrorCode> removeContainer(const QSharedPointer<ServerConfig> &serverConfig, 
                                     DockerContainer container);

    QFuture<ErrorCode> updateContainer(const QSharedPointer<ServerConfig> &serverConfig, 
                                     DockerContainer container, 
                                     const QJsonObject &containerConfig);

    QFuture<ErrorCode> rebootServer(const QSharedPointer<ServerConfig> &serverConfig);

    QFuture<ErrorCode> removeAllContainers(const QSharedPointer<ServerConfig> &serverConfig);

    QFuture<bool> checkSshConnection(const QSharedPointer<ServerConfig> &serverConfig);

    QFuture<ErrorCode> mountSftpDrive(const QSharedPointer<ServerConfig> &serverConfig, 
                                    const QString &port, 
                                    const QString &password, 
                                    const QString &username);

signals:
    void installationFinished(const QString &finishMessage, bool isServiceInstall);
    void installationProgress(const QString &message);
    void installationError(ErrorCode errorCode);

private:
    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &serverCredentials,
                                          const QSharedPointer<ServerController> &serverController,
                                          QMap<DockerContainer, QJsonObject> &installedContainers);

    std::shared_ptr<Settings> m_settings;
};

#endif // INSTALLATIONCONTROLLER_H 
