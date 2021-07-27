/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <botan/bigint.h>

#include <memory>

namespace Botan {
class Private_Key;
class RandomNumberGenerator;
}

namespace QSsh {
namespace Internal {

class OpenSshKeyFileReader
{
public:
    OpenSshKeyFileReader(Botan::RandomNumberGenerator &rng) : m_rng(rng) {}

    bool parseKey(const QByteArray &privKeyFileContents);
    QByteArray keyType() const { return m_keyType; }
    std::unique_ptr<Botan::Private_Key> privateKey() const;
    QList<Botan::BigInt> allParameters() const { return m_parameters; }
    QList<Botan::BigInt> publicParameters() const;

private:
    void doParse(const QByteArray &payload);
    void parseKdfOptions(const QByteArray &kdfOptions);
    void decryptPrivateKeyList();
    void parsePrivateKeyList();
    [[noreturn]] void throwException(const QString &reason);

    Botan::RandomNumberGenerator &m_rng;
    QByteArray m_keyType;
    QList<Botan::BigInt> m_parameters;
    QByteArray m_cipherName;
    QByteArray m_kdf;
    QByteArray m_salt;
    quint32 m_rounds;
    QByteArray m_privateKeyList;
};

} // namespace Internal
} // namespace QSsh

