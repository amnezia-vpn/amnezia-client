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

#include "ssh_global.h"

#include <QByteArray>
#include <QObject>
#include <QSharedPointer>
#include <QString>

namespace QSsh {

namespace Internal {
class SftpChannelPrivate;
class SshChannelManager;
class SshSendFacility;
} // namespace Internal

class QSSH_EXPORT SftpChannel : public QObject
{
    Q_OBJECT

    friend class Internal::SftpChannelPrivate;
    friend class Internal::SshChannelManager;
public:
    typedef QSharedPointer<SftpChannel> Ptr;

    enum State { Uninitialized, Initializing, Initialized, Closing, Closed };
    State state() const;

    void initialize();
    void closeChannel();

    SftpJobId statFile(const QString &path);
    SftpJobId listDirectory(const QString &dirPath);
    SftpJobId createDirectory(const QString &dirPath);
    SftpJobId removeDirectory(const QString &dirPath);
    SftpJobId removeFile(const QString &filePath);
    SftpJobId renameFileOrDirectory(const QString &oldPath,
        const QString &newPath);
    SftpJobId createFile(const QString &filePath, SftpOverwriteMode mode);
    SftpJobId createLink(const QString &filePath, const QString &target);
    SftpJobId uploadFile(const QString &localFilePath,
        const QString &remoteFilePath, SftpOverwriteMode mode);
    SftpJobId downloadFile(const QString &remoteFilePath,
        const QString &localFilePath, SftpOverwriteMode mode);
    SftpJobId uploadDir(const QString &localDirPath,
        const QString &remoteParentDirPath);

    ~SftpChannel();

signals:
    void initialized();
    void channelError(const QString &reason);
    void closed();

    // error.isEmpty <=> finished successfully
    void finished(QSsh::SftpJobId job, const QString &error = QString());

     // TODO: Also emit for each file copied by uploadDir().
    void dataAvailable(QSsh::SftpJobId job, const QString &data);

    /*
     * This signal is emitted as a result of:
     *     - statFile() (with the list having exactly one element)
     *     - listDirectory() (potentially more than once)
     */
    void fileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);

private:
    SftpChannel(quint32 channelId, Internal::SshSendFacility &sendFacility);

    Internal::SftpChannelPrivate *d;
};

} // namespace QSsh
