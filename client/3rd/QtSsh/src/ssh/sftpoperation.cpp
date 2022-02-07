/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "sftpoperation_p.h"

#include "sftpoutgoingpacket_p.h"

#include <QFile>

namespace QSsh {
namespace Internal {

AbstractSftpOperation::AbstractSftpOperation(SftpJobId jobId) : jobId(jobId)
{
}

AbstractSftpOperation::~AbstractSftpOperation() { }


SftpStatFile::SftpStatFile(SftpJobId jobId, const QString &path)
    : AbstractSftpOperation(jobId), path(path)
{
}

SftpOutgoingPacket &SftpStatFile::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateStat(path, jobId);
}

SftpMakeDir::SftpMakeDir(SftpJobId jobId, const QString &path,
    const SftpUploadDir::Ptr &parentJob)
    : AbstractSftpOperation(jobId), parentJob(parentJob), remoteDir(path)
{
}

SftpOutgoingPacket &SftpMakeDir::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateMkDir(remoteDir, jobId);
}


SftpRmDir::SftpRmDir(SftpJobId id, const QString &path)
    : AbstractSftpOperation(id), remoteDir(path)
{
}

SftpOutgoingPacket &SftpRmDir::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateRmDir(remoteDir, jobId);
}


SftpRm::SftpRm(SftpJobId jobId, const QString &path)
    : AbstractSftpOperation(jobId), remoteFile(path) {}

SftpOutgoingPacket &SftpRm::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateRm(remoteFile, jobId);
}


SftpRename::SftpRename(SftpJobId jobId, const QString &oldPath,
    const QString &newPath)
    : AbstractSftpOperation(jobId), oldPath(oldPath), newPath(newPath)
{
}

SftpOutgoingPacket &SftpRename::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateRename(oldPath, newPath, jobId);
}


SftpCreateLink::SftpCreateLink(SftpJobId jobId, const QString &filePath, const QString &target)
    : AbstractSftpOperation(jobId), filePath(filePath), target(target)
{
}

SftpOutgoingPacket &SftpCreateLink::initialPacket(SftpOutgoingPacket &packet)
{
    return packet.generateCreateLink(filePath, target, jobId);
}


AbstractSftpOperationWithHandle::AbstractSftpOperationWithHandle(SftpJobId jobId,
    const QString &remotePath)
    : AbstractSftpOperation(jobId),
      remotePath(remotePath), state(Inactive), hasError(false)
{
}

AbstractSftpOperationWithHandle::~AbstractSftpOperationWithHandle() { }


SftpListDir::SftpListDir(SftpJobId jobId, const QString &path,
    const QSharedPointer<SftpDownloadDir> &parentJob)
    : AbstractSftpOperationWithHandle(jobId, path), parentJob(parentJob)
{
}

SftpOutgoingPacket &SftpListDir::initialPacket(SftpOutgoingPacket &packet)
{
    state = OpenRequested;
    return packet.generateOpenDir(remotePath, jobId);
}


SftpCreateFile::SftpCreateFile(SftpJobId jobId, const QString &path,
    SftpOverwriteMode mode)
    : AbstractSftpOperationWithHandle(jobId, path), mode(mode)
{
}

SftpOutgoingPacket & SftpCreateFile::initialPacket(SftpOutgoingPacket &packet)
{
    state = OpenRequested;
    return packet.generateOpenFileForWriting(remotePath, mode,
        SftpOutgoingPacket::DefaultPermissions, jobId);
}


const int AbstractSftpTransfer::MaxInFlightCount = 10; // Experimentally found to be enough.

AbstractSftpTransfer::AbstractSftpTransfer(SftpJobId jobId, const QString &remotePath,
    const QSharedPointer<QIODevice> &localFile)
    : AbstractSftpOperationWithHandle(jobId, remotePath),
      localFile(localFile), fileSize(0), offset(0), inFlightCount(0),
      statRequested(false)
{
}

AbstractSftpTransfer::~AbstractSftpTransfer() {}

void AbstractSftpTransfer::calculateInFlightCount(quint32 chunkSize)
{
    if (fileSize == 0) {
        inFlightCount = 1;
    } else {
        inFlightCount = fileSize / chunkSize;
        if (fileSize % chunkSize)
            ++inFlightCount;
        if (inFlightCount > MaxInFlightCount)
            inFlightCount = MaxInFlightCount;
    }
}


SftpDownload::SftpDownload(SftpJobId jobId, const QString &remotePath,
    const QSharedPointer<QIODevice> &localFile, SftpOverwriteMode mode,
    const QSharedPointer<QSsh::Internal::SftpDownloadDir> &parentJob)
    : AbstractSftpTransfer(jobId, remotePath, localFile), eofId(SftpInvalidJob), mode(mode),
      parentJob(parentJob)
{
}

SftpOutgoingPacket &SftpDownload::initialPacket(SftpOutgoingPacket &packet)
{
    state = OpenRequested;
    return packet.generateOpenFileForReading(remotePath, jobId);
}


SftpUploadFile::SftpUploadFile(SftpJobId jobId, const QString &remotePath,
    const QSharedPointer<QIODevice> &localFile, SftpOverwriteMode mode,
    const SftpUploadDir::Ptr &parentJob)
    : AbstractSftpTransfer(jobId, remotePath, localFile),
      parentJob(parentJob), mode(mode)
{
    fileSize = localFile->size();
}

SftpOutgoingPacket &SftpUploadFile::initialPacket(SftpOutgoingPacket &packet)
{
    state = OpenRequested;
    quint32 permissions = 0;
    QFileDevice *fileDevice = qobject_cast<QFileDevice*>(localFile.data());
    if (fileDevice) {
        const QFile::Permissions &qtPermissions = fileDevice->permissions();
        if (qtPermissions & QFile::ExeOther)
            permissions |= 1 << 0;
        if (qtPermissions & QFile::WriteOther)
            permissions |= 1 << 1;
        if (qtPermissions & QFile::ReadOther)
            permissions |= 1 << 2;
        if (qtPermissions & QFile::ExeGroup)
            permissions |= 1<< 3;
        if (qtPermissions & QFile::WriteGroup)
            permissions |= 1<< 4;
        if (qtPermissions & QFile::ReadGroup)
            permissions |= 1<< 5;
        if (qtPermissions & QFile::ExeOwner)
            permissions |= 1<< 6;
        if (qtPermissions & QFile::WriteOwner)
            permissions |= 1<< 7;
        if (qtPermissions & QFile::ReadOwner)
            permissions |= 1<< 8;
    } else {
        // write owner
        permissions |= 1<< 7;
        // read owner
        permissions |= 1<< 8;
    }
    return packet.generateOpenFileForWriting(remotePath, mode, permissions, jobId);
}

SftpUploadDir::~SftpUploadDir() {}

} // namespace Internal
} // namespace QSsh
