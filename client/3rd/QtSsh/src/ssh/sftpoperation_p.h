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

#ifndef SFTPOPERATION_P_H
#define SFTPOPERATION_P_H

#include "sftpdefs.h"

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace QSsh {
namespace Internal {

class SftpOutgoingPacket;

struct AbstractSftpOperation
{
    typedef QSharedPointer<AbstractSftpOperation> Ptr;
    enum Type {
        StatFile, ListDir, MakeDir, RmDir, Rm, Rename, CreateLink, CreateFile, Download, UploadFile
    };

    AbstractSftpOperation(SftpJobId jobId);
    virtual ~AbstractSftpOperation();
    virtual Type type() const = 0;
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet) = 0;

    const SftpJobId jobId;

private:
    AbstractSftpOperation(const AbstractSftpOperation &);
    AbstractSftpOperation &operator=(const AbstractSftpOperation &);
};

struct SftpUploadDir;
struct SftpDownloadDir;

struct SftpStatFile : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpStatFile> Ptr;

    SftpStatFile(SftpJobId jobId, const QString &path);
    virtual Type type() const { return StatFile; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString path;
};

struct SftpMakeDir : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpMakeDir> Ptr;

    SftpMakeDir(SftpJobId jobId, const QString &path,
        const QSharedPointer<SftpUploadDir> &parentJob = QSharedPointer<SftpUploadDir>());
    virtual Type type() const { return MakeDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QSharedPointer<SftpUploadDir> parentJob;
    const QString remoteDir;
};

struct SftpRmDir : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRmDir> Ptr;

    SftpRmDir(SftpJobId id, const QString &path);
    virtual Type type() const { return RmDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString remoteDir;
};

struct SftpRm : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRm> Ptr;

    SftpRm(SftpJobId jobId, const QString &path);
    virtual Type type() const { return Rm; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString remoteFile;
};

struct SftpRename : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRename> Ptr;

    SftpRename(SftpJobId jobId, const QString &oldPath, const QString &newPath);
    virtual Type type() const { return Rename; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString oldPath;
    const QString newPath;
};

struct SftpCreateLink : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpCreateLink> Ptr;

    SftpCreateLink(SftpJobId jobId, const QString &filePath, const QString &target);
    virtual Type type() const { return CreateLink; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString filePath;
    const QString target;
};


struct AbstractSftpOperationWithHandle : public AbstractSftpOperation
{
    typedef QSharedPointer<AbstractSftpOperationWithHandle> Ptr;
    enum State { Inactive, OpenRequested, Open, CloseRequested };

    AbstractSftpOperationWithHandle(SftpJobId jobId, const QString &remotePath);
    ~AbstractSftpOperationWithHandle();

    const QString remotePath;
    QByteArray remoteHandle;
    State state;
    bool hasError;
};


struct SftpListDir : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<SftpListDir> Ptr;

    SftpListDir(SftpJobId jobId, const QString &path,
        const QSharedPointer<SftpDownloadDir> &parentJob = QSharedPointer<SftpDownloadDir>());
    virtual Type type() const { return ListDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QSharedPointer<SftpDownloadDir> parentJob;
};


struct SftpCreateFile : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<SftpCreateFile> Ptr;

    SftpCreateFile(SftpJobId jobId, const QString &path, SftpOverwriteMode mode);
    virtual Type type() const { return CreateFile; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const SftpOverwriteMode mode;
};

struct AbstractSftpTransfer : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<AbstractSftpTransfer> Ptr;

    AbstractSftpTransfer(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QIODevice> &localFile);
    ~AbstractSftpTransfer();
    void calculateInFlightCount(quint32 chunkSize);

    static const int MaxInFlightCount;

    const QSharedPointer<QIODevice> localFile;
    quint64 fileSize;
    quint64 offset;
    int inFlightCount;
    bool statRequested;
};

struct SftpDownload : public AbstractSftpTransfer
{
    typedef QSharedPointer<SftpDownload> Ptr;
    SftpDownload(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QIODevice> &localFile, SftpOverwriteMode mode,
        const QSharedPointer<SftpDownloadDir> &parentJob = QSharedPointer<SftpDownloadDir>());
    virtual Type type() const { return Download; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    QMap<quint32, quint64> offsets;
    SftpJobId eofId;
    SftpOverwriteMode mode;
    const QSharedPointer<QSsh::Internal::SftpDownloadDir> parentJob;
};

struct SftpUploadFile : public AbstractSftpTransfer
{
    typedef QSharedPointer<SftpUploadFile> Ptr;

    SftpUploadFile(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QIODevice> &localFile, SftpOverwriteMode mode,
        const QSharedPointer<SftpUploadDir> &parentJob = QSharedPointer<SftpUploadDir>());
    virtual Type type() const { return UploadFile; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QSharedPointer<SftpUploadDir> parentJob;
    SftpOverwriteMode mode;
};

// Composite operation.
struct SftpUploadDir
{
    typedef QSharedPointer<SftpUploadDir> Ptr;

    struct Dir {
        Dir(const QString &l, const QString &r) : localDir(l), remoteDir(r) {}
        QString localDir;
        QString remoteDir;
    };

    SftpUploadDir(SftpJobId jobId) : jobId(jobId), hasError(false) {}
    ~SftpUploadDir();

    void setError()
    {
        hasError = true;
        uploadsInProgress.clear();
        mkdirsInProgress.clear();
    }

    const SftpJobId jobId;
    bool hasError;
    QList<SftpUploadFile::Ptr> uploadsInProgress;
    QMap<SftpMakeDir::Ptr, Dir> mkdirsInProgress;
};

// Composite operation.
struct SftpDownloadDir
{
    typedef QSharedPointer<SftpDownloadDir> Ptr;

    struct Dir {
        Dir() {}
        Dir(const QString &l, const QString &r) : localDir(l), remoteDir(r) {}
        QString localDir;
        QString remoteDir;
    };

    SftpDownloadDir(SftpJobId jobId, SftpOverwriteMode mode)
        : jobId(jobId), hasError(false), mode(mode) {}

    ~SftpDownloadDir() {}

    void setError()
    {
        hasError = true;
        downloadsInProgress.clear();
        lsdirsInProgress.clear();
    }

    const SftpJobId jobId;
    bool hasError;
    SftpOverwriteMode mode;
    QList<SftpDownload::Ptr> downloadsInProgress;
    QMap<SftpListDir::Ptr, Dir> lsdirsInProgress;
};

} // namespace Internal
} // namespace QSsh

#endif // SFTPOPERATION_P_H
