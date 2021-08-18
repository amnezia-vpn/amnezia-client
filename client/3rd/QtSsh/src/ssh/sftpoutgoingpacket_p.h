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

#ifndef SFTPOUTGOINGPACKET_P_H
#define SFTPOUTGOINGPACKET_P_H

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

#endif // SFTPOUTGOINGPACKET_P_H
