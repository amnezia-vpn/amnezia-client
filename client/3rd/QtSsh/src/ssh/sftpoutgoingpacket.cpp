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

#include "sftpoutgoingpacket_p.h"

#include "sshlogging_p.h"
#include "sshpacket_p.h"

#include <QtEndian>

#include <limits>

namespace QSsh {
namespace Internal {

namespace {
    const quint32 DefaultAttributes = 0;
    const quint32 SSH_FXF_READ = 0x00000001;
    const quint32 SSH_FXF_WRITE = 0x00000002;
    const quint32 SSH_FXF_APPEND = 0x00000004;
    const quint32 SSH_FXF_CREAT = 0x00000008;
    const quint32 SSH_FXF_TRUNC = 0x00000010;
    const quint32 SSH_FXF_EXCL = 0x00000020;
}

SftpOutgoingPacket::SftpOutgoingPacket()
{
}

SftpOutgoingPacket &SftpOutgoingPacket::generateInit(quint32 version)
{
    return init(SSH_FXP_INIT, 0).appendInt(version).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateStat(const QString &path, quint32 requestId)
{
    return init(SSH_FXP_LSTAT, requestId).appendString(path).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateOpenDir(const QString &path,
    quint32 requestId)
{
    return init(SSH_FXP_OPENDIR, requestId).appendString(path).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateReadDir(const QByteArray &handle,
    quint32 requestId)
{
    return init(SSH_FXP_READDIR, requestId).appendString(handle).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateCloseHandle(const QByteArray &handle,
    quint32 requestId)
{
    return init(SSH_FXP_CLOSE, requestId).appendString(handle).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateMkDir(const QString &path,
    quint32 requestId)
{
    return init(SSH_FXP_MKDIR, requestId).appendString(path)
        .appendInt(DefaultAttributes).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateRmDir(const QString &path,
    quint32 requestId)
{
    return init(SSH_FXP_RMDIR, requestId).appendString(path).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateRm(const QString &path,
    quint32 requestId)
{
    return init(SSH_FXP_REMOVE, requestId).appendString(path).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateRename(const QString &oldPath,
    const QString &newPath, quint32 requestId)
{
    return init(SSH_FXP_RENAME, requestId).appendString(oldPath)
        .appendString(newPath).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateOpenFileForWriting(const QString &path,
    SftpOverwriteMode mode, quint32 permissions, quint32 requestId)
{
    QList<quint32> attributes;
    if (permissions != DefaultPermissions)
        attributes << SSH_FILEXFER_ATTR_PERMISSIONS << permissions;
    else
        attributes << DefaultAttributes;
    return generateOpenFile(path, Write, mode, attributes, requestId);
}

SftpOutgoingPacket &SftpOutgoingPacket::generateOpenFileForReading(const QString &path,
    quint32 requestId)
{
    // Note: Overwrite mode is irrelevant and will be ignored.
    return generateOpenFile(path, Read, SftpSkipExisting, QList<quint32>() << DefaultAttributes,
        requestId);
}

SftpOutgoingPacket &SftpOutgoingPacket::generateReadFile(const QByteArray &handle,
    quint64 offset, quint32 length, quint32 requestId)
{
    return init(SSH_FXP_READ, requestId).appendString(handle).appendInt64(offset)
        .appendInt(length).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateFstat(const QByteArray &handle,
    quint32 requestId)
{
    return init(SSH_FXP_FSTAT, requestId).appendString(handle).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateWriteFile(const QByteArray &handle,
    quint64 offset, const QByteArray &data, quint32 requestId)
{
    return init(SSH_FXP_WRITE, requestId).appendString(handle)
        .appendInt64(offset).appendString(data).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateCreateLink(const QString &filePath,
    const QString &target, quint32 requestId)
{
    return init(SSH_FXP_SYMLINK, requestId).appendString(filePath).appendString(target).finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::generateOpenFile(const QString &path,
    OpenType openType, SftpOverwriteMode mode, const QList<quint32> &attributes, quint32 requestId)
{
    quint32 pFlags = 0;
    switch (openType) {
    case Read:
        pFlags = SSH_FXF_READ;
        break;
    case Write:
        pFlags = SSH_FXF_WRITE | SSH_FXF_CREAT;
        switch (mode) {
        case SftpOverwriteExisting: pFlags |= SSH_FXF_TRUNC; break;
        case SftpAppendToExisting: pFlags |= SSH_FXF_APPEND; break;
        case SftpSkipExisting: pFlags |= SSH_FXF_EXCL; break;
        }
        break;
    }

    init(SSH_FXP_OPEN, requestId).appendString(path).appendInt(pFlags);
    foreach (const quint32 attribute, attributes)
        appendInt(attribute);
    return finalize();
}

SftpOutgoingPacket &SftpOutgoingPacket::init(SftpPacketType type,
    quint32 requestId)
{
    m_data.resize(TypeOffset + 1);
    m_data[TypeOffset] = type;
    if (type != SSH_FXP_INIT) {
        appendInt(requestId);
        qCDebug(sshLog, "Generating SFTP packet of type %d with request id %u", type, requestId);
    }
    return *this;
}

SftpOutgoingPacket &SftpOutgoingPacket::appendInt(quint32 val)
{
    m_data.append(AbstractSshPacket::encodeInt(val));
    return *this;
}

SftpOutgoingPacket &SftpOutgoingPacket::appendInt64(quint64 value)
{
    m_data.append(AbstractSshPacket::encodeInt(value));
    return *this;
}

SftpOutgoingPacket &SftpOutgoingPacket::appendString(const QString &string)
{
    m_data.append(AbstractSshPacket::encodeString(string.toUtf8()));
    return *this;
}

SftpOutgoingPacket &SftpOutgoingPacket::appendString(const QByteArray &string)
{
    m_data += AbstractSshPacket::encodeString(string);
    return *this;
}

SftpOutgoingPacket &SftpOutgoingPacket::finalize()
{
    AbstractSshPacket::setLengthField(m_data);
    return *this;
}

const quint32 SftpOutgoingPacket::DefaultPermissions = std::numeric_limits<quint32>::max();

} // namespace Internal
} // namespace QSsh
