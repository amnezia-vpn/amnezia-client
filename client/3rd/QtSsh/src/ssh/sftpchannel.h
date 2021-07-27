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

#ifndef SFTCHANNEL_H
#define SFTCHANNEL_H

#include "sftpdefs.h"

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


/*!
    \class QSsh::SftpChannel

    \brief This class provides SFTP operations.

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

class QSSH_EXPORT SftpChannel : public QObject
{
    Q_OBJECT

    friend class Internal::SftpChannelPrivate;
    friend class Internal::SshChannelManager;
public:
    /// Convenience typedef
    typedef QSharedPointer<SftpChannel> Ptr;

    /// \see state
    enum State { Uninitialized, Initializing, Initialized, Closing, Closed };

    /// Current state of this channel
    State state() const;

    /*!
     * @brief Makes this channel ready to use.
     */
    void initialize();

    /*!
     * @brief Call this when you are done with the channel.
     */
    void closeChannel();

    /*!
     * \brief Get information about a remote path, file or directory
     * \param path Remote path to state
     * \return A unique ID identifying this job
     */
    SftpJobId statFile(const QString &path);

    /*!
     * \brief Get list of contents of a directory
     * \param dirPath Remote path of directory
     * \return A unique ID identifying this job
     */
    SftpJobId listDirectory(const QString &dirPath);

    /*!
     * \brief Create remote directory
     * \param dirPath Remote path of directory
     * \return A unique ID identifying this job
     */
    SftpJobId createDirectory(const QString &dirPath);

    /*!
     * \brief Remove remote directory
     * \param dirPath Remote path of directory
     * \return A unique ID identifying this job
     */
    SftpJobId removeDirectory(const QString &dirPath);

    /*!
     * \brief Remove remote file
     * \param filePath Remote path of file
     * \return A unique ID identifying this job
     */
    SftpJobId removeFile(const QString &filePath);

    /*!
     * \brief Rename or move a remote file or directory
     * \param oldPath Path of existing file or directory
     * \param newPath New path the file or directory should be available as
     * \return A unique ID identifying this job
     */
    SftpJobId renameFileOrDirectory(const QString &oldPath,
        const QString &newPath);

    /*!
     * \brief Create a new empty file.
     * \param filePath Remote path of the file.
     * \param mode The behavior if the file already exists.
     * \return A unique ID identifying this job
     */
    SftpJobId createFile(const QString &filePath, SftpOverwriteMode mode);

    /*!
     * \brief Creates a symbolic link pointing to another file.
     * \param filePath The path of the symbolic
     * \param target The path the symbolic link should point to
     * \return A unique ID identifying this job
     */
    SftpJobId createLink(const QString &filePath, const QString &target);

    /*!
     * \brief Creates a remote file and fills it with data from \a device
     * \param device If this is not open already it will be opened in \a QIODevice::ReadOnly mode
     * \param remoteFilePath The path on the server to upload the file to
     * \param mode #QSsh::SftpOverwriteMode defines the behavior if the file already exists
     * \return A unique ID identifying this job
     */
    SftpJobId uploadFile(QSharedPointer<QIODevice> device,
        const QString &remoteFilePath, SftpOverwriteMode mode);

    /*!
     * \brief Uploads a local file to the remote host.
     * \param localFilePath The local path to an existing file
     * \param remoteFilePath The remote path the file should be uploaded to
     * \param mode What it will do if the file already exists
     * \return A unique ID identifying this job
     */
    SftpJobId uploadFile(const QString &localFilePath,
        const QString &remoteFilePath, SftpOverwriteMode mode);

    /*!
     * \brief Downloads a remote file to a local path
     * \param remoteFilePath The remote path to the file to be downloaded
     * \param localFilePath The local path for where to download the file
     * \param mode Controls what happens if the local file already exists
     * \return A unique ID identifying this job
     */
    SftpJobId downloadFile(const QString &remoteFilePath,
        const QString &localFilePath, SftpOverwriteMode mode);

    /*!
     * \brief Retrieves the contents of a remote file and writes it to \a device
     * \param remoteFilePath The remote path of the file to retrieve the contents of
     * \param device The QIODevice to write the data to, this needs to be open in a writable mode
     * \return A unique ID identifying this job
     */
    SftpJobId downloadFile(const QString &remoteFilePath,
        QSharedPointer<QIODevice> device);

    /*!
     * \brief Uploads a local directory (recursively) with files to the remote host
     * \param localDirPath The path to an existing local directory
     * \param remoteParentDirPath The remote path to upload it to, the name of the local directory will be appended to this
     * \return A unique ID identifying this job
     */
    SftpJobId uploadDir(const QString &localDirPath,
        const QString &remoteParentDirPath);

    /*!
     * \brief Downloads a remote directory (recursively) to a local path
     * \param remoteDirPath The remote path of an existing directory to download
     * \param localDirPath The local path to download the directory to
     * \param mode
     * \return
     */
    SftpJobId downloadDir(const QString &remoteDirPath,
        const QString &localDirPath, SftpOverwriteMode mode);

    ~SftpChannel();

signals:
    /// Emitted when you can start using the channel
    void initialized();

    /// Emitted when an error happened
    void channelError(const QString &reason);

    /// Emitted when the channel has closed for some reason, either an error occured or it was asked for.
    void closed();

    /// error.isEmpty means it finished successfully
    void finished(QSsh::SftpJobId job, const SftpError errorType = SftpError::NoError, const QString &error = QString());

    /*!
     * Continously emitted during data transfer.
     * Does not emit for each file copied by uploadDir().
     */
    void dataAvailable(QSsh::SftpJobId job, const QString &data);

    /*!
     * This signal is emitted as a result of:
     *     - statFile() (with the list having exactly one element)
     *     - listDirectory() (potentially more than once)
     * It will continously be emitted as data is discovered, not only when the job is done.
     */
    void fileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo> &fileInfoList);

    /*!
     * Emitted during upload or download
     */
    void transferProgress(QSsh::SftpJobId job, quint64 progress, quint64 total);

private:
    SftpChannel(quint32 channelId, Internal::SshSendFacility &sendFacility);

    Internal::SftpChannelPrivate *d;
};

} // namespace QSsh

#endif // SFTPCHANNEL_H
