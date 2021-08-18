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

#ifndef SFTCHANNEL_P_H
#define SFTCHANNEL_P_H

#include "sftpdefs.h"
#include "sftpincomingpacket_p.h"
#include "sftpoperation_p.h"
#include "sftpoutgoingpacket_p.h"
#include "sshchannel_p.h"

#include <QByteArray>
#include <QMap>

namespace QSsh {
class SftpChannel;
namespace Internal {

class SftpChannelPrivate : public AbstractSshChannel
{
    Q_OBJECT
    friend class QSsh::SftpChannel;
public:
    enum SftpState { Inactive, SubsystemRequested, InitSent, Initialized };

signals:
    void initialized();
    void channelError(const QString &reason);
    void closed();
    void finished(QSsh::SftpJobId job, const SftpError errorType = SftpError::NoError, const QString &error = QString());
    void dataAvailable(QSsh::SftpJobId job, const QString &data);
    void fileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);
    void transferProgress(QSsh::SftpJobId job, quint64 progress, quint64 total);

private:
    typedef QMap<SftpJobId, AbstractSftpOperation::Ptr> JobMap;

    SftpChannelPrivate(quint32 channelId, SshSendFacility &sendFacility,
        SftpChannel *sftp);
    SftpJobId createJob(const AbstractSftpOperation::Ptr &job);

    virtual void handleChannelSuccess();
    virtual void handleChannelFailure();

    virtual void handleOpenSuccessInternal();
    virtual void handleOpenFailureInternal(const QString &reason);
    virtual void handleChannelDataInternal(const QByteArray &data);
    virtual void handleChannelExtendedDataInternal(quint32 type,
        const QByteArray &data);
    virtual void handleExitStatus(const SshChannelExitStatus &exitStatus);
    virtual void handleExitSignal(const SshChannelExitSignal &signal);

    virtual void closeHook();

    void handleCurrentPacket();
    void handleServerVersion();
    void handleHandle();
    void handleStatus();
    void handleName();
    void handleReadData();
    void handleAttrs();

    void handleDownloadDir(SftpListDir::Ptr op, const QList<SftpFileInfo> & fileInfoList);

    void handleStatusGeneric(JobMap::Iterator it,
        const SftpStatusResponse &response);
    void handleMkdirStatus(JobMap::Iterator it,
        const SftpStatusResponse &response);
    void handleLsStatus(JobMap::Iterator it,
        const SftpStatusResponse &response);
    void handleGetStatus(JobMap::Iterator it,
        const SftpStatusResponse &response);
    void handlePutStatus(JobMap::Iterator it,
        const SftpStatusResponse &response);

    void handleLsHandle(JobMap::Iterator it);
    void handleCreateFileHandle(JobMap::Iterator it);
    void handleGetHandle(JobMap::Iterator it);
    void handlePutHandle(JobMap::Iterator it);

    void spawnReadRequests(const SftpDownload::Ptr &job);
    void spawnWriteRequests(JobMap::Iterator it);
    void sendReadRequest(const SftpDownload::Ptr &job, quint32 requestId);
    void sendWriteRequest(JobMap::Iterator it);
    void finishTransferRequest(JobMap::Iterator it);
    void removeTransferRequest(JobMap::Iterator it);
    void reportRequestError(const AbstractSftpOperationWithHandle::Ptr &job, const SftpError errorType,
        const QString &error);
    void sendTransferCloseHandle(const AbstractSftpTransfer::Ptr &job,
        quint32 requestId);

    void attributesToFileInfo(const SftpFileAttributes &attributes, SftpFileInfo &fileInfo) const;

    JobMap::Iterator lookupJob(SftpJobId id);
    JobMap m_jobs;
    SftpOutgoingPacket m_outgoingPacket;
    SftpIncomingPacket m_incomingPacket;
    QByteArray m_incomingData;
    SftpJobId m_nextJobId;
    SftpState m_sftpState;
    SftpChannel *m_sftp;
};

} // namespace Internal
} // namespace QSsh

#endif // SFTPCHANNEL_P_H
