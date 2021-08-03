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

#ifndef SSHEXCEPTION_P_H
#define SSHEXCEPTION_P_H

#include "ssherrors.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QString>

#include <exception>

namespace QSsh {
namespace Internal {

enum SshErrorCode {
    SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT = 1,
    SSH_DISCONNECT_PROTOCOL_ERROR = 2,
    SSH_DISCONNECT_KEY_EXCHANGE_FAILED = 3,
    SSH_DISCONNECT_RESERVED = 4,
    SSH_DISCONNECT_MAC_ERROR = 5,
    SSH_DISCONNECT_COMPRESSION_ERROR = 6,
    SSH_DISCONNECT_SERVICE_NOT_AVAILABLE = 7,
    SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED = 8,
    SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE = 9,
    SSH_DISCONNECT_CONNECTION_LOST = 10,
    SSH_DISCONNECT_BY_APPLICATION = 11,
    SSH_DISCONNECT_TOO_MANY_CONNECTIONS = 12,
    SSH_DISCONNECT_AUTH_CANCELLED_BY_USER = 13,
    SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE = 14,
    SSH_DISCONNECT_ILLEGAL_USER_NAME = 15
};

#define SSH_TR(string) QCoreApplication::translate("SshConnection", string)

#define SSH_SERVER_EXCEPTION(error, errorString)                              \
    SshServerException((error), (errorString), SSH_TR(errorString))

struct SshServerException : public std::exception
{
    SshServerException(SshErrorCode error, const QByteArray &errorStringServer,
        const QString &errorStringUser)
        : error(error), errorStringServer(errorStringServer),
          errorStringUser(errorStringUser) {}
    const char *what() const noexcept override { return errorStringServer.constData(); }

    const SshErrorCode error;
    const QByteArray errorStringServer;
    const QString errorStringUser;
};

struct SshClientException : public std::exception
{
    SshClientException(SshError error, const QString &errorString)
        : error(error), errorString(errorString), errorStringPrintable(errorString.toLocal8Bit()) {}
    const char *what() const noexcept override { return errorStringPrintable.constData(); }

    const SshError error;
    const QString errorString;
    const QByteArray errorStringPrintable;
};

} // namespace Internal
} // namespace QSsh

#endif // SSHEXCEPTION_P_H
