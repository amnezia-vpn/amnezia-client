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


SftpListDir::SftpListDir(SftpJobId jobId, const QString &path)
    : AbstractSftpOperationWithHandle(jobId, path)
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
    const QSharedPointer<QFile> &localFile)
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
    const QSharedPointer<QFile> &localFile)
    : AbstractSftpTransfer(jobId, remotePath, localFile), eofId(SftpInvalidJob)
{
}

SftpOutgoingPacket &SftpDownload::initialPacket(SftpOutgoingPacket &packet)
{
    state = OpenRequested;
    return packet.generateOpenFileForReading(remotePath, jobId);
}


SftpUploadFile::SftpUploadFile(SftpJobId jobId, const QString &remotePath,
    const QSharedPointer<QFile> &localFile, SftpOverwriteMode mode,
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
    const QFile::Permissions &qtPermissions = localFile->permissions();
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
    return packet.generateOpenFileForWriting(remotePath, mode, permissions, jobId);
}

SftpUploadDir::~SftpUploadDir() {}

} // namespace Internal
} // namespace QSsh
