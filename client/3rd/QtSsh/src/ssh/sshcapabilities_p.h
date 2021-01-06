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

namespace QSsh {
namespace Internal {

class SshCapabilities
{
public:
    static const QByteArray DiffieHellmanGroup1Sha1;
    static const QByteArray DiffieHellmanGroup14Sha1;
    static const QByteArray EcdhKexNamePrefix;
    static const QByteArray EcdhNistp256;
    static const QByteArray EcdhNistp384;
    static const QByteArray EcdhNistp521; // sic
    static const QList<QByteArray> KeyExchangeMethods;

    static const QByteArray PubKeyDss;
    static const QByteArray PubKeyRsa;
    static const QByteArray PubKeyEcdsaPrefix;
    static const QByteArray PubKeyEcdsa256;
    static const QByteArray PubKeyEcdsa384;
    static const QByteArray PubKeyEcdsa521;
    static const QList<QByteArray> PublicKeyAlgorithms;

    static const QByteArray CryptAlgo3DesCbc;
    static const QByteArray CryptAlgo3DesCtr;
    static const QByteArray CryptAlgoAes128Cbc;
    static const QByteArray CryptAlgoAes128Ctr;
    static const QByteArray CryptAlgoAes192Ctr;
    static const QByteArray CryptAlgoAes256Ctr;
    static const QList<QByteArray> EncryptionAlgorithms;

    static const QByteArray HMacSha1;
    static const QByteArray HMacSha196;
    static const QByteArray HMacSha256;
    static const QByteArray HMacSha384;
    static const QByteArray HMacSha512;
    static const QList<QByteArray> MacAlgorithms;

    static const QList<QByteArray> CompressionAlgorithms;

    static const QByteArray SshConnectionService;

    static QList<QByteArray> commonCapabilities(const QList<QByteArray> &myCapabilities,
                                                const QList<QByteArray> &serverCapabilities);
    static QByteArray findBestMatch(const QList<QByteArray> &myCapabilities,
        const QList<QByteArray> &serverCapabilities);

    static int ecdsaIntegerWidthInBytes(const QByteArray &ecdsaAlgo);
    static QByteArray ecdsaPubKeyAlgoForKeyWidth(int keyWidthInBytes);
    static const char *oid(const QByteArray &ecdsaAlgo);
};

} // namespace Internal
} // namespace QSsh
