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

#pragma once

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
    void finished(QSsh::SftpJobId job, const QString &error = QString());
    void dataAvailable(QSsh::SftpJobId job, const QString &data);
    void fileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);

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

    void handleStatusGeneric(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleMkdirStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleLsStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handleGetStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);
    void handlePutStatus(const JobMap::Iterator &it,
        const SftpStatusResponse &response);

    void handleLsHandle(const JobMap::Iterator &it);
    void handleCreateFileHandle(const JobMap::Iterator &it);
    void handleGetHandle(const JobMap::Iterator &it);
    void handlePutHandle(const JobMap::Iterator &it);

    void spawnReadRequests(const SftpDownload::Ptr &job);
    void spawnWriteRequests(const JobMap::Iterator &it);
    void sendReadRequest(const SftpDownload::Ptr &job, quint32 requestId);
    void sendWriteRequest(const JobMap::Iterator &it);
    void finishTransferRequest(const JobMap::Iterator &it);
    void removeTransferRequest(const JobMap::Iterator &it);
    void reportRequestError(const AbstractSftpOperationWithHandle::Ptr &job,
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
