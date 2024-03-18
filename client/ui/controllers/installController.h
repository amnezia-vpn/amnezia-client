#ifndef INSTALLCONTROLLER_H
#define INSTALLCONTROLLER_H

#include <QObject>
#include <QProcess>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/protocols_model.h"
#include "ui/models/servers_model.h"

class InstallController : public QObject
{
    Q_OBJECT
public:
    explicit InstallController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                               const QSharedPointer<ProtocolsModel> &protocolsModel,
                               const QSharedPointer<ClientManagementModel> &clientManagementModel,
                               const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);
    ~InstallController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto);
    void setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);
    void setShouldCreateServer(bool shouldCreateServer);

    void scanServerForInstalledContainers();

    void updateContainer(QJsonObject config);

    void removeProcessedServer();
    void rebootProcessedServer();
    void removeAllContainers();
    void removeProcessedContainer();

    void removeApiConfig();

    void clearCachedProfile();

    QRegularExpression ipAddressPortRegExp();
    QRegularExpression ipAddressRegExp();

    void mountSftpDrive(const QString &port, const QString &password, const QString &username);

    bool checkSshConnection();

    void setEncryptedPassphrase(QString passphrase);

    void addEmptyServer();

signals:
    void installContainerFinished(const QString &finishMessage, bool isServiceInstall);
    void installServerFinished(const QString &finishMessage);

    void updateContainerFinished(const QString &message);

    void scanServerFinished(bool isInstalledContainerFound);

    void rebootProcessedServerFinished(const QString &finishedMessage);
    void removeProcessedServerFinished(const QString &finishedMessage);
    void removeAllContainersFinished(const QString &finishedMessage);
    void removeProcessedContainerFinished(const QString &finishedMessage);

    void installationErrorOccurred(const QString &errorMessage);

    void serverAlreadyExists(int serverIndex);

    void passphraseRequestStarted();
    void passphraseRequestFinished();

    void serverIsBusy(const bool isBusy);
    void cancelInstallation();

    void currentContainerUpdated();

    void cachedProfileCleared(const QString &message);

private:
    void installServer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                       const ServerCredentials &serverCredentials, QString &finishMessage);
    void installContainer(const DockerContainer container, const QMap<DockerContainer, QJsonObject> &installedContainers,
                          const ServerCredentials &serverCredentials, QString &finishMessage);
    bool isServerAlreadyExists();

    ErrorCode getAlreadyInstalledContainers(const ServerCredentials &credentials, QMap<DockerContainer, QJsonObject> &installedContainers);
    bool isUpdateDockerContainerRequired(const DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ProtocolsModel> m_protocolModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;
    std::shared_ptr<Settings> m_settings;

    ServerCredentials m_processedServerCredentials;

    bool m_shouldCreateServer;

    QString m_privateKeyPassphrase;

#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLCONTROLLER_H
