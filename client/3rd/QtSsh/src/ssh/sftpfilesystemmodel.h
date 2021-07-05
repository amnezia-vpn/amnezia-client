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

#include "ssh_global.h"

#include <QAbstractItemModel>

namespace QSsh {
class SshConnectionParameters;

namespace Internal { class SftpFileSystemModelPrivate; }

// Very simple read-only model. Symbolic links are not followed.
class QSSH_EXPORT SftpFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SftpFileSystemModel(QObject *parent = 0);
    ~SftpFileSystemModel();

    /*
     * Once this is called, an SFTP connection is established and the model is populated.
     * The effect of additional calls is undefined.
     */
    void setSshConnection(const SshConnectionParameters &sshParams);

    void setRootDirectory(const QString &path); // Default is "/".
    QString rootDirectory() const;

    SftpJobId downloadFile(const QModelIndex &index, const QString &targetFilePath);

    // Use this to get the full path of a file or directory.
    static const int PathRole = Qt::UserRole;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

signals:
     /*
      * E.g. "Permission denied". Note that this can happen without direct user intervention,
      * due to e.g. the view calling rowCount() on a non-readable directory. This signal should
      * therefore not result in a message box or similar, since it might occur very often.
      */
    void sftpOperationFailed(const QString &errorMessage);

    /*
     * This error is not recoverable. The model will not have any content after
     * the signal has been emitted.
     */
    void connectionError(const QString &errorMessage);

    // Success <=> error.isEmpty().
    void sftpOperationFinished(QSsh::SftpJobId, const QString &error);

private:
    void handleSshConnectionEstablished();
    void handleSshConnectionFailure();
    void handleSftpChannelInitialized();
    void handleSftpChannelError(const QString &reason);
    void handleFileInfo(QSsh::SftpJobId jobId, const QList<QSsh::SftpFileInfo> &fileInfoList);
    void handleSftpJobFinished(QSsh::SftpJobId jobId, const QString &errorMessage);

    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void statRootDirectory();
    void shutDown();

    Internal::SftpFileSystemModelPrivate * const d;
};

} // namespace QSsh;
