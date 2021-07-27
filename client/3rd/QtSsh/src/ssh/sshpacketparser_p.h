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

#ifndef SSHPACKETPARSER_P_H
#define SSHPACKETPARSER_P_H

#include <botan/bigint.h>

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
    static QByteArray asString(const QByteArray &data, quint32 offset);
    static QByteArray asString(const QByteArray &data, quint32 *offset);
    static QString asUserString(const QByteArray &data, quint32 *offset);
    static SshNameList asNameList(const QByteArray &data, quint32 *offset);
    static Botan::BigInt asBigInt(const QByteArray &data, quint32 *offset);

    static QString asUserString(const QByteArray &rawString);
};

} // namespace Internal
} // namespace QSsh

#endif // SSHPACKETPARSER_P_H
