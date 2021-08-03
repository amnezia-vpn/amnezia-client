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

#ifndef SSHERRORS_P_H
#define SSHERRORS_P_H

#include <QMetaType>

namespace QSsh {

/*!
 * \brief SSH specific errors
 */
enum SshError {
    /// No error has occured
    SshNoError,

    /// There was a network socket error
    SshSocketError,

    /// The connection timed out
    SshTimeoutError,

    /// There was an error communicating with the server
    SshProtocolError,

    /// There was a problem with the remote host key
    SshHostKeyError,

    /// We failed to read or parse the key file used for authentication
    SshKeyFileError,

    /// We failed to authenticate
    SshAuthenticationError,

    /// The server closed our connection
    SshClosedByServerError,

    /// The ssh-agent used for authenticating failed somehow
    SshAgentError,

    /// Something bad happened on the server
    SshInternalError
};

} // namespace QSsh

Q_DECLARE_METATYPE(QSsh::SshError)

#endif // SSHERRORS_P_H
