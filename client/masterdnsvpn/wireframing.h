// SPDX-License-Identifier: GPL-3.0-or-later
//
// Wire framing — the inner-VPN-packet binary codec.
//
// A "packet" is a session-id byte, a packet-type byte, an ordered set of
// optional extensions (stream id / sequence number / fragment id+total /
// compression byte), a session cookie byte, a 1-byte rolling check, and an
// optional opaque payload. Per-type extension presence is fixed (§3.3 +
// §3.4 of docs/masterdnsvpn-wire-spec.md); this module is the source of
// truth for the on-the-wire bit layout.
//
// The codec is **pure** — no I/O, no Qt main-loop dependency, no dynamic
// allocation beyond the QByteArray output buffers. That makes it trivially
// unit-testable: see client/tests/testMasterDnsVpnWireFraming.cpp.
//
// Higher layers (encryption, base codec, DNS framing) sit *outside* this
// module: encode() returns the plaintext binary frame, decode() consumes
// the plaintext binary frame. The encryption layer wraps that frame.

#ifndef MASTERDNSVPN_WIREFRAMING_H
#define MASTERDNSVPN_WIREFRAMING_H

#include <QByteArray>
#include <QtGlobal>
#include <cstdint>
#include <optional>

namespace amnezia::masterdnsvpn {

// Packet types as published by upstream's Go enum. Numeric values are wire-
// stable; do not reorder. Reserved/unknown values are rejected on decode.
enum class PacketType : quint8 {
    MtuUpReq = 0x01,
    MtuUpRes = 0x02,
    MtuDownReq = 0x03,
    MtuDownRes = 0x04,
    SessionInit = 0x05,
    SessionAccept = 0x06,
    Ping = 0x07,
    Pong = 0x08,
    StreamSyn = 0x09,
    StreamSynAck = 0x0A,
    StreamConnected = 0x0B,
    StreamConnectedAck = 0x0C,
    StreamConnectFail = 0x0D,
    StreamConnectFailAck = 0x0E,
    StreamData = 0x0F,
    StreamDataAck = 0x10,
    StreamDataNack = 0x11,
    StreamResend = 0x12,
    PackedControlBlocks = 0x13,
    StreamCloseWrite = 0x14,
    StreamCloseWriteAck = 0x15,
    StreamCloseRead = 0x16,
    StreamCloseReadAck = 0x17,
    StreamRst = 0x18,
    StreamRstAck = 0x19,
    Socks5Syn = 0x1A,
    Socks5SynAck = 0x1B,
    // Per-failure-reason SOCKS5 reply types share a contiguous hex range
    // (0x1C..0x2F) including ACK siblings; see §3.4 of the spec for the
    // full table. Callers test membership via isSocks5ReplyType().
    Socks5ConnectFail = 0x1C,
    Socks5ConnectFailAck = 0x1D,
    Socks5RulesetDenied = 0x1E,
    Socks5RulesetDeniedAck = 0x1F,
    Socks5NetworkUnreachable = 0x20,
    Socks5NetworkUnreachableAck = 0x21,
    Socks5HostUnreachable = 0x22,
    Socks5HostUnreachableAck = 0x23,
    Socks5ConnectionRefused = 0x24,
    Socks5ConnectionRefusedAck = 0x25,
    Socks5TtlExpired = 0x26,
    Socks5TtlExpiredAck = 0x27,
    Socks5CommandUnsupported = 0x28,
    Socks5CommandUnsupportedAck = 0x29,
    Socks5AddressTypeUnsupported = 0x2A,
    Socks5AddressTypeUnsupportedAck = 0x2B,
    Socks5AuthFailed = 0x2C,
    Socks5AuthFailedAck = 0x2D,
    Socks5UpstreamUnavailable = 0x2E,
    Socks5UpstreamUnavailableAck = 0x2F,
    Socks5Connected = 0x30,
    Socks5ConnectedAck = 0x31,
    DnsQueryReq = 0x32,
    DnsQueryRes = 0x33,
    DnsQueryReqAck = 0x34,
    DnsQueryResAck = 0x35,
    SessionClose = 0x36,
    SessionBusy = 0x37,
    ErrorDrop = 0xFF,
};

// Inner packet — the structured form. encode() serialises into the
// binary wire format with header check applied; decode() validates the
// check and returns the structured form. Callers populate exactly the
// extensions enabled for their packet type (see headerExtensions()); the
// codec ignores extensions not enabled for the type during encode and
// returns std::nullopt during decode if the wire bytes don't match the
// expected layout.
struct Packet {
    quint8 sessionId = 0;
    PacketType type = PacketType::Ping;
    quint8 cookie = 0;

    // Extensions — only the ones flagged by headerExtensions(type) are
    // emitted on encode and parsed on decode. Defaults are ignored when
    // the extension is not enabled.
    std::optional<quint16> streamId;
    std::optional<quint16> sequenceNum;
    std::optional<quint8> fragmentId;
    std::optional<quint8> totalFragments;
    std::optional<quint8> compression;

    QByteArray payload;
};

// Bitset-like flag for header-extension presence. Stays a bitfield for the
// per-type table below.
struct HeaderExtensions {
    bool stream : 1;
    bool sequence : 1;
    bool fragment : 1;
    bool compression : 1;

    constexpr int extensionBytes() const
    {
        return (stream ? 2 : 0) + (sequence ? 2 : 0) + (fragment ? 2 : 0) + (compression ? 1 : 0);
    }

    // Total header length: 2 (sessId+type) + extensions + 2 (cookie+check).
    constexpr int headerBytes() const { return 4 + extensionBytes(); }
};

// Per-type extension table — the canonical map from a packet type to which
// optional fields are present. Source of truth: §3.3-§3.4.
HeaderExtensions headerExtensions(PacketType type);

// True if `type` is a "packable" control packet (§4.1) — i.e. eligible to
// be batched into a PACKED_CONTROL_BLOCKS container when its payload is
// empty.
bool isPackableControl(PacketType type);

// Encode the structured Packet into its binary wire form (plaintext —
// the encryption + base codec layers wrap on top). Returns the bytes
// suitable for passing to the encryption layer.
QByteArray encode(const Packet &packet);

// Inverse of encode(). Returns std::nullopt if:
//   - the input is too short for the type's expected layout, or
//   - the packet type byte is unknown, or
//   - the trailing 1-byte check disagrees with the computed check.
//
// On success, only the extension fields enabled for the decoded type are
// populated; the others remain std::nullopt.
std::optional<Packet> decode(const QByteArray &wire);

// 1-byte rolling check algorithm (§3.2). Exposed for the unit tests; the
// encode/decode pair already applies it.
quint8 computeCheck(const QByteArray &headerBytes);

// ----- Packed control blocks (§4) ---------------------------------------

struct PackedBlock {
    PacketType type = PacketType::Ping;
    quint16 streamId = 0;
    quint16 sequenceNum = 0;
    quint8 fragmentId = 0;
    quint8 totalFragments = 0;
};

// Pack N blocks into the PACKED_CONTROL_BLOCKS payload — exactly 7 bytes
// per block, no terminator. Caller wraps the output in a Packet of type
// PackedControlBlocks (Compression-only header).
QByteArray packBlocks(const QVector<PackedBlock> &blocks);

// Inverse — iterate while `offset + 7 <= len(payload)` and parse each block.
// The trailing partial block (≤ 6 bytes) is silently ignored, matching the
// reference receiver's behaviour.
QVector<PackedBlock> unpackBlocks(const QByteArray &payload);

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_WIREFRAMING_H
