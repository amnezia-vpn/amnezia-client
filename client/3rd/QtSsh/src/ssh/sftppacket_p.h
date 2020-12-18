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
