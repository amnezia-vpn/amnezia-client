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

#include <botan/botan.h>

#include <QByteArray>
#include <QList>
#include <QString>

namespace QSsh {
namespace Internal {

struct SshNameList
{
    SshNameList() : originalLength(0) {}
    SshNameList(quint32 originalLength) : originalLength(originalLength) {}
    quint32 originalLength;
    QList<QByteArray> names;
};

class SshPacketParseException { };

// This class's functions try to read a byte array at a certain offset
// as the respective chunk of data as specified in the SSH RFCs.
// If they succeed, they update the offset, so they can easily
// be called in succession by client code.
// For convenience, some have also versions that don't update the offset,
// so they can be called with rvalues if the new value is not needed.
// If they fail, they throw an SshPacketParseException.
class SshPacketParser
{
public:
    static bool asBool(const QByteArray &data, quint32 offset);
    static bool asBool(const QByteArray &data, quint32 *offset);
    static quint16 asUint16(const QByteArray &data, quint32 offset);
    static quint16 asUint16(const QByteArray &data, quint32 *offset);
    static quint64 asUint64(const QByteArray &data, quint32 offset);
    static quint64 asUint64(const QByteArray &data, quint32 *offset);
    static quint32 asUint32(const QByteArray &data, quint32 offset);
    static quint32 asUint32(const QByteArray &data, quint32 *offset);
    static QByteArray asString(const QByteArray &data, quint32 *offset);
    static QString asUserString(const QByteArray &data, quint32 *offset);
    static SshNameList asNameList(const QByteArray &data, quint32 *offset);
    static Botan::BigInt asBigInt(const QByteArray &data, quint32 *offset);

    static QString asUserString(const QByteArray &rawString);
};

} // namespace Internal
} // namespace QSsh
