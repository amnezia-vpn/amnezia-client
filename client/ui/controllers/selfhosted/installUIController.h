#ifndef INSTALLUICONTROLLER_H
#define INSTALLUICONTROLLER_H

#include <QObject>
#include <QProcess>

#include "core/models/containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/selfhosted/clientManagementModel.h"
#include "ui/models/containers_model.h"
#include "ui/models/protocols_model.h"
#include "ui/models/servers_model.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/controllers/selfhosted/clientManagementController.h"

class ApiConfigsController;

class InstallUIController : public QObject
{
    Q_OBJECT
public:
    explicit InstallUIController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                 const QSharedPointer<ProtocolsModel> &protocolsModel,
                                 const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                 QSharedPointer<InstallController> coreInstallController,
                                 QSharedPointer<ApiConfigsController> apiConfigsCoreController,
                                 QSharedPointer<ClientManagementController> clientManagementController,
                                 QObject *parent = nullptr);
    ~InstallUIController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto);
    void setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);
    void setShouldCreateServer(bool shouldCreateServer);

    void scanServerForInstalledContainers();

    void updateContainer();

    void removeProcessedServer();
    void rebootProcessedServer();
    void removeAllContainers();
    void removeProcessedContainer();

    void removeApiConfig(const int serverIndex);

    Q_INVOKABLE void clearCachedProfile();
    Q_INVOKABLE void mountSftpDrive(const QString &port, const QString &password, const QString &username);

    Q_INVOKABLE QRegularExpression ipAddressPortRegExp();
    Q_INVOKABLE QRegularExpression ipAddressRegExp();
    Q_INVOKABLE bool checkSshConnection();

    void setEncryptedPassphrase(QString passphrase);

    void addEmptyServer();

    bool isConfigValid();

signals:
    void installContainerFinished(const QString &finishMessage, bool isServiceInstall);
    void installServerFinished(const QString &finishMessage);

    void updateContainerFinished(const QString &message);

    void scanServerFinished(bool isInstalledContainerFound);

    void rebootProcessedServerFinished(const QString &finishedMessage);
    void removeProcessedServerFinished(const QString &finishedMessage);
    void removeAllContainersFinished(const QString &finishedMessage);
    void removeProcessedContainerFinished(const QString &finishedMessage);

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

private:
    bool isServerAlreadyExists();

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    QSharedPointer<ProtocolsModel> m_protocolModel;
    QSharedPointer<ClientManagementModel> m_clientManagementModel;
    QSharedPointer<InstallController> m_coreInstallController;
    QSharedPointer<ApiConfigsController> m_apiConfigsCoreController;
    QSharedPointer<ClientManagementController> m_clientManagementController;

    ServerCredentials m_processedServerCredentials;

    bool m_shouldCreateServer;

    QString m_privateKeyPassphrase;

#ifndef Q_OS_IOS
    QList<QSharedPointer<QProcess>> m_sftpMountProcesses;
#endif
};

#endif // INSTALLUICONTROLLER_H
