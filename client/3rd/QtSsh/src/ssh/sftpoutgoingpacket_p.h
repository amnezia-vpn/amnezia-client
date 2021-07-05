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

#include "sftppacket_p.h"
#include "sftpdefs.h"

namespace QSsh {
namespace Internal {

class SftpOutgoingPacket : public AbstractSftpPacket
{
public:
    SftpOutgoingPacket();
    SftpOutgoingPacket &generateInit(quint32 version);
    SftpOutgoingPacket &generateStat(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateOpenDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateReadDir(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateCloseHandle(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateMkDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRmDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRm(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRename(const QString &oldPath,
        const QString &newPath, quint32 requestId);
    SftpOutgoingPacket &generateOpenFileForWriting(const QString &path,
         SftpOverwriteMode mode, quint32 permissions, quint32 requestId);
    SftpOutgoingPacket &generateOpenFileForReading(const QString &path,
        quint32 requestId);
    SftpOutgoingPacket &generateReadFile(const QByteArray &handle,
        quint64 offset, quint32 length, quint32 requestId);
    SftpOutgoingPacket &generateFstat(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateWriteFile(const QByteArray &handle,
        quint64 offset, const QByteArray &data, quint32 requestId);

    // Note: OpenSSH's SFTP server has a bug that reverses the filePath and target
    //       arguments, so this operation is not portable.
    SftpOutgoingPacket &generateCreateLink(const QString &filePath, const QString &target,
        quint32 requestId);

    static const quint32 DefaultPermissions;

private:
    static QByteArray encodeString(const QString &string);

    enum OpenType { Read, Write };
    SftpOutgoingPacket &generateOpenFile(const QString &path, OpenType openType,
        SftpOverwriteMode mode, const QList<quint32> &attributes, quint32 requestId);

    SftpOutgoingPacket &init(SftpPacketType type, quint32 requestId);
    SftpOutgoingPacket &appendInt(quint32 value);
    SftpOutgoingPacket &appendInt64(quint64 value);
    SftpOutgoingPacket &appendString(const QString &string);
    SftpOutgoingPacket &appendString(const QByteArray &string);
    SftpOutgoingPacket &finalize();
};

} // namespace Internal
} // namespace QSsh
