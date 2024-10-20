#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QJsonObject>
#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/sshclient.h"

class Settings;
class VpnConfigurator;

using namespace amnezia;

class ServerController : public QObject
{
    Q_OBJECT
public:
    ServerController(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
    ~ServerController();

    typedef QList<QPair<QString, QString>> Vars;

    ErrorCode rebootServer(const ServerCredentials &credentials);
    ErrorCode removeAllContainers(const ServerCredentials &credentials);
    ErrorCode removeContainer(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode setupContainer(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config, bool isUpdate = false);
    ErrorCode updateContainer(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &oldConfig,
                              QJsonObject &newConfig);

    ErrorCode startupContainerWorker(const ServerCredentials &credentials, DockerContainer container,
                                     const QJsonObject &config = QJsonObject());

    ErrorCode uploadTextFileToContainer(DockerContainer container, const ServerCredentials &credentials, const QString &file,
                                        const QString &path,
                                        libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);
    QByteArray getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                        ErrorCode &errorCode);

    QString replaceVars(const QString &script, const Vars &vars);
    Vars genVarsForScript(const ServerCredentials &credentials, DockerContainer container = DockerContainer::None,
                          const QJsonObject &config = QJsonObject());

    ErrorCode runScript(const ServerCredentials &credentials, QString script,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    ErrorCode runContainerScript(const ServerCredentials &credentials, DockerContainer container, QString script,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    QString checkSshConnection(const ServerCredentials &credentials, ErrorCode &errorCode);

    void cancelInstallation();

    ErrorCode getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey,
                                     const std::function<QString()> &callback);

    bool isNewAwgContainer(const ServerCredentials &credentials);

private:
    ErrorCode installDockerWorker(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode prepareHostWorker(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config = QJsonObject());
    ErrorCode buildContainerWorker(const ServerCredentials &credentials, DockerContainer container,
                                   const QJsonObject &config = QJsonObject());
    ErrorCode runContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);
    ErrorCode configureContainerWorker(const ServerCredentials &credentials, DockerContainer container, QJsonObject &config);

    ErrorCode isServerPortBusy(const ServerCredentials &credentials, DockerContainer container, const QJsonObject &config);
    bool isReinstallContainerRequired(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);
    ErrorCode isUserInSudo(const ServerCredentials &credentials, DockerContainer container);
    ErrorCode isServerDpkgBusy(const ServerCredentials &credentials, DockerContainer container);

    ErrorCode uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
                               libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);

    ErrorCode setupServerFirewall(const ServerCredentials &credentials);

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    bool m_cancelInstallation = false;
    libssh::Client m_sshClient;
signals:
    void serverIsBusy(const bool isBusy);
};

#endif // SERVERCONTROLLER_H
