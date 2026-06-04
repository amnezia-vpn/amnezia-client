#include "sshSession.h"

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPointer>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include <chrono>
#include <thread>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/networkUtilities.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "logger.h"
#include "core/utils/utilities.h"

namespace
{
    Logger logger("SshSession");
}

SshSession::SshSession(QObject *parent) : QObject(parent)
{
}

SshSession::~SshSession()
{
    m_sshClient.disconnectFromHost();
}

ErrorCode SshSession::runScript(const ServerCredentials &credentials, QString script,
                                const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut,
                                const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr)
{

    auto error = m_sshClient.connectToHost(credentials);
    if (error != ErrorCode::NoError) {
        return error;
    }

    script.replace("\r", "");

    qDebug() << "SshSession::Run script";

    QString totalLine;
    QStringList statements;
    const QStringList &lines = script.split("\n", Qt::SkipEmptyParts);
    for (int i = 0; i < lines.count(); i++) {
        QString currentLine = lines.at(i);

        if (totalLine.isEmpty()) {
            totalLine = currentLine;
        } else {
            totalLine = totalLine + "\n" + currentLine;
        }

        if (currentLine.endsWith("\\")) {
            continue;
        }

        QString lineToExec = totalLine;
        totalLine.clear();

        if (lineToExec.startsWith("#")) {
            continue;
        }

        statements << lineToExec;
    }

    if (statements.isEmpty()) {
        qDebug().noquote() << "SshSession::runScript finished (no statements)\n";
        return ErrorCode::NoError;
    }

    const QString batchedScript = statements.join("\n");
    qDebug().noquote() << batchedScript;

    error = m_sshClient.executeCommand(batchedScript, cbReadStdOut, cbReadStdErr);
    if (error != ErrorCode::NoError) {
        return error;
    }

    qDebug().noquote() << "SshSession::runScript finished\n";
    return ErrorCode::NoError;
}

ErrorCode SshSession::runContainerScript(const ServerCredentials &credentials, DockerContainer container, QString script,
                                         const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdOut,
                                         const std::function<ErrorCode(const QString &, libssh::Client &)> &cbReadStdErr)
{
    const bool useSh = container == DockerContainer::Socks5Proxy || container == DockerContainer::MtProxy || container == DockerContainer::Telemt;
    const QString shell = useSh ? QStringLiteral("sh") : QStringLiteral("bash");
    const QString b64 = QString::fromLatin1(script.toUtf8().toBase64());

    const QString command = QStringLiteral("printf '%s' '%1' | base64 -d | sudo docker exec -i $CONTAINER_NAME %2")
                                .arg(b64, shell);

    return runScript(credentials,
                     replaceVars(command, amnezia::genBaseVars(credentials, container, QString(), QString())),
                     cbReadStdOut, cbReadStdErr);
}

ErrorCode SshSession::uploadTextFileToContainer(DockerContainer container, const ServerCredentials &credentials, const QString &file,
                                                const QString &path, libssh::ScpOverwriteMode overwriteMode)
{
    if (overwriteMode != libssh::ScpOverwriteMode::ScpOverwriteExisting
        && overwriteMode != libssh::ScpOverwriteMode::ScpAppendToExisting) {
        return ErrorCode::NotImplementedError;
    }

    QString stdOut;
    auto cbReadStd = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    auto baseVars = amnezia::genBaseVars(credentials, container, QString(), QString());

    const QString b64 = QString::fromLatin1(file.toUtf8().toBase64());
    const QString dir = QFileInfo(path).path();
    const QString redirect = (overwriteMode == libssh::ScpOverwriteMode::ScpAppendToExisting)
                                 ? QStringLiteral(">>")
                                 : QStringLiteral(">");

    const QString command = QStringLiteral("printf '%s' '%1' | base64 -d | "
                                           "sudo docker exec -i $CONTAINER_NAME sh -c 'mkdir -p \"%2\" && cat %3 \"%4\"'")
                                    .arg(b64, dir, redirect, path);

    ErrorCode e = runScript(credentials, replaceVars(command, baseVars), cbReadStd, cbReadStd);
    if (e)
        return e;

    if (stdOut.contains("Error") && stdOut.contains("No such container")) {
        return ErrorCode::ServerContainerMissingError;
    }

    return e;
}

QByteArray SshSession::getTextFileFromContainer(DockerContainer container, const ServerCredentials &credentials, const QString &path,
                                                ErrorCode &errorCode)
{

    errorCode = ErrorCode::NoError;

    QString script = QStringLiteral("sudo docker exec -i %1 sh -c \"xxd -p '%2'\"").arg(ContainerUtils::containerToString(container), path);

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };

    errorCode = runScript(credentials, script, cbReadStdOut);
    return QByteArray::fromHex(stdOut.toUtf8());
}

QList<QByteArray> SshSession::getTextFilesFromContainer(DockerContainer container, const ServerCredentials &credentials,
                                                        const QStringList &paths, ErrorCode &errorCode)
{
    errorCode = ErrorCode::NoError;
    QList<QByteArray> result;
    if (paths.isEmpty()) {
        return result;
    }

    const QString sep = QStringLiteral("ZZAMNSEPZZ");
    QString inner;
    for (const QString &path : paths) {
        inner += QStringLiteral("xxd -p '%1'; echo '%2'; ").arg(path, sep);
    }
    QString script = QStringLiteral("sudo docker exec -i %1 sh -c \"%2\"")
                         .arg(ContainerUtils::containerToString(container), inner);

    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data;
        return ErrorCode::NoError;
    };
    errorCode = runScript(credentials, script, cbReadStdOut);

    const QStringList parts = stdOut.split(sep);
    for (int i = 0; i < paths.size(); ++i) {
        const QString hex = (i < parts.size()) ? parts.at(i) : QString();
        result.append(QByteArray::fromHex(hex.toUtf8()));
    }
    return result;
}

ErrorCode SshSession::uploadFileToHost(const ServerCredentials &credentials, const QByteArray &data, const QString &remotePath,
                                       libssh::ScpOverwriteMode overwriteMode)
{
    auto error = m_sshClient.connectToHost(credentials);
    if (error != ErrorCode::NoError) {
        return error;
    }

    QTemporaryFile localFile;
    localFile.open();
    localFile.write(data);
    localFile.close();

    error = m_sshClient.scpFileCopy(overwriteMode, localFile.fileName(), remotePath, "non_desc");

    if (error != ErrorCode::NoError) {
        return error;
    }
    return ErrorCode::NoError;
}

QString SshSession::checkSshConnection(const ServerCredentials &credentials, ErrorCode &errorCode)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };
    auto cbReadStdErr = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    errorCode = runScript(credentials, amnezia::scriptData(SharedScriptType::check_connection), cbReadStdOut, cbReadStdErr);

    return stdOut;
}

QString SshSession::replaceVars(const QString &script, const Vars &vars)
{
    QString s = script;
    for (const QPair<QString, QString> &var : vars) {
        s.replace(var.first, var.second);
    }
    return s;
}

ErrorCode SshSession::getDecryptedPrivateKey(const ServerCredentials &credentials, QString &decryptedPrivateKey,
                                             const std::function<QString()> &callback)
{
    auto error = m_sshClient.getDecryptedPrivateKey(credentials, decryptedPrivateKey, callback);
    return error;
}
