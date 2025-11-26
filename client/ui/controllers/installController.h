#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QProcess>

#include "containers/containers_defs.h"
#include "core/controllers/serversController.h"
#include "core/controllers/clientManagementController.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "core/defs.h"
#include "ui/models/containers_model.h"
#include "ui/models/protocols_model.h"
#include "ui/models/servers_model.h"

class InstallController : public QObject
{
    Q_OBJECT
public:
    explicit InstallController(ServersController* serversController,
                               ServersModel* serversModel, ContainersModel* containersModel,
                               ProtocolsModel* protocolsModel,
                               ClientManagementController* clientManagementController,
                               QAppSettingsRepository* appSettingsRepository,
                               const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);
    ~InstallController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto, int serverIndex);
    void setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);

    void scanServerForInstalledContainers(int serverIndex);

    void updateContainer(int serverIndex, QJsonObject config);

    void removeServer(int serverIndex);
    void rebootServer(int serverIndex);
    void removeAllContainers(int serverIndex);
    void removeContainer(int serverIndex);

    void removeApiConfig(int serverIndex);

    void clearCachedProfile(int serverIndex, QSharedPointer<ServerController> serverController = nullptr);

    QRegularExpression ipAddressPortRegExp();
    QRegularExpression ipAddressRegExp();

    void mountSftpDrive(int serverIndex, const QString &port, const QString &password, const QString &username);

    bool checkSshConnection(QSharedPointer<ServerController> serverController = nullptr);

    void setEncryptedPassphrase(QString passphrase);

    void addEmptyServer();

    bool isConfigValid();

signals:
    void installContainerFinished(const QString &finishMessage, bool isServiceInstall);
    void installServerFinished(const QString &finishMessage);

    void updateContainerFinished(const QString &message);

    void scanServerFinished(bool isInstalledContainerFound);

    void rebootServerFinished(const QString &finishedMessage);
    void removeServerFinished(const QString &finishedMessage);
    void removeAllContainersFinished(const QString &finishedMessage);
    void removeContainerFinished(const QString &finishedMessage);

    void installationErrorOccurred(ErrorCode errorCode);
    void wrongInstallationUser(const QString &message);

    void serverAlreadyExists(int serverIndex);

    void passphraseRequestStarted();
    void passphraseRequestFinished();

    void serverIsBusy(const bool isBusy);
    void cancelInstallation();

    void currentContainerUpdated();

    void cachedProfileCleared(const QString &message);
    void apiConfigRemoved(const QString &message);

    void noInstalledContainers();

    void profileCleared(const QJsonObject &config);

private:
    void installServer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                       const ServerCredentials &serverCredentials, const QSharedPointer<ServerController> &serverController,
                       QString &finishMessage);
    void installContainer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                          const ServerCredentials &serverCredentials, const QSharedPointer<ServerController> &serverController,
                          QString &finishMessage, int serverIndex);
    bool isServerAlreadyExists();

    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, const QSharedPointer<ServerController> &serverController,
                                            QMap<DockerContainer, QJsonObject> &installedContainers);
    bool isUpdateDockerContainerRequired(const DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);

    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ProtocolsModel* m_protocolModel;
    ClientManagementController* m_clientManagementController;
    QAppSettingsRepository* m_appSettingsRepository;

    std::shared_ptr<Settings> m_settings;

    ServerCredentials m_processedServerCredentials;

    QString m_privateKeyPassphrase;

#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLCONTROLLER_H
