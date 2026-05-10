// SPDX-License-Identifier: GPL-3.0-or-later

#include "wireframing.h"

#include <QDebug>
#include <QtEndian>
#include <QVector>

namespace amnezia::masterdnsvpn {

// ---------------------------------------------------------------------------
// Per-type extension table
// ---------------------------------------------------------------------------

HeaderExtensions headerExtensions(PacketType type)
{
    // Encoding shorthand: (S, N, F, C). Numeric column comments mirror the
    // packet-type catalogue in docs/masterdnsvpn-wire-spec.md §3.4.
    constexpr HeaderExtensions kNone { false, false, false, false };
    constexpr HeaderExtensions kSN { true, true, false, false };
    constexpr HeaderExtensions kSNF { true, true, true, false };
    constexpr HeaderExtensions kSNFC { true, true, true, true };
    constexpr HeaderExtensions kC { false, false, false, true };

    switch (type) {
    case PacketType::MtuUpReq:
        return kSNFC;
    case PacketType::MtuUpRes:
        return kNone;
    case PacketType::MtuDownReq:
        return kNone;
    case PacketType::MtuDownRes:
        return kSNFC;
    case PacketType::SessionInit:
    case PacketType::SessionAccept:
    case PacketType::Ping:
    case PacketType::Pong:
        return kNone;
    case PacketType::StreamSyn:
    case PacketType::StreamSynAck:
    case PacketType::StreamConnected:
    case PacketType::StreamConnectedAck:
    case PacketType::StreamConnectFail:
    case PacketType::StreamConnectFailAck:
        return kSN;
    case PacketType::StreamData:
        return kSNFC;
    case PacketType::StreamDataAck:
    case PacketType::StreamDataNack:
        return kSN;
    case PacketType::StreamResend:
        return kSNFC;
    case PacketType::PackedControlBlocks:
        return kC;
    case PacketType::StreamCloseWrite:
    case PacketType::StreamCloseWriteAck:
    case PacketType::StreamCloseRead:
    case PacketType::StreamCloseReadAck:
    case PacketType::StreamRst:
    case PacketType::StreamRstAck:
        return kSN;
    case PacketType::Socks5Syn:
        return kSNF;
    case PacketType::Socks5SynAck:
    case PacketType::Socks5ConnectFail:
    case PacketType::Socks5ConnectFailAck:
    case PacketType::Socks5RulesetDenied:
    case PacketType::Socks5RulesetDeniedAck:
    case PacketType::Socks5NetworkUnreachable:
    case PacketType::Socks5NetworkUnreachableAck:
    case PacketType::Socks5HostUnreachable:
    case PacketType::Socks5HostUnreachableAck:
    case PacketType::Socks5ConnectionRefused:
    case PacketType::Socks5ConnectionRefusedAck:
    case PacketType::Socks5TtlExpired:
    case PacketType::Socks5TtlExpiredAck:
    case PacketType::Socks5CommandUnsupported:
    case PacketType::Socks5CommandUnsupportedAck:
    case PacketType::Socks5AddressTypeUnsupported:
    case PacketType::Socks5AddressTypeUnsupportedAck:
    case PacketType::Socks5AuthFailed:
    case PacketType::Socks5AuthFailedAck:
    case PacketType::Socks5UpstreamUnavailable:
    case PacketType::Socks5UpstreamUnavailableAck:
    case PacketType::Socks5Connected:
    case PacketType::Socks5ConnectedAck:
        return kSN;
    case PacketType::DnsQueryReq:
    case PacketType::DnsQueryRes:
        return kSNFC;
    case PacketType::DnsQueryReqAck:
    case PacketType::DnsQueryResAck:
        return kSN;
    case PacketType::SessionClose:
    case PacketType::SessionBusy:
    case PacketType::ErrorDrop:
        return kNone;
    }
    // Unreachable in normal flow: any new packet type must be added above.
    return kNone;
}

bool isPackableControl(PacketType type)
{
    // §4.1 catalogue. Any type whose payload-empty form is bundleable
    // into PACKED_CONTROL_BLOCKS.
    switch (type) {
    case PacketType::StreamDataAck:
    case PacketType::StreamDataNack:
    case PacketType::StreamSynAck:
    case PacketType::StreamCloseWriteAck:
    case PacketType::StreamCloseReadAck:
    case PacketType::StreamRstAck:
    case PacketType::Socks5SynAck:
    case PacketType::StreamConnected:
    case PacketType::StreamConnectedAck:
    case PacketType::StreamConnectFail:
    case PacketType::StreamConnectFailAck:
    // The full SOCKS5 reply set + their ACK siblings are eligible.
    case PacketType::Socks5ConnectFail:
    case PacketType::Socks5ConnectFailAck:
    case PacketType::Socks5RulesetDenied:
    case PacketType::Socks5RulesetDeniedAck:
    case PacketType::Socks5NetworkUnreachable:
    case PacketType::Socks5NetworkUnreachableAck:
    case PacketType::Socks5HostUnreachable:
    case PacketType::Socks5HostUnreachableAck:
    case PacketType::Socks5ConnectionRefused:
    case PacketType::Socks5ConnectionRefusedAck:
    case PacketType::Socks5TtlExpired:
    case PacketType::Socks5TtlExpiredAck:
    case PacketType::Socks5CommandUnsupported:
    case PacketType::Socks5CommandUnsupportedAck:
    case PacketType::Socks5AddressTypeUnsupported:
    case PacketType::Socks5AddressTypeUnsupportedAck:
    case PacketType::Socks5AuthFailed:
    case PacketType::Socks5AuthFailedAck:
    case PacketType::Socks5UpstreamUnavailable:
    case PacketType::Socks5UpstreamUnavailableAck:
    case PacketType::Socks5Connected:
    case PacketType::Socks5ConnectedAck:
    case PacketType::DnsQueryReqAck:
    case PacketType::DnsQueryResAck:
        return true;
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// Header-check algorithm (§3.2)
// ---------------------------------------------------------------------------

quint8 computeCheck(const QByteArray &headerBytes)
{
    // Identical to the spec pseudocode; no rounds, single linear pass.
    const int n = headerBytes.size();
    quint32 acc = (static_cast<quint32>(n) * 17u + 0x5Du) & 0xFFu;
    for (int idx = 0; idx < n; ++idx) {
        const quint8 v = static_cast<quint8>(headerBytes[idx]);
        acc = (acc + v + static_cast<quint32>(idx)) & 0xFFu;
        const quint8 shifted = static_cast<quint8>(v << (idx & 0x03));
        acc = acc ^ shifted;
    }
    return static_cast<quint8>(acc & 0xFFu);
}

// ---------------------------------------------------------------------------
// encode / decode
// ---------------------------------------------------------------------------

QByteArray encode(const Packet &packet)
{
    const HeaderExtensions ext = headerExtensions(packet.type);

    QByteArray header;
    header.reserve(ext.headerBytes() - 1); // less the trailing check byte
    header.append(static_cast<char>(packet.sessionId));
    header.append(static_cast<char>(packet.type));

    auto appendU16 = [&header](quint16 v) {
        char buf[2];
        qToBigEndian<quint16>(v, buf);
        header.append(buf, 2);
    };

    if (ext.stream) {
        appendU16(packet.streamId.value_or(0));
    }
    if (ext.sequence) {
        appendU16(packet.sequenceNum.value_or(0));
    }
    if (ext.fragment) {
        header.append(static_cast<char>(packet.fragmentId.value_or(0)));
        header.append(static_cast<char>(packet.totalFragments.value_or(1)));
    }
    if (ext.compression) {
        header.append(static_cast<char>(packet.compression.value_or(0)));
    }
    header.append(static_cast<char>(packet.cookie));

    const quint8 check = computeCheck(header);
    header.append(static_cast<char>(check));
    header.append(packet.payload);
    return header;
}

std::optional<Packet> decode(const QByteArray &wire)
{
    if (wire.size() < 4) {
        return std::nullopt;
    }

    const quint8 sessionId = static_cast<quint8>(wire[0]);
    const quint8 typeByte = static_cast<quint8>(wire[1]);

    // Validate the type byte against the enum. The list of valid values is
    // exactly what headerExtensions() switches over.
    auto isValid = [](quint8 t) {
        switch (t) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
            return true;
        case 0xFF:
            return true;
        default:
            return t >= 0x1C && t <= 0x37;
        }
    };
    if (!isValid(typeByte)) {
        return std::nullopt;
    }
    const PacketType type = static_cast<PacketType>(typeByte);

    const HeaderExtensions ext = headerExtensions(type);
    const int headerLen = ext.headerBytes();
    if (wire.size() < headerLen) {
        return std::nullopt;
    }

    Packet out;
    out.sessionId = sessionId;
    out.type = type;

    int offset = 2;

    auto readU16 = [&](quint16 &dst) {
        dst = qFromBigEndian<quint16>(wire.constData() + offset);
        offset += 2;
    };

    if (ext.stream) {
        quint16 v = 0;
        readU16(v);
        out.streamId = v;
    }
    if (ext.sequence) {
        quint16 v = 0;
        readU16(v);
        out.sequenceNum = v;
    }
    if (ext.fragment) {
        out.fragmentId = static_cast<quint8>(wire[offset++]);
        out.totalFragments = static_cast<quint8>(wire[offset++]);
    }
    if (ext.compression) {
        out.compression = static_cast<quint8>(wire[offset++]);
    }
    out.cookie = static_cast<quint8>(wire[offset++]);

    // Trailing check byte. The check is computed over the headerLen-1 bytes
    // before it (i.e. everything from sessId through cookie inclusive).
    const quint8 expectedCheck = computeCheck(wire.left(offset));
    const quint8 actualCheck = static_cast<quint8>(wire[offset++]);
    if (expectedCheck != actualCheck) {
        return std::nullopt;
    }

    if (offset < wire.size()) {
        out.payload = wire.mid(offset);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Packed control blocks (§4)
// ---------------------------------------------------------------------------

QByteArray packBlocks(const QVector<PackedBlock> &blocks)
{
    QByteArray out;
    out.reserve(blocks.size() * 7);
    for (const PackedBlock &b : blocks) {
        out.append(static_cast<char>(b.type));
        char buf[2];
        qToBigEndian<quint16>(b.streamId, buf);
        out.append(buf, 2);
        qToBigEndian<quint16>(b.sequenceNum, buf);
        out.append(buf, 2);
        out.append(static_cast<char>(b.fragmentId));
        out.append(static_cast<char>(b.totalFragments));
    }
    return out;
}

QVector<PackedBlock> unpackBlocks(const QByteArray &payload)
{
    QVector<PackedBlock> out;
    int offset = 0;
    while (offset + 7 <= payload.size()) {
        PackedBlock b;
        b.type = static_cast<PacketType>(static_cast<quint8>(payload[offset]));
        b.streamId = qFromBigEndian<quint16>(payload.constData() + offset + 1);
        b.sequenceNum = qFromBigEndian<quint16>(payload.constData() + offset + 3);
        b.fragmentId = static_cast<quint8>(payload[offset + 5]);
        b.totalFragments = static_cast<quint8>(payload[offset + 6]);
        out.append(b);
        offset += 7;
    }
    return out;
}

} // namespace amnezia::masterdnsvpn
