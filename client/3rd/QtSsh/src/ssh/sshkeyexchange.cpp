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

#include "sshkeyexchange_p.h"

#include "ssh_global.h"
#include "sshbotanconversions_p.h"
#include "sshcapabilities_p.h"
#include "sshlogging_p.h"
#include "sshsendfacility_p.h"
#include "sshexception_p.h"
#include "sshincomingpacket_p.h"

#include <botan/botan.h>

#include <string>

using namespace Botan;

namespace QSsh {
namespace Internal {

namespace {

    // For debugging
    void printNameList(const char *listName, const SshNameList &list)
    {
        qCDebug(sshLog, "%s:", listName);
        foreach (const QByteArray &name, list.names)
            qCDebug(sshLog, "%s", name.constData());
    }

    void printData(const char *name, const QByteArray &data)
    {
        qCDebug(sshLog, "The client thinks the %s has length %d and is: %s", name, data.count(),
                data.toHex().constData());
    }

} // anonymous namespace

SshKeyExchange::SshKeyExchange(const SshConnectionParameters &connParams,
                               SshSendFacility &sendFacility)
    : m_connParams(connParams), m_sendFacility(sendFacility)
{
}

SshKeyExchange::~SshKeyExchange() {}

void SshKeyExchange::sendKexInitPacket(const QByteArray &serverId)
{
    m_serverId = serverId;
    m_clientKexInitPayload = m_sendFacility.sendKeyExchangeInitPacket();
}

bool SshKeyExchange::sendDhInitPacket(const SshIncomingPacket &serverKexInit)
{
    qCDebug(sshLog, "server requests key exchange");
    serverKexInit.printRawBytes();
    SshKeyExchangeInit kexInitParams
            = serverKexInit.extractKeyExchangeInitData();

    printNameList("Key Algorithms", kexInitParams.keyAlgorithms);
    printNameList("Server Host Key Algorithms", kexInitParams.serverHostKeyAlgorithms);
    printNameList("Encryption algorithms client to server", kexInitParams.encryptionAlgorithmsClientToServer);
    printNameList("Encryption algorithms server to client", kexInitParams.encryptionAlgorithmsServerToClient);
    printNameList("MAC algorithms client to server", kexInitParams.macAlgorithmsClientToServer);
    printNameList("MAC algorithms server to client", kexInitParams.macAlgorithmsServerToClient);
    printNameList("Compression algorithms client to server", kexInitParams.compressionAlgorithmsClientToServer);
    printNameList("Compression algorithms client to server", kexInitParams.compressionAlgorithmsClientToServer);
    printNameList("Languages client to server", kexInitParams.languagesClientToServer);
    printNameList("Languages server to client", kexInitParams.languagesServerToClient);
    qCDebug(sshLog, "First packet follows: %d", kexInitParams.firstKexPacketFollows);

    m_kexAlgoName = SshCapabilities::findBestMatch(SshCapabilities::KeyExchangeMethods,
                                                   kexInitParams.keyAlgorithms.names);
    m_serverHostKeyAlgo = SshCapabilities::findBestMatch(SshCapabilities::PublicKeyAlgorithms,
            kexInitParams.serverHostKeyAlgorithms.names);
    determineHashingAlgorithm(kexInitParams, true);
    determineHashingAlgorithm(kexInitParams, false);

    m_encryptionAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::EncryptionAlgorithms,
              kexInitParams.encryptionAlgorithmsClientToServer.names);
    m_decryptionAlgo
        = SshCapabilities::findBestMatch(SshCapabilities::EncryptionAlgorithms,
              kexInitParams.encryptionAlgorithmsServerToClient.names);
    SshCapabilities::findBestMatch(SshCapabilities::CompressionAlgorithms,
        kexInitParams.compressionAlgorithmsClientToServer.names);
    SshCapabilities::findBestMatch(SshCapabilities::CompressionAlgorithms,
        kexInitParams.compressionAlgorithmsServerToClient.names);

    AutoSeeded_RNG rng;
    if (m_kexAlgoName.startsWith(SshCapabilities::EcdhKexNamePrefix)) {
        m_ecdhKey.reset(new ECDH_PrivateKey(rng, EC_Group(botanKeyExchangeAlgoName(m_kexAlgoName))));
        m_sendFacility.sendKeyEcdhInitPacket(convertByteArray(m_ecdhKey->public_value()));
    } else {
        m_dhKey.reset(new DH_PrivateKey(rng, DL_Group(botanKeyExchangeAlgoName(m_kexAlgoName))));
        m_sendFacility.sendKeyDhInitPacket(m_dhKey->get_y());
    }

    m_serverKexInitPayload = serverKexInit.payLoad();
    return kexInitParams.firstKexPacketFollows;
}

void SshKeyExchange::sendNewKeysPacket(const SshIncomingPacket &dhReply,
    const QByteArray &clientId)
{

    const SshKeyExchangeReply &reply
        = dhReply.extractKeyExchangeReply(m_kexAlgoName, m_serverHostKeyAlgo);
    if (m_dhKey && (reply.f <= 0 || reply.f >= m_dhKey->group_p())) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Server sent invalid f.");
    }

    QByteArray concatenatedData = AbstractSshPacket::encodeString(clientId);
    concatenatedData += AbstractSshPacket::encodeString(m_serverId);
    concatenatedData += AbstractSshPacket::encodeString(m_clientKexInitPayload);
    concatenatedData += AbstractSshPacket::encodeString(m_serverKexInitPayload);
    concatenatedData += reply.k_s;

    printData("Client Id", AbstractSshPacket::encodeString(clientId));
    printData("Server Id", AbstractSshPacket::encodeString(m_serverId));
    printData("Client Payload", AbstractSshPacket::encodeString(m_clientKexInitPayload));
    printData("Server payload", AbstractSshPacket::encodeString(m_serverKexInitPayload));
    printData("K_S", reply.k_s);

    SecureVector<byte> encodedK;
    if (m_dhKey) {
        concatenatedData += AbstractSshPacket::encodeMpInt(m_dhKey->get_y());
        concatenatedData += AbstractSshPacket::encodeMpInt(reply.f);
        DH_KA_Operation dhOp(*m_dhKey);
        SecureVector<byte> encodedF = BigInt::encode(reply.f);
        encodedK = dhOp.agree(encodedF, encodedF.size());
        printData("y", AbstractSshPacket::encodeMpInt(m_dhKey->get_y()));
        printData("f", AbstractSshPacket::encodeMpInt(reply.f));
        m_dhKey.reset(nullptr);
    } else {
        Q_ASSERT(m_ecdhKey);
        concatenatedData // Q_C.
                += AbstractSshPacket::encodeString(convertByteArray(m_ecdhKey->public_value()));
        concatenatedData += AbstractSshPacket::encodeString(reply.q_s);
        ECDH_KA_Operation ecdhOp(*m_ecdhKey);
        encodedK = ecdhOp.agree(convertByteArray(reply.q_s), reply.q_s.count());
        m_ecdhKey.reset(nullptr);
    }

    const BigInt k = BigInt::decode(encodedK);
    m_k = AbstractSshPacket::encodeMpInt(k); // Roundtrip, as Botan encodes BigInts somewhat differently.
    printData("K", m_k);
    concatenatedData += m_k;
    printData("Concatenated data", concatenatedData);

    m_hash.reset(get_hash(botanHMacAlgoName(hashAlgoForKexAlgo())));
    const SecureVector<byte> &hashResult = m_hash->process(convertByteArray(concatenatedData),
                                                           concatenatedData.size());
    m_h = convertByteArray(hashResult);
    printData("H", m_h);

    QScopedPointer<Public_Key> sigKey;
    if (m_serverHostKeyAlgo == SshCapabilities::PubKeyDss) {
        const DL_Group group(reply.hostKeyParameters.at(0), reply.hostKeyParameters.at(1),
            reply.hostKeyParameters.at(2));
        DSA_PublicKey * const dsaKey
            = new DSA_PublicKey(group, reply.hostKeyParameters.at(3));
        sigKey.reset(dsaKey);
    } else if (m_serverHostKeyAlgo == SshCapabilities::PubKeyRsa) {
        RSA_PublicKey * const rsaKey
            = new RSA_PublicKey(reply.hostKeyParameters.at(1), reply.hostKeyParameters.at(0));
        sigKey.reset(rsaKey);
    } else {
        QSSH_ASSERT_AND_RETURN(m_serverHostKeyAlgo.startsWith(SshCapabilities::PubKeyEcdsaPrefix));
        const EC_Group domain(SshCapabilities::oid(m_serverHostKeyAlgo));
        const PointGFp point = OS2ECP(convertByteArray(reply.q), reply.q.count(),
                                      domain.get_curve());
        ECDSA_PublicKey * const ecdsaKey = new ECDSA_PublicKey(domain, point);
        sigKey.reset(ecdsaKey);
    }

    const byte * const botanH = convertByteArray(m_h);
    const Botan::byte * const botanSig = convertByteArray(reply.signatureBlob);
    PK_Verifier verifier(*sigKey, botanEmsaAlgoName(m_serverHostKeyAlgo));
    if (!verifier.verify_message(botanH, m_h.size(), botanSig, reply.signatureBlob.size())) {
        throw SSH_SERVER_EXCEPTION(SSH_DISCONNECT_KEY_EXCHANGE_FAILED,
            "Invalid signature in key exchange reply packet.");
    }

    checkHostKey(reply.k_s);

    m_sendFacility.sendNewKeysPacket();
}

QByteArray SshKeyExchange::hashAlgoForKexAlgo() const
{
    if (m_kexAlgoName == SshCapabilities::EcdhNistp256)
        return SshCapabilities::HMacSha256;
    if (m_kexAlgoName == SshCapabilities::EcdhNistp384)
        return SshCapabilities::HMacSha384;
    if (m_kexAlgoName == SshCapabilities::EcdhNistp521)
        return SshCapabilities::HMacSha512;
    return SshCapabilities::HMacSha1;
}

void SshKeyExchange::determineHashingAlgorithm(const SshKeyExchangeInit &kexInit,
                                               bool serverToClient)
{
    QByteArray * const algo = serverToClient ? &m_s2cHMacAlgo : &m_c2sHMacAlgo;
    const QList<QByteArray> &serverCapabilities = serverToClient
            ? kexInit.macAlgorithmsServerToClient.names
            : kexInit.macAlgorithmsClientToServer.names;
    *algo = SshCapabilities::findBestMatch(SshCapabilities::MacAlgorithms, serverCapabilities);
}

void SshKeyExchange::checkHostKey(const QByteArray &hostKey)
{
    if (m_connParams.hostKeyCheckingMode == SshHostKeyCheckingNone) {
        if (m_connParams.hostKeyDatabase)
            m_connParams.hostKeyDatabase->insertHostKey(m_connParams.host, hostKey);
        return;
    }

    if (!m_connParams.hostKeyDatabase) {
        throw SshClientException(SshInternalError,
                                 SSH_TR("Host key database must exist "
                                        "if host key checking is enabled."));
    }

    switch (m_connParams.hostKeyDatabase->matchHostKey(m_connParams.host, hostKey)) {
    case SshHostKeyDatabase::KeyLookupMatch:
        return; // Nothing to do.
    case SshHostKeyDatabase::KeyLookupMismatch:
        if (m_connParams.hostKeyCheckingMode != SshHostKeyCheckingAllowMismatch)
            throwHostKeyException();
        break;
    case SshHostKeyDatabase::KeyLookupNoMatch:
        if (m_connParams.hostKeyCheckingMode == SshHostKeyCheckingStrict)
            throwHostKeyException();
        break;
    }
    m_connParams.hostKeyDatabase->insertHostKey(m_connParams.host, hostKey);
}

void SshKeyExchange::throwHostKeyException()
{
    throw SshServerException(SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE, "Host key changed",
                             SSH_TR("Host key of machine \"%1\" has changed.")
                             .arg(m_connParams.host));
}

} // namespace Internal
} // namespace QSsh
