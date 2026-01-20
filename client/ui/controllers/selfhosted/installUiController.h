#ifndef INSTALLUICONTROLLER_H
#define INSTALLUICONTROLLER_H

#include <QObject>
#include <QProcess>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/selfhosted/usersController.h"
#include "core/controllers/selfhosted/installController.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "ui/models/containersModel.h"
#include "ui/models/protocolsModel.h"
#include "ui/models/serversModel.h"

class InstallUiController : public QObject
{
    Q_OBJECT
public:
    explicit InstallUiController(InstallController* installController,
                               ServersController* serversController,
                               SettingsController* settingsController,
                               ServersModel* serversModel, ContainersModel* containersModel,
                               ProtocolsModel* protocolsModel,
                               UsersController* usersController,
                               QObject *parent = nullptr);
    ~InstallUiController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto, int serverIndex);
    void setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);

    void scanServerForInstalledContainers(int serverIndex);

    void updateContainer(int serverIndex, QJsonObject config);

    void removeServer(int serverIndex);
    void rebootServer(int serverIndex);
    void removeAllContainers(int serverIndex);
    void removeContainer(int serverIndex);

    void clearCachedProfile(int serverIndex);

    QRegularExpression ipAddressPortRegExp();
    QRegularExpression ipAddressRegExp();

    void mountSftpDrive(int serverIndex, const QString &port, const QString &password, const QString &username);

    bool checkSshConnection(SshSession* sshSession = nullptr);

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

    InstallController* m_installController;
    ServersController* m_serversController;
    SettingsController* m_settingsController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    ProtocolsModel* m_protocolModel;
    UsersController* m_usersController;

    ServerCredentials m_processedServerCredentials;

    QString m_privateKeyPassphrase;
};

#endif // INSTALLUICONTROLLER_H
