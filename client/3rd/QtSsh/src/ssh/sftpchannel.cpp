/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "sftpchannel.h"
#include "sftpchannel_p.h"

#include "sshexception_p.h"
#include "sshincomingpacket_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"

#include <QDir>
#include <QFile>

/*!
    \class QSsh::SftpChannel

    \brief The SftpChannel class provides SFTP operations.

    Objects are created via SshConnection::createSftpChannel().
    The channel needs to be initialized with
    a call to initialize() and is closed via closeChannel(). After closing
    a channel, no more operations are possible. It cannot be re-opened
    using initialize(); use SshConnection::createSftpChannel() if you need
    a new one.

    After the initialized() signal has been emitted, operations can be started.
    All SFTP operations are asynchronous (non-blocking) and can be in-flight
    simultaneously (though callers must ensure that concurrently running jobs
    are independent of each other, e.g. they must not write to the same file).
    Operations are identified by their job id, which is returned by
    the respective member function. If the function can right away detect that
    the operation cannot succeed, it returns SftpInvalidJob. If an error occurs
    later, the finished() signal is emitted for the respective job with a
    non-empty error string.

    Note that directory names must not have a trailing slash.
*/

namespace QSsh {
namespace Internal {
namespace {
    const quint32 ProtocolVersion = 3;

    QString errorMessage(const QString &serverMessage,
        const QString &alternativeMessage)
    {
        return serverMessage.isEmpty() ? alternativeMessage : serverMessage;
    }

    QString errorMessage(const SftpStatusResponse &response,
        const QString &alternativeMessage)
    {
        return response.status == SSH_FX_OK ? QString()
            : errorMessage(response.errorString, alternativeMessage);
    }
} // anonymous namespace
} // namespace Internal

SftpChannel::SftpChannel(quint32 channelId,
    Internal::SshSendFacility &sendFacility)
    : d(new Internal::SftpChannelPrivate(channelId, sendFacility, this))
{
    connect(d, &Internal::SftpChannelPrivate::initialized,
            this, &SftpChannel::initialized, Qt::QueuedConnection);
    connect(d, &Internal::SftpChannelPrivate::channelError,
            this, &SftpChannel::channelError, Qt::QueuedConnection);
    connect(d, &Internal::SftpChannelPrivate::dataAvailable,
            this, &SftpChannel::dataAvailable, Qt::QueuedConnection);
    connect(d, &Internal::SftpChannelPrivate::fileInfoAvailable,
            this, &SftpChannel::fileInfoAvailable, Qt::QueuedConnection);
    connect(d, &Internal::SftpChannelPrivate::finished,
            this, &SftpChannel::finished, Qt::QueuedConnection);
    connect(d, &Internal::SftpChannelPrivate::closed,
            this, &SftpChannel::closed, Qt::QueuedConnection);
}

SftpChannel::State SftpChannel::state() const
{
    switch (d->channelState()) {
    case Internal::AbstractSshChannel::Inactive:
        return Uninitialized;
    case Internal::AbstractSshChannel::SessionRequested:
        return Initializing;
    case Internal::AbstractSshChannel::CloseRequested:
        return Closing;
    case Internal::AbstractSshChannel::Closed:
        return Closed;
    case Internal::AbstractSshChannel::SessionEstablished:
        return d->m_sftpState == Internal::SftpChannelPrivate::Initialized
            ? Initialized : Initializing;
    default:
        Q_ASSERT(!"Oh no, we forgot to handle a channel state!");
        return Closed; // For the compiler.
    }
}

void SftpChannel::initialize()
{
    d->requestSessionStart();
    d->m_sftpState = Internal::SftpChannelPrivate::SubsystemRequested;
}

void SftpChannel::closeChannel()
{
    d->closeChannel();
}

SftpJobId SftpChannel::statFile(const QString &path)
{
    return d->createJob(Internal::SftpStatFile::Ptr(
        new Internal::SftpStatFile(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::listDirectory(const QString &path)
{
    return d->createJob(Internal::SftpListDir::Ptr(
        new Internal::SftpListDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::createDirectory(const QString &path)
{
    return d->createJob(Internal::SftpMakeDir::Ptr(
        new Internal::SftpMakeDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::removeDirectory(const QString &path)
{
    return d->createJob(Internal::SftpRmDir::Ptr(
        new Internal::SftpRmDir(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::removeFile(const QString &path)
{
    return d->createJob(Internal::SftpRm::Ptr(
        new Internal::SftpRm(++d->m_nextJobId, path)));
}

SftpJobId SftpChannel::renameFileOrDirectory(const QString &oldPath,
    const QString &newPath)
{
    return d->createJob(Internal::SftpRename::Ptr(
        new Internal::SftpRename(++d->m_nextJobId, oldPath, newPath)));
}

SftpJobId SftpChannel::createLink(const QString &filePath, const QString &target)
{
    return d->createJob(Internal::SftpCreateLink::Ptr(
        new Internal::SftpCreateLink(++d->m_nextJobId, filePath, target)));
}

SftpJobId SftpChannel::createFile(const QString &path, SftpOverwriteMode mode)
{
    return d->createJob(Internal::SftpCreateFile::Ptr(
        new Internal::SftpCreateFile(++d->m_nextJobId, path, mode)));
}

SftpJobId SftpChannel::uploadFile(const QString &localFilePath,
    const QString &remoteFilePath, SftpOverwriteMode mode)
{
    QSharedPointer<QFile> localFile(new QFile(localFilePath));
    if (!localFile->open(QIODevice::ReadOnly))
        return SftpInvalidJob;
    return d->createJob(Internal::SftpUploadFile::Ptr(
        new Internal::SftpUploadFile(++d->m_nextJobId, remoteFilePath, localFile, mode)));
}

SftpJobId SftpChannel::downloadFile(const QString &remoteFilePath,
    const QString &localFilePath, SftpOverwriteMode mode)
{
    QSharedPointer<QFile> localFile(new QFile(localFilePath));
    if (mode == SftpSkipExisting && localFile->exists())
        return SftpInvalidJob;
    QIODevice::OpenMode openMode = QIODevice::WriteOnly;
    if (mode == SftpOverwriteExisting)
        openMode |= QIODevice::Truncate;
    else if (mode == SftpAppendToExisting)
        openMode |= QIODevice::Append;
    if (!localFile->open(openMode))
        return SftpInvalidJob;
    return d->createJob(Internal::SftpDownload::Ptr(
        new Internal::SftpDownload(++d->m_nextJobId, remoteFilePath, localFile)));
}

SftpJobId SftpChannel::uploadDir(const QString &localDirPath,
    const QString &remoteParentDirPath)
{
    if (state() != Initialized)
        return SftpInvalidJob;
    const QDir localDir(localDirPath);
    if (!localDir.exists() || !localDir.isReadable())
        return SftpInvalidJob;
    const Internal::SftpUploadDir::Ptr uploadDirOp(
        new Internal::SftpUploadDir(++d->m_nextJobId));
    const QString remoteDirPath
        = remoteParentDirPath + QLatin1Char('/') + localDir.dirName();
    const Internal::SftpMakeDir::Ptr mkdirOp(
        new Internal::SftpMakeDir(++d->m_nextJobId, remoteDirPath, uploadDirOp));
    uploadDirOp->mkdirsInProgress.insert(mkdirOp,
        Internal::SftpUploadDir::Dir(localDirPath, remoteDirPath));
    d->createJob(mkdirOp);
    return uploadDirOp->jobId;
}

SftpChannel::~SftpChannel()
{
    delete d;
}


namespace Internal {

SftpChannelPrivate::SftpChannelPrivate(quint32 channelId,
    SshSendFacility &sendFacility, SftpChannel *sftp)
    : AbstractSshChannel(channelId, sendFacility),
      m_nextJobId(0), m_sftpState(Inactive), m_sftp(sftp)
{
}

SftpJobId SftpChannelPrivate::createJob(const AbstractSftpOperation::Ptr &job)
{
   if (m_sftp->state() != SftpChannel::Initialized)
       return SftpInvalidJob;
   m_jobs.insert(job->jobId, job);
   sendData(job->initialPacket(m_outgoingPacket).rawData());
   return job->jobId;
}

void SftpChannelPrivate::handleChannelSuccess()
{
    if (channelState() == CloseRequested)
        return;
    qCDebug(sshLog, "sftp subsystem initialized");
    sendData(m_outgoingPacket.generateInit(ProtocolVersion).rawData());
    m_sftpState = InitSent;
}

void SftpChannelPrivate::handleChannelFailure()
{
    if (channelState() == CloseRequested)
        return;

    if (m_sftpState != SubsystemRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_FAILURE packet.");
    }
    emit channelError(tr("Server could not start SFTP subsystem."));
    closeChannel();
}

void SftpChannelPrivate::handleChannelDataInternal(const QByteArray &data)
{
    if (channelState() == CloseRequested)
        return;

    m_incomingData += data;
    m_incomingPacket.consumeData(m_incomingData);
    while (m_incomingPacket.isComplete()) {
        handleCurrentPacket();
        m_incomingPacket.clear();
        m_incomingPacket.consumeData(m_incomingData);
    }
}

void SftpChannelPrivate::handleChannelExtendedDataInternal(quint32 type,
    const QByteArray &data)
{
    qCWarning(sshLog, "Unexpected extended data '%s' of type %d on SFTP channel.",
              data.data(), type);
}

void SftpChannelPrivate::handleExitStatus(const SshChannelExitStatus &exitStatus)
{
    qCDebug(sshLog, "Remote SFTP service exited with exit code %d", exitStatus.exitStatus);

    if (channelState() == CloseRequested || channelState() == Closed)
        return;

    emit channelError(tr("The SFTP server finished unexpectedly with exit code %1.")
                      .arg(exitStatus.exitStatus));

    // Note: According to the specs, the server must close the channel after this happens,
    // but OpenSSH doesn't do that, so we need to initiate the closing procedure ourselves.
    closeChannel();
}

void SftpChannelPrivate::handleExitSignal(const SshChannelExitSignal &signal)
{
    emit channelError(tr("The SFTP server crashed: %1.").arg(signal.error));
    closeChannel(); // See above.
}

void SftpChannelPrivate::handleCurrentPacket()
{
    qCDebug(sshLog, "Handling SFTP packet of type %d", m_incomingPacket.type());
    switch (m_incomingPacket.type()) {
    case SSH_FXP_VERSION:
        handleServerVersion();
        break;
    case SSH_FXP_HANDLE:
        handleHandle();
        break;
    case SSH_FXP_NAME:
        handleName();
        break;
    case SSH_FXP_STATUS:
        handleStatus();
        break;
    case SSH_FXP_DATA:
        handleReadData();
        break;
    case SSH_FXP_ATTRS:
        handleAttrs();
        break;
    default:
        throw SshServerException(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected packet.",
            tr("Unexpected packet of type %1.").arg(m_incomingPacket.type()));
    }
}

void SftpChannelPrivate::handleServerVersion()
{
    checkChannelActive();
    if (m_sftpState != InitSent) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_VERSION packet.");
    }

    qCDebug(sshLog, "sftp init received");
    const quint32 serverVersion = m_incomingPacket.extractServerVersion();
    if (serverVersion != ProtocolVersion) {
        emit channelError(tr("Protocol version mismatch: Expected %1, got %2")
            .arg(serverVersion).arg(ProtocolVersion));
        closeChannel();
    } else {
        m_sftpState = Initialized;
        emit initialized();
    }
}

void SftpChannelPrivate::handleHandle()
{
    const SftpHandleResponse &response = m_incomingPacket.asHandleResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    const QSharedPointer<AbstractSftpOperationWithHandle> job
        = it.value().dynamicCast<AbstractSftpOperationWithHandle>();
    if (job.isNull()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_HANDLE packet.");
    }
    if (job->state != AbstractSftpOperationWithHandle::OpenRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_HANDLE packet.");
    }
    job->remoteHandle = response.handle;
    job->state = AbstractSftpOperationWithHandle::Open;

    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir:
        handleLsHandle(it);
        break;
    case AbstractSftpOperation::CreateFile:
        handleCreateFileHandle(it);
        break;
    case AbstractSftpOperation::Download:
        handleGetHandle(it);
        break;
    case AbstractSftpOperation::UploadFile:
        handlePutHandle(it);
        break;
    default:
        Q_ASSERT(!"Oh no, I forgot to handle an SFTP operation type!");
    }
}

void SftpChannelPrivate::handleLsHandle(const JobMap::Iterator &it)
{
    SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
    sendData(m_outgoingPacket.generateReadDir(op->remoteHandle,
        op->jobId).rawData());
}

void SftpChannelPrivate::handleCreateFileHandle(const JobMap::Iterator &it)
{
    SftpCreateFile::Ptr op = it.value().staticCast<SftpCreateFile>();
    sendData(m_outgoingPacket.generateCloseHandle(op->remoteHandle,
        op->jobId).rawData());
}

void SftpChannelPrivate::handleGetHandle(const JobMap::Iterator &it)
{
    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    sendData(m_outgoingPacket.generateFstat(op->remoteHandle,
        op->jobId).rawData());
    op->statRequested = true;
}

void SftpChannelPrivate::handlePutHandle(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr op = it.value().staticCast<SftpUploadFile>();
    if (op->parentJob && op->parentJob->hasError)
        sendTransferCloseHandle(op, it.key());

    // OpenSSH does not implement the RFC's append functionality, so we
    // have to emulate it.
    if (op->mode == SftpAppendToExisting) {
        sendData(m_outgoingPacket.generateFstat(op->remoteHandle,
            op->jobId).rawData());
        op->statRequested = true;
    } else {
        spawnWriteRequests(it);
    }
}

void SftpChannelPrivate::handleStatus()
{
    const SftpStatusResponse &response = m_incomingPacket.asStatusResponse();
    qCDebug(sshLog, "%s: status = %d", Q_FUNC_INFO, response.status);
    JobMap::Iterator it = lookupJob(response.requestId);
    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir:
        handleLsStatus(it, response);
        break;
    case AbstractSftpOperation::Download:
        handleGetStatus(it, response);
        break;
    case AbstractSftpOperation::UploadFile:
        handlePutStatus(it, response);
        break;
    case AbstractSftpOperation::MakeDir:
        handleMkdirStatus(it, response);
        break;
    case AbstractSftpOperation::StatFile:
    case AbstractSftpOperation::RmDir:
    case AbstractSftpOperation::Rm:
    case AbstractSftpOperation::Rename:
    case AbstractSftpOperation::CreateFile:
    case AbstractSftpOperation::CreateLink:
        handleStatusGeneric(it, response);
        break;
    }
}

void SftpChannelPrivate::handleStatusGeneric(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    AbstractSftpOperation::Ptr op = it.value();
    const QString error = errorMessage(response, tr("Unknown error."));
    emit finished(op->jobId, error);
    m_jobs.erase(it);
}

void SftpChannelPrivate::handleMkdirStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpMakeDir::Ptr op = it.value().staticCast<SftpMakeDir>();
    QSharedPointer<SftpUploadDir> parentJob = op->parentJob;
    if (parentJob == SftpUploadDir::Ptr()) {
        handleStatusGeneric(it, response);
        return;
    }
    if (parentJob->hasError) {
        m_jobs.erase(it);
        return;
    }

    typedef QMap<SftpMakeDir::Ptr, SftpUploadDir::Dir>::Iterator DirIt;
    DirIt dirIt = parentJob->mkdirsInProgress.find(op);
    Q_ASSERT(dirIt != parentJob->mkdirsInProgress.end());
    const QString &remoteDir = dirIt.value().remoteDir;
    if (response.status == SSH_FX_OK) {
        emit dataAvailable(parentJob->jobId,
            tr("Created remote directory \"%1\".").arg(remoteDir));
    } else if (response.status == SSH_FX_FAILURE) {
        emit dataAvailable(parentJob->jobId,
            tr("Remote directory \"%1\" already exists.").arg(remoteDir));
    } else {
        parentJob->setError();
        emit finished(parentJob->jobId,
            tr("Error creating directory \"%1\": %2")
            .arg(remoteDir, response.errorString));
        m_jobs.erase(it);
        return;
    }

    QDir localDir(dirIt.value().localDir);
    const QFileInfoList &dirInfos
        = localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &dirInfo, dirInfos) {
        const QString remoteSubDir = remoteDir + QLatin1Char('/') + dirInfo.fileName();
        const SftpMakeDir::Ptr mkdirOp(
            new SftpMakeDir(++m_nextJobId, remoteSubDir, parentJob));
        parentJob->mkdirsInProgress.insert(mkdirOp,
            SftpUploadDir::Dir(dirInfo.absoluteFilePath(), remoteSubDir));
        createJob(mkdirOp);
    }

    const QFileInfoList &fileInfos = localDir.entryInfoList(QDir::Files);
    foreach (const QFileInfo &fileInfo, fileInfos) {
        QSharedPointer<QFile> localFile(new QFile(fileInfo.absoluteFilePath()));
        if (!localFile->open(QIODevice::ReadOnly)) {
            parentJob->setError();
            emit finished(parentJob->jobId,
                tr("Could not open local file \"%1\": %2")
                .arg(fileInfo.absoluteFilePath(), localFile->errorString()));
            m_jobs.erase(it);
            return;
        }

        const QString remoteFilePath = remoteDir + QLatin1Char('/') + fileInfo.fileName();
        SftpUploadFile::Ptr uploadFileOp(new SftpUploadFile(++m_nextJobId,
            remoteFilePath, localFile, SftpOverwriteExisting, parentJob));
        createJob(uploadFileOp);
        parentJob->uploadsInProgress.append(uploadFileOp);
    }

    parentJob->mkdirsInProgress.erase(dirIt);
    if (parentJob->mkdirsInProgress.isEmpty()
        && parentJob->uploadsInProgress.isEmpty())
        emit finished(parentJob->jobId);
    m_jobs.erase(it);
}

void SftpChannelPrivate::handleLsStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
    switch (op->state) {
    case SftpListDir::OpenRequested:
        emit finished(op->jobId, errorMessage(response.errorString,
            tr("Remote directory could not be opened for reading.")));
        m_jobs.erase(it);
        break;
    case SftpListDir::Open:
        if (response.status != SSH_FX_EOF)
            reportRequestError(op, errorMessage(response.errorString,
                tr("Failed to list remote directory contents.")));
        op->state = SftpListDir::CloseRequested;
        sendData(m_outgoingPacket.generateCloseHandle(op->remoteHandle,
            op->jobId).rawData());
        break;
    case SftpListDir::CloseRequested:
        if (!op->hasError) {
            const QString error = errorMessage(response,
                tr("Failed to close remote directory."));
            emit finished(op->jobId, error);
        }
        m_jobs.erase(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handleGetStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    switch (op->state) {
    case SftpDownload::OpenRequested:
        emit finished(op->jobId,
            errorMessage(response.errorString,
                tr("Failed to open remote file for reading.")));
        m_jobs.erase(it);
        break;
    case SftpDownload::Open:
        if (op->statRequested) {
            reportRequestError(op, errorMessage(response.errorString,
                tr("Failed to retrieve information on the remote file ('stat' failed).")));
            sendTransferCloseHandle(op, response.requestId);
        } else {
            if ((response.status != SSH_FX_EOF || response.requestId != op->eofId)
                && !op->hasError)
                reportRequestError(op, errorMessage(response.errorString,
                    tr("Failed to read remote file.")));
            finishTransferRequest(it);
        }
        break;
    case SftpDownload::CloseRequested:
        Q_ASSERT(op->inFlightCount == 1);
        if (!op->hasError) {
            if (response.status == SSH_FX_OK)
                emit finished(op->jobId);
            else
                reportRequestError(op, errorMessage(response.errorString,
                    tr("Failed to close remote file.")));
        }
        removeTransferRequest(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handlePutStatus(const JobMap::Iterator &it,
    const SftpStatusResponse &response)
{
    SftpUploadFile::Ptr job = it.value().staticCast<SftpUploadFile>();
    switch (job->state) {
    case SftpUploadFile::OpenRequested: {
        bool emitError = false;
        if (job->parentJob) {
            if (!job->parentJob->hasError) {
                job->parentJob->setError();
                emitError = true;
            }
        } else {
            emitError = true;
        }

        if (emitError) {
            emit finished(job->jobId,
                errorMessage(response.errorString,
                    tr("Failed to open remote file for writing.")));
        }
        m_jobs.erase(it);
        break;
    }
    case SftpUploadFile::Open:
        if (job->hasError || (job->parentJob && job->parentJob->hasError)) {
            job->hasError = true;
            finishTransferRequest(it);
            return;
        }

        if (response.status == SSH_FX_OK) {
            sendWriteRequest(it);
        } else {
            if (job->parentJob)
                job->parentJob->setError();
            reportRequestError(job, errorMessage(response.errorString,
                tr("Failed to write remote file.")));
            finishTransferRequest(it);
        }
        break;
    case SftpUploadFile::CloseRequested:
        Q_ASSERT(job->inFlightCount == 1);
        if (job->hasError || (job->parentJob && job->parentJob->hasError)) {
            m_jobs.erase(it);
            return;
        }

        if (response.status == SSH_FX_OK) {
            if (job->parentJob) {
                job->parentJob->uploadsInProgress.removeOne(job);
                if (job->parentJob->mkdirsInProgress.isEmpty()
                    && job->parentJob->uploadsInProgress.isEmpty())
                    emit finished(job->parentJob->jobId);
            } else {
                emit finished(job->jobId);
            }
        } else {
            const QString error = errorMessage(response.errorString,
                tr("Failed to close remote file."));
            if (job->parentJob) {
                job->parentJob->setError();
                emit finished(job->parentJob->jobId, error);
            } else {
                emit finished(job->jobId, error);
            }
        }
        m_jobs.erase(it);
        break;
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_STATUS packet.");
    }
}

void SftpChannelPrivate::handleName()
{
    const SftpNameResponse &response = m_incomingPacket.asNameResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    switch (it.value()->type()) {
    case AbstractSftpOperation::ListDir: {
        SftpListDir::Ptr op = it.value().staticCast<SftpListDir>();
        if (op->state != SftpListDir::Open) {
            throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
                "Unexpected SSH_FXP_NAME packet.");
        }

        QList<SftpFileInfo> fileInfoList;
        for (int i = 0; i < response.files.count(); ++i) {
            const SftpFile &file = response.files.at(i);

            SftpFileInfo fileInfo;
            fileInfo.name = file.fileName;
            attributesToFileInfo(file.attributes, fileInfo);
            fileInfoList << fileInfo;
        }
        emit fileInfoAvailable(op->jobId, fileInfoList);
        sendData(m_outgoingPacket.generateReadDir(op->remoteHandle,
            op->jobId).rawData());
        break;
    }
    default:
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_NAME packet.");
    }
}

void SftpChannelPrivate::handleReadData()
{
    const SftpDataResponse &response = m_incomingPacket.asDataResponse();
    JobMap::Iterator it = lookupJob(response.requestId);
    if (it.value()->type() != AbstractSftpOperation::Download) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_DATA packet.");
    }

    SftpDownload::Ptr op = it.value().staticCast<SftpDownload>();
    if (op->hasError) {
        finishTransferRequest(it);
        return;
    }

    if (!op->localFile->seek(op->offsets[response.requestId])) {
        reportRequestError(op, op->localFile->errorString());
        finishTransferRequest(it);
        return;
    }

    if (op->localFile->write(response.data) != response.data.size()) {
        reportRequestError(op, op->localFile->errorString());
        finishTransferRequest(it);
        return;
    }

    if (op->offset >= op->fileSize && op->fileSize != 0)
        finishTransferRequest(it);
    else
        sendReadRequest(op, response.requestId);
}

void SftpChannelPrivate::handleAttrs()
{
    const SftpAttrsResponse &response = m_incomingPacket.asAttrsResponse();
    JobMap::Iterator it = lookupJob(response.requestId);

    SftpStatFile::Ptr statOp = it.value().dynamicCast<SftpStatFile>();
    if (statOp) {
        SftpFileInfo fileInfo;
        fileInfo.name = QFileInfo(statOp->path).fileName();
        attributesToFileInfo(response.attrs, fileInfo);
        emit fileInfoAvailable(it.key(), QList<SftpFileInfo>() << fileInfo);
        emit finished(it.key());
        m_jobs.erase(it);
        return;
    }

    AbstractSftpTransfer::Ptr transfer
        = it.value().dynamicCast<AbstractSftpTransfer>();
    if (!transfer || transfer->state != AbstractSftpTransfer::Open
        || !transfer->statRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_FXP_ATTRS packet.");
    }
    Q_ASSERT(transfer->type() == AbstractSftpOperation::UploadFile
        || transfer->type() == AbstractSftpOperation::Download);

    if (transfer->type() == AbstractSftpOperation::Download) {
        SftpDownload::Ptr op = transfer.staticCast<SftpDownload>();
        if (response.attrs.sizePresent) {
            op->fileSize = response.attrs.size;
        } else {
            op->fileSize = 0;
            op->eofId = op->jobId;
        }
        op->statRequested = false;
        spawnReadRequests(op);
    } else {
        SftpUploadFile::Ptr op = transfer.staticCast<SftpUploadFile>();
        if (op->parentJob && op->parentJob->hasError) {
            op->hasError = true;
            sendTransferCloseHandle(op, op->jobId);
            return;
        }

        if (response.attrs.sizePresent) {
            op->offset = response.attrs.size;
            spawnWriteRequests(it);
        } else {
            if (op->parentJob)
                op->parentJob->setError();
            reportRequestError(op, tr("Cannot append to remote file: "
                "Server does not support the file size attribute."));
            sendTransferCloseHandle(op, op->jobId);
        }
    }
}

SftpChannelPrivate::JobMap::Iterator SftpChannelPrivate::lookupJob(SftpJobId id)
{
    JobMap::Iterator it = m_jobs.find(id);
    if (it == m_jobs.end()) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid request id in SFTP packet.");
    }
    return it;
}

void SftpChannelPrivate::closeHook()
{
    for (JobMap::ConstIterator it = m_jobs.constBegin(); it != m_jobs.constEnd(); ++it)
        emit finished(it.key(), tr("SFTP channel closed unexpectedly."));
    m_jobs.clear();
    m_incomingData.clear();
    m_incomingPacket.clear();
    emit closed();
}

void SftpChannelPrivate::handleOpenSuccessInternal()
{
    qCDebug(sshLog, "SFTP session started");
    m_sendFacility.sendSftpPacket(remoteChannel());
    m_sftpState = SubsystemRequested;
}

void SftpChannelPrivate::handleOpenFailureInternal(const QString &reason)
{
    if (channelState() != SessionRequested) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Unexpected SSH_MSG_CHANNEL_OPEN_FAILURE packet.");
    }
    emit channelError(tr("Server could not start session: %1").arg(reason));
}

void SftpChannelPrivate::sendReadRequest(const SftpDownload::Ptr &job,
    quint32 requestId)
{
    Q_ASSERT(job->eofId == SftpInvalidJob);
    sendData(m_outgoingPacket.generateReadFile(job->remoteHandle, job->offset,
        AbstractSftpPacket::MaxDataSize, requestId).rawData());
    job->offsets[requestId] = job->offset;
    job->offset += AbstractSftpPacket::MaxDataSize;
    if (job->offset >= job->fileSize)
        job->eofId = requestId;
}

void SftpChannelPrivate::reportRequestError(const AbstractSftpOperationWithHandle::Ptr &job,
    const QString &error)
{
    emit finished(job->jobId, error);
    job->hasError = true;
}

void SftpChannelPrivate::finishTransferRequest(const JobMap::Iterator &it)
{
    AbstractSftpTransfer::Ptr job = it.value().staticCast<AbstractSftpTransfer>();
    if (job->inFlightCount == 1)
        sendTransferCloseHandle(job, it.key());
    else
        removeTransferRequest(it);
}

void SftpChannelPrivate::sendTransferCloseHandle(const AbstractSftpTransfer::Ptr &job,
    quint32 requestId)
{
    sendData(m_outgoingPacket.generateCloseHandle(job->remoteHandle,
       requestId).rawData());
    job->state = SftpDownload::CloseRequested;
}

void SftpChannelPrivate::attributesToFileInfo(const SftpFileAttributes &attributes,
    SftpFileInfo &fileInfo) const
{
    if (attributes.sizePresent) {
        fileInfo.sizeValid = true;
        fileInfo.size = attributes.size;
    }
    if (attributes.permissionsPresent) {
        if (attributes.permissions & 0x8000) // S_IFREG
            fileInfo.type = FileTypeRegular;
        else if (attributes.permissions & 0x4000) // S_IFDIR
            fileInfo.type = FileTypeDirectory;
        else
            fileInfo.type = FileTypeOther;
        fileInfo.permissionsValid = true;
        fileInfo.permissions = 0;
        if (attributes.permissions & 00001) // S_IXOTH
            fileInfo.permissions |= QFile::ExeOther;
        if (attributes.permissions & 00002) // S_IWOTH
            fileInfo.permissions |= QFile::WriteOther;
        if (attributes.permissions & 00004) // S_IROTH
            fileInfo.permissions |= QFile::ReadOther;
        if (attributes.permissions & 00010) // S_IXGRP
            fileInfo.permissions |= QFile::ExeGroup;
        if (attributes.permissions & 00020) // S_IWGRP
            fileInfo.permissions |= QFile::WriteGroup;
        if (attributes.permissions & 00040) // S_IRGRP
            fileInfo.permissions |= QFile::ReadGroup;
        if (attributes.permissions & 00100) // S_IXUSR
            fileInfo.permissions |= QFile::ExeUser | QFile::ExeOwner;
        if (attributes.permissions & 00200) // S_IWUSR
            fileInfo.permissions |= QFile::WriteUser | QFile::WriteOwner;
        if (attributes.permissions & 00400) // S_IRUSR
            fileInfo.permissions |= QFile::ReadUser | QFile::ReadOwner;
    }
}

void SftpChannelPrivate::removeTransferRequest(const JobMap::Iterator &it)
{
    --it.value().staticCast<AbstractSftpTransfer>()->inFlightCount;
    m_jobs.erase(it);
}

void SftpChannelPrivate::sendWriteRequest(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr job = it.value().staticCast<SftpUploadFile>();
    QByteArray data = job->localFile->read(AbstractSftpPacket::MaxDataSize);
    if (job->localFile->error() != QFile::NoError) {
        if (job->parentJob)
            job->parentJob->setError();
        reportRequestError(job, tr("Error reading local file: %1")
            .arg(job->localFile->errorString()));
        finishTransferRequest(it);
    } else if (data.isEmpty()) {
        finishTransferRequest(it);
    } else {
        sendData(m_outgoingPacket.generateWriteFile(job->remoteHandle,
            job->offset, data, it.key()).rawData());
        job->offset += AbstractSftpPacket::MaxDataSize;
    }
}

void SftpChannelPrivate::spawnWriteRequests(const JobMap::Iterator &it)
{
    SftpUploadFile::Ptr op = it.value().staticCast<SftpUploadFile>();
    op->calculateInFlightCount(AbstractSftpPacket::MaxDataSize);
    sendWriteRequest(it);
    for (int i = 1; !op->hasError && i < op->inFlightCount; ++i)
        sendWriteRequest(m_jobs.insert(++m_nextJobId, op));
}

void SftpChannelPrivate::spawnReadRequests(const SftpDownload::Ptr &job)
{
    job->calculateInFlightCount(AbstractSftpPacket::MaxDataSize);
    sendReadRequest(job, job->jobId);
    for (int i = 1; i < job->inFlightCount; ++i) {
        const quint32 requestId = ++m_nextJobId;
        m_jobs.insert(requestId, job);
        sendReadRequest(job, requestId);
    }
}

} // namespace Internal
} // namespace QSsh
