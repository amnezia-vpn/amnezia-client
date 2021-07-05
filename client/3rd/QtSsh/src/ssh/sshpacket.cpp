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

#include "sshpacket_p.h"

#include "sshcapabilities_p.h"
#include "sshcryptofacility_p.h"
#include "sshexception_p.h"
#include "sshlogging_p.h"
#include "sshpacketparser_p.h"

#include <QDebug>

#include <cctype>

namespace QSsh {
namespace Internal {

const quint32 AbstractSshPacket::PaddingLengthOffset = 4;
const quint32 AbstractSshPacket::PayloadOffset = PaddingLengthOffset + 1;
const quint32 AbstractSshPacket::TypeOffset = PayloadOffset;
const quint32 AbstractSshPacket::MinPaddingLength = 4;

static void printByteArray(const QByteArray &data)
{
    qCDebug(sshLog, "%s", data.toHex().constData());
}


AbstractSshPacket::AbstractSshPacket() : m_length(0) { }
AbstractSshPacket::~AbstractSshPacket() {}

bool AbstractSshPacket::isComplete() const
{
    if (currentDataSize() < minPacketSize())
        return false;
    return 4 + length() + macLength() == currentDataSize();
}

void AbstractSshPacket::clear()
{
    m_data.clear();
    m_length = 0;
}

SshPacketType AbstractSshPacket::type() const
{
    Q_ASSERT(isComplete());
    return static_cast<SshPacketType>(m_data.at(TypeOffset));
}

QByteArray AbstractSshPacket::payLoad() const
{
    return QByteArray(m_data.constData() + PayloadOffset,
        length() - paddingLength() - 1);
}

void AbstractSshPacket::printRawBytes() const
{
    printByteArray(m_data);
}

QByteArray AbstractSshPacket::encodeString(const QByteArray &string)
{
    QByteArray data;
    data.resize(4);
    data += string;
    setLengthField(data);
    return data;
}

QByteArray AbstractSshPacket::encodeMpInt(const Botan::BigInt &number)
{
    if (number.is_zero())
        return QByteArray(4, 0);

    int stringLength = static_cast<int>(number.bytes());
    const bool positiveAndMsbSet = number.sign() == Botan::BigInt::Positive
                                   && (number.byte_at(stringLength - 1) & 0x80);
    if (positiveAndMsbSet)
        ++stringLength;
    QByteArray data;
    data.resize(4 + stringLength);
    int pos = 4;
    if (positiveAndMsbSet)
        data[pos++] = '\0';
    number.binary_encode(reinterpret_cast<Botan::byte *>(data.data()) + pos);
    setLengthField(data);
    return data;
}

int AbstractSshPacket::paddingLength() const
{
    return m_data[PaddingLengthOffset];
}

quint32 AbstractSshPacket::length() const
{
    //Q_ASSERT(currentDataSize() >= minPacketSize());
    if (m_length == 0)
        calculateLength();
    return m_length;
}

void AbstractSshPacket::calculateLength() const
{
    m_length = SshPacketParser::asUint32(m_data, static_cast<quint32>(0));
}

QByteArray AbstractSshPacket::generateMac(const SshAbstractCryptoFacility &crypt,
    quint32 seqNr) const
{
    const quint32 seqNrBe = qToBigEndian(seqNr);
    QByteArray data(reinterpret_cast<const char *>(&seqNrBe), sizeof seqNrBe);
    data += QByteArray(m_data.constData(), length() + 4);
    return crypt.generateMac(data, data.size());
}

quint32 AbstractSshPacket::minPacketSize() const
{
    return qMax<quint32>(cipherBlockSize(), 16) + macLength();
}

void AbstractSshPacket::setLengthField(QByteArray &data)
{
    const quint32 length = qToBigEndian(data.size() - 4);
    data.replace(0, 4, reinterpret_cast<const char *>(&length), 4);
}

} // namespace Internal
} // namespace QSsh
