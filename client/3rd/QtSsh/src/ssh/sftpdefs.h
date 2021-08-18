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

#ifndef SFTPDEFS_H
#define SFTPDEFS_H

#include "ssh_global.h"

#include <QFile>
#include <QString>

/*!
 * \namespace QSsh
 * \brief The namespace used for the entire library
 */
namespace QSsh {


/*!
 *\brief Unique ID used for tracking individual jobs.
 */
typedef quint32 SftpJobId;

/*!
    Special ID representing an invalid job, e. g. if a requested job could not be started.
*/
QSSH_EXPORT extern const SftpJobId SftpInvalidJob;


/*!
 * \brief The behavior when uploading a file and the remote path already exists
 */
enum SftpOverwriteMode {
    /*! Overwrite any existing files */
    SftpOverwriteExisting,

    /*! Append new content if the file already exists */
    SftpAppendToExisting,

    /*! If the file or directory already exists skip it */
    SftpSkipExisting
};

/*!
 * \brief The type of a remote file.
 */
enum SftpFileType { FileTypeRegular, FileTypeDirectory, FileTypeOther, FileTypeUnknown };

/*!
 * \brief Possible errors.
*/
enum SftpError { NoError, EndOfFile, FileNotFound, PermissionDenied, GenericFailure, BadMessage, NoConnection, ConnectionLost, UnsupportedOperation  };

/*!
    \brief Contains information about a remote file.
*/
class QSSH_EXPORT SftpFileInfo
{
public:
    SftpFileInfo() : type(FileTypeUnknown), sizeValid(false), permissionsValid(false) { }

    /// The remote file name, only file attribute required by the RFC to be present so this is always set
    QString name;

    /// The type of file
    SftpFileType type = FileTypeUnknown;

    /// The remote file size in bytes.
    quint64 size = 0;

    /// The permissions set on the file, might be empty as the RFC allows an SFTP server not to support any file attributes beyond the name.
    QFileDevice::Permissions permissions{};

    /// Last time file was accessed.
    quint32 atime = 0;

    /// Last time file was modified.
    quint32 mtime = 0;

    /// If the timestamps (\ref atime and \ref mtime) are valid, the RFC allows an SFTP server not to support any file attributes beyond the name.
    bool timestampsValid = false;

    /// The RFC allows an SFTP server not to support any file attributes beyond the name.
    bool sizeValid = false;

    /// The RFC allows an SFTP server not to support any file attributes beyond the name.
    bool permissionsValid = false;
};

} // namespace QSsh

#endif // SFTPDEFS_H
