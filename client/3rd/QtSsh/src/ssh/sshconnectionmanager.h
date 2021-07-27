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

#ifndef SSHCONNECTIONMANAGER_H
#define SSHCONNECTIONMANAGER_H

#include "ssh_global.h"

namespace QSsh {

class SshConnection;
class SshConnectionParameters;

/*!
 * \brief Creates a new connection or returns an existing one if there already is one with identical sshParams
 * \param sshParams Parameters used during connection
 * \return A connection
 */
QSSH_EXPORT SshConnection *acquireConnection(const SshConnectionParameters &sshParams);

/*!
 * \brief Call this when you are done with a connection, might be disconnected and destroyed if there are no others who have called acquireConnection()
 * \param connection The connection to be released
 */
QSSH_EXPORT void releaseConnection(SshConnection *connection);

/*!
 * \brief Creates a new connection, unlike acquireConnection() it will not reuse an existing one.
 * \param sshParams Parameters used during connection
 * Make sure the next acquireConnection with the given parameters will return a new connection.
 */
QSSH_EXPORT void forceNewConnection(const SshConnectionParameters &sshParams);

} // namespace QSsh

#endif // SSHCONNECTIONMANAGER_H
