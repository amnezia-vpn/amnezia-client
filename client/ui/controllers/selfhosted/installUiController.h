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
#include "core/models/containerConfig.h"
#include "ui/models/protocolsModel.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "ui/models/protocols/openvpnConfigModel.h"
#include "ui/models/protocols/xrayConfigModel.h"
#ifdef Q_OS_WINDOWS
#include "ui/models/protocols/ikev2ConfigModel.h"
#endif
#include "ui/models/services/sftpConfigModel.h"
#include "ui/models/services/socks5ProxyConfigModel.h"
#include "ui/models/services/torConfigModel.h"
#include "core/models/protocols/sftpProtocolConfig.h"
#include "core/models/protocols/socks5ProxyProtocolConfig.h"

class InstallUiController : public QObject
{
    Q_OBJECT
public:
    explicit InstallUiController(InstallController* installController,
                               ServersController* serversController,
                               SettingsController* settingsController,
                               ProtocolsModel* protocolsModel,
                               UsersController* usersController,
                               AwgConfigModel* awgConfigModel,
                               WireGuardConfigModel* wireGuardConfigModel,
                               OpenVpnConfigModel* openVpnConfigModel,
                               XrayConfigModel* xrayConfigModel,
                               TorConfigModel* torConfigModel,
#ifdef Q_OS_WINDOWS
                               Ikev2ConfigModel* ikev2ConfigModel,
#endif
                               SftpConfigModel* sftpConfigModel,
                               Socks5ProxyConfigModel* socks5ConfigModel,
                               QObject *parent = nullptr);
    ~InstallUiController();

public slots:
    void install(DockerContainer container, int port, TransportProto transportProto, int serverIndex);
    void setProcessedServerCredentials(const QString &hostName, const QString &userName, const QString &secretData);
    void clearProcessedServerCredentials();

    void scanServerForInstalledContainers(int serverIndex);

    void updateContainer(int serverIndex, int containerIndex, int protocolIndex);

    void removeServer(int serverIndex);
    void rebootServer(int serverIndex);
    void removeAllContainers(int serverIndex);
    void removeContainer(int serverIndex, int containerIndex);

    void clearCachedProfile(int serverIndex, int containerIndex);

    QRegularExpression ipAddressRegExp();

    void mountSftpDrive(int serverIndex, const QString &port, const QString &password, const QString &username);

    bool checkSshConnection();

    void setEncryptedPassphrase(QString passphrase);

    void addEmptyServer();

    void validateConfig();
    
    Q_INVOKABLE void updateProtocols(int serverIndex, int containerIndex);
    
    void openServerSettings(int serverIndex, int containerIndex, int protocolIndex);
    void openClientSettings(int serverIndex, int containerIndex, int protocolIndex);
    
    int defaultPort(int protocolIndex);
    int getPortForInstall(int protocolIndex);
    int defaultTransportProto(int protocolIndex);
    bool defaultPortChangeable(int protocolIndex);
    bool defaultTransportProtoChangeable(int protocolIndex);

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
    void configValidated(bool isValid);

private:

    InstallController* m_installController;
    ServersController* m_serversController;
    SettingsController* m_settingsController;
    ProtocolsModel* m_protocolModel;
    UsersController* m_usersController;

    AwgConfigModel* m_awgConfigModel;
    WireGuardConfigModel* m_wireGuardConfigModel;
    OpenVpnConfigModel* m_openVpnConfigModel;
    XrayConfigModel* m_xrayConfigModel;
    TorConfigModel* m_torConfigModel;
#ifdef Q_OS_WINDOWS
    Ikev2ConfigModel* m_ikev2ConfigModel;
#endif
    SftpConfigModel* m_sftpConfigModel;
    Socks5ProxyConfigModel* m_socks5ConfigModel;

    ServerCredentials m_processedServerCredentials;

    QString m_privateKeyPassphrase;
    
    void updateProtocolConfigModel(int serverIndex, int containerIndex, int protocolIndex);
};

#endif // INSTALLUICONTROLLER_H
