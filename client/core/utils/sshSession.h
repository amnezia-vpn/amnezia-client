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

    ErrorCode uploadTextFileToContainer(DockerContainer container, const ServerCredentials &credentials, const QString &file,
                                        const QString &path,
                                        libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);
    QByteArray getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                        ErrorCode &errorCode);

    QString replaceVars(const QString &script, const Vars &vars);

    ErrorCode runScript(const ServerCredentials &credentials, QString script,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                        const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    ErrorCode runContainerScript(const ServerCredentials &credentials, DockerContainer container, QString script,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut = nullptr,
                                 const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr = nullptr);

    QString checkSshConnection(const ServerCredentials &credentials, ErrorCode &errorCode);

    ErrorCode getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey,
                                     const std::function<QString()> &callback);

    ErrorCode uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
                               libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);

    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    libssh::Client m_sshClient;
};

#endif // SERVERCONTROLLER_H
