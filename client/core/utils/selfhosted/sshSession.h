#ifndef SSHSESSION_H
#define SSHSESSION_H

#include <QJsonObject>
#include <QObject>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/selfhosted/sshClient.h"

using namespace amnezia;

class SshSession : public QObject
{
    Q_OBJECT
public:
    SshSession(QObject *parent = nullptr);
    ~SshSession();

    typedef QList<QPair<QString, QString>> Vars;

    ErrorCode uploadTextFileToContainer(DockerContainer container, const ServerCredentials &credentials, const QString &file,
                                        const QString &path,
                                        libssh::ScpOverwriteMode overwriteMode = libssh::ScpOverwriteMode::ScpOverwriteExisting);
    QByteArray getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                        ErrorCode &errorCode);

    static QString replaceVars(const QString &script, const Vars &vars);

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

private:
    libssh::Client m_sshClient;
};

#endif // SSHSESSION_H
