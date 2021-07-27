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

#ifndef SFTPPACKET_P_H
#define SFTPPACKET_P_H

#include <QByteArray>
#include <QList>
#include <QString>

namespace QSsh {
namespace Internal {

enum SftpPacketType {
    SSH_FXP_INIT = 1,
    SSH_FXP_VERSION = 2,
    SSH_FXP_OPEN = 3,
    SSH_FXP_CLOSE = 4,
    SSH_FXP_READ = 5,
    SSH_FXP_WRITE = 6,
    SSH_FXP_LSTAT = 7,
    SSH_FXP_FSTAT = 8,
    SSH_FXP_SETSTAT = 9,
    SSH_FXP_FSETSTAT = 10,
    SSH_FXP_OPENDIR = 11,
    SSH_FXP_READDIR = 12,
    SSH_FXP_REMOVE = 13,
    SSH_FXP_MKDIR = 14,
    SSH_FXP_RMDIR = 15,
    SSH_FXP_REALPATH = 16,
    SSH_FXP_STAT = 17,
    SSH_FXP_RENAME = 18,
    SSH_FXP_READLINK = 19,
    SSH_FXP_SYMLINK = 20, // Removed from later protocol versions. Try not to use.

    SSH_FXP_STATUS = 101,
    SSH_FXP_HANDLE = 102,
    SSH_FXP_DATA = 103,
    SSH_FXP_NAME = 104,
    SSH_FXP_ATTRS = 105,

    SSH_FXP_EXTENDED = 200,
    SSH_FXP_EXTENDED_REPLY = 201
};

enum SftpStatusCode {
    SSH_FX_OK = 0,
    SSH_FX_EOF = 1,
    SSH_FX_NO_SUCH_FILE = 2,
    SSH_FX_PERMISSION_DENIED = 3,
    SSH_FX_FAILURE = 4,
    SSH_FX_BAD_MESSAGE = 5,
    SSH_FX_NO_CONNECTION = 6,
    SSH_FX_CONNECTION_LOST = 7,
    SSH_FX_OP_UNSUPPORTED = 8
};

enum SftpAttributeType {
    SSH_FILEXFER_ATTR_SIZE = 0x00000001,
    SSH_FILEXFER_ATTR_UIDGID = 0x00000002,
    SSH_FILEXFER_ATTR_PERMISSIONS = 0x00000004,
    SSH_FILEXFER_ATTR_ACMODTIME = 0x00000008,
    SSH_FILEXFER_ATTR_EXTENDED = 0x80000000
};

class AbstractSftpPacket
{
public:
    AbstractSftpPacket();
    quint32 requestId() const;
    const QByteArray &rawData() const { return m_data; }
    SftpPacketType type() const { return static_cast<SftpPacketType>(m_data.at(TypeOffset)); }

    static const quint32 MaxDataSize; // "Pure" data size per read/writepacket.
    static const quint32 MaxPacketSize;

protected:
    quint32 dataSize() const { return static_cast<quint32>(m_data.size()); }

    static const int TypeOffset;
    static const int RequestIdOffset;
    static const int PayloadOffset;

    QByteArray m_data;
};

} // namespace Internal
} // namespace QSsh

#endif // SFTPPACKET_P_H
