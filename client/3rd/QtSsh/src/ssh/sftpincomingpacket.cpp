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

#include "sftpincomingpacket_p.h"

#include "sshexception_p.h"
#include "sshlogging_p.h"
#include "sshpacketparser_p.h"

namespace QSsh {
namespace Internal {

SftpIncomingPacket::SftpIncomingPacket() : m_length(0)
{
}

void SftpIncomingPacket::consumeData(QByteArray &newData)
{
    qCDebug(sshLog, "%s: current data size = %d, new data size = %d", Q_FUNC_INFO,
        m_data.size(), newData.size());

    if (isComplete() || dataSize() + newData.size() < sizeof m_length)
        return;

    if (dataSize() < sizeof m_length) {
        moveFirstBytes(m_data, newData, sizeof m_length - m_data.size());
        m_length = SshPacketParser::asUint32(m_data, static_cast<quint32>(0));
        if (m_length < static_cast<quint32>(TypeOffset + 1)
            || m_length > MaxPacketSize) {
            throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
                "Invalid length field in SFTP packet.");
        }
    }

    moveFirstBytes(m_data, newData,
        qMin<quint32>(m_length - dataSize() + 4, newData.size()));
}

void SftpIncomingPacket::moveFirstBytes(QByteArray &target, QByteArray &source,
    int n)
{
    target.append(source.left(n));
    source.remove(0, n);
}

bool SftpIncomingPacket::isComplete() const
{
    return m_length == dataSize() - 4;
}

void SftpIncomingPacket::clear()
{
    m_data.clear();
    m_length = 0;
}

quint32 SftpIncomingPacket::extractServerVersion() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_VERSION);
    try {
        return SshPacketParser::asUint32(m_data, TypeOffset + 1);
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_VERSION packet.");
    }
}

SftpHandleResponse SftpIncomingPacket::asHandleResponse() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_HANDLE);
    try {
        SftpHandleResponse response;
        quint32 offset = RequestIdOffset;
        response.requestId = SshPacketParser::asUint32(m_data, &offset);
        response.handle = SshPacketParser::asString(m_data, &offset);
        return response;
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_HANDLE packet");
    }
}

SftpStatusResponse SftpIncomingPacket::asStatusResponse() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_STATUS);
    try {
        SftpStatusResponse response;
        quint32 offset = RequestIdOffset;
        response.requestId = SshPacketParser::asUint32(m_data, &offset);
        response.status = static_cast<SftpStatusCode>(SshPacketParser::asUint32(m_data, &offset));
        response.errorString = SshPacketParser::asUserString(m_data, &offset);
        response.language = SshPacketParser::asString(m_data, &offset);
        return response;
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_STATUS packet.");
    }
}

SftpNameResponse SftpIncomingPacket::asNameResponse() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_NAME);
    try {
        SftpNameResponse response;
        quint32 offset = RequestIdOffset;
        response.requestId = SshPacketParser::asUint32(m_data, &offset);
        const quint32 count = SshPacketParser::asUint32(m_data, &offset);
        for (quint32 i = 0; i < count; ++i)
            response.files << asFile(offset);
        return response;
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_NAME packet.");
    }
}

SftpDataResponse SftpIncomingPacket::asDataResponse() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_DATA);
    try {
        SftpDataResponse response;
        quint32 offset = RequestIdOffset;
        response.requestId = SshPacketParser::asUint32(m_data, &offset);
        response.data = SshPacketParser::asString(m_data, &offset);
        return response;
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_DATA packet.");
    }
}

SftpAttrsResponse SftpIncomingPacket::asAttrsResponse() const
{
    Q_ASSERT(isComplete());
    Q_ASSERT(type() == SSH_FXP_ATTRS);
    try {
        SftpAttrsResponse response;
        quint32 offset = RequestIdOffset;
        response.requestId = SshPacketParser::asUint32(m_data, &offset);
        response.attrs = asFileAttributes(offset);
        return response;
    } catch (const SshPacketParseException &) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_PROTOCOL_ERROR,
            "Invalid SSH_FXP_ATTRS packet.");
    }
}

SftpFile SftpIncomingPacket::asFile(quint32 &offset) const
{
    SftpFile file;
    file.fileName
        = QString::fromUtf8(SshPacketParser::asString(m_data, &offset));
    file.longName
        = QString::fromUtf8(SshPacketParser::asString(m_data, &offset));
    file.attributes = asFileAttributes(offset);
    return file;
}

SftpFileAttributes SftpIncomingPacket::asFileAttributes(quint32 &offset) const
{
    SftpFileAttributes attributes;
    const quint32 flags = SshPacketParser::asUint32(m_data, &offset);
    attributes.sizePresent = flags & SSH_FILEXFER_ATTR_SIZE;
    attributes.timesPresent = flags & SSH_FILEXFER_ATTR_ACMODTIME;
    attributes.uidAndGidPresent = flags & SSH_FILEXFER_ATTR_UIDGID;
    attributes.permissionsPresent = flags & SSH_FILEXFER_ATTR_PERMISSIONS;
    if (attributes.sizePresent)
        attributes.size = SshPacketParser::asUint64(m_data, &offset);
    if (attributes.uidAndGidPresent) {
        attributes.uid = SshPacketParser::asUint32(m_data, &offset);
        attributes.gid = SshPacketParser::asUint32(m_data, &offset);
    }
    if (attributes.permissionsPresent)
        attributes.permissions = SshPacketParser::asUint32(m_data, &offset);
    if (attributes.timesPresent) {
        attributes.atime = SshPacketParser::asUint32(m_data, &offset);
        attributes.mtime = SshPacketParser::asUint32(m_data, &offset);
    }
    if (flags & SSH_FILEXFER_ATTR_EXTENDED) {
        const quint32 count = SshPacketParser::asUint32(m_data, &offset);
        for (quint32 i = 0; i < count; ++i) {
            SshPacketParser::asString(m_data, &offset);
            SshPacketParser::asString(m_data, &offset);
        }
    }
    return attributes;
}

} // namespace Internal
} // namespace QSsh
