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

#include "sshpacketparser_p.h"

#include <cctype>

namespace QSsh {
namespace Internal {

namespace { quint32 size(const QByteArray &data) { return data.size(); } }

QString SshPacketParser::asUserString(const QByteArray &rawString)
{
    QByteArray filteredString;
    filteredString.resize(rawString.size());
    for (int i = 0; i < rawString.size(); ++i) {
        const char c = rawString.at(i);
        filteredString[i]
            = std::isprint(c) || c == '\n' || c == '\r' || c == '\t' ? c : '?';
    }
    return QString::fromUtf8(filteredString);
}

bool SshPacketParser::asBool(const QByteArray &data, quint32 offset)
{
    if (size(data) <= offset)
        throw SshPacketParseException();
    return data.at(offset);
}

bool SshPacketParser::asBool(const QByteArray &data, quint32 *offset)
{
    bool b = asBool(data, *offset);
    ++(*offset);
    return b;
}


quint32 SshPacketParser::asUint32(const QByteArray &data, quint32 offset)
{
    if (size(data) < offset + 4)
        throw SshPacketParseException();
    const quint32 value = ((data.at(offset) & 0xff) << 24)
        + ((data.at(offset + 1) & 0xff) << 16)
        + ((data.at(offset + 2) & 0xff) << 8) + (data.at(offset + 3) & 0xff);
    return value;
}

quint32 SshPacketParser::asUint32(const QByteArray &data, quint32 *offset)
{
    const quint32 v = asUint32(data, *offset);
    *offset += 4;
    return v;
}

quint64 SshPacketParser::asUint64(const QByteArray &data, quint32 offset)
{
    if (size(data) < offset + 8)
        throw SshPacketParseException();
    const quint64 value = (static_cast<quint64>(data.at(offset) & 0xff) << 56)
        + (static_cast<quint64>(data.at(offset + 1) & 0xff) << 48)
        + (static_cast<quint64>(data.at(offset + 2) & 0xff) << 40)
        + (static_cast<quint64>(data.at(offset + 3) & 0xff) << 32)
        + ((data.at(offset + 4) & 0xff) << 24)
        + ((data.at(offset + 5) & 0xff) << 16)
        + ((data.at(offset + 6) & 0xff) << 8)
        + (data.at(offset + 7) & 0xff);
    return value;
}

quint64 SshPacketParser::asUint64(const QByteArray &data, quint32 *offset)
{
    const quint64 val = asUint64(data, *offset);
    *offset += 8;
    return val;
}

QByteArray SshPacketParser::asString(const QByteArray &data, quint32 *offset)
{
    const quint32 length = asUint32(data, offset);
    if (size(data) < *offset + length)
        throw SshPacketParseException();
    const QByteArray &string = data.mid(*offset, length);
    *offset += length;
    return string;
}

QString SshPacketParser::asUserString(const QByteArray &data, quint32 *offset)
{
    return asUserString(asString(data, offset));
}

SshNameList SshPacketParser::asNameList(const QByteArray &data, quint32 *offset)
{
    const quint32 length = asUint32(data, offset);
    const int listEndPos = *offset + length;
    if (data.size() < listEndPos)
        throw SshPacketParseException();
    SshNameList names(length + 4);
    int nextNameOffset = *offset;
    int nextCommaOffset = data.indexOf(',', nextNameOffset);
    while (nextNameOffset > 0 && nextNameOffset < listEndPos) {
        const int stringEndPos = nextCommaOffset == -1
            || nextCommaOffset > listEndPos ? listEndPos : nextCommaOffset;
        names.names << QByteArray(data.constData() + nextNameOffset,
            stringEndPos - nextNameOffset);
        nextNameOffset = nextCommaOffset + 1;
        nextCommaOffset = data.indexOf(',', nextNameOffset);
    }
    *offset += length;
    return names;
}

Botan::BigInt SshPacketParser::asBigInt(const QByteArray &data, quint32 *offset)
{
    const quint32 length = asUint32(data, offset);
    if (length == 0)
        return Botan::BigInt();
    const Botan::byte *numberStart
        = reinterpret_cast<const Botan::byte *>(data.constData() + *offset);
    *offset += length;
    return Botan::BigInt::decode(numberStart, length);
}

} // namespace Internal
} // namespace QSsh
