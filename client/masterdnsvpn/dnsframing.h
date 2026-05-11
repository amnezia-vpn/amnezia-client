// SPDX-License-Identifier: GPL-3.0-or-later
//
// DNS framing — RFC 1035 question/answer construction + the lower-base36
// label codec MasterDnsVPN uses to stuff a binary frame into a QNAME.
//
// Two halves:
//
//   * **Codec** (base36 / base32): bit-packing of bytes into DNS-label-safe
//     ASCII. Default is lower-base36 (packs 7 bytes -> 11 chars); lower-
//     base32 is also implemented because the spec advertises it as a
//     selectable alternative. See §5.5-§5.6.
//
//   * **DNS framing**: builds a TXT-question DNS query whose QNAME is
//     `<encoded-frame>.<DOMAIN>` and parses TXT-RR answer chunks back into
//     the binary frame (with the 2-byte chunk header stripping per §2.2).
//
// Both halves are pure data layers — no I/O, no Qt main loop. Higher
// layers (resolverpool.cpp) marshal the resulting bytes to the wire over
// QUdpSocket.

#ifndef MASTERDNSVPN_DNSFRAMING_H
#define MASTERDNSVPN_DNSFRAMING_H

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QtGlobal>
#include <optional>

namespace amnezia::masterdnsvpn {

// ---- DNS protocol constants (RFC 1035 / RFC 6891) ------------------------
//
// Exposed publicly so test code can pin the wire values. The naming mirrors
// upstream's enums package (internal/enums/dns.go) — we expose only the
// subset the client engine references; the full DNS qtype table lives in
// upstream Go but the client only ever needs TXT/OPT in practice.

// DNS QTYPE — record type a question asks about (RFC 1035 §3.2.2).
constexpr quint16 kDnsRecordTypeTxt = 16;
constexpr quint16 kDnsRecordTypeOpt = 41;

// DNS QCLASS — record class (RFC 1035 §3.2.4). MasterDnsVPN only uses IN.
constexpr quint16 kDnsQClassIn = 1;

// DNS RCODE — server response code (RFC 1035 §4.1.1). The full table runs
// 0..10 but the client only acts on NO_ERROR / REFUSED in practice.
constexpr quint8 kDnsRCodeNoError       = 0;
constexpr quint8 kDnsRCodeFormatError   = 1;
constexpr quint8 kDnsRCodeServerFailure = 2;
constexpr quint8 kDnsRCodeNameError     = 3;
constexpr quint8 kDnsRCodeNotImpl       = 4;
constexpr quint8 kDnsRCodeRefused       = 5;

// ---- base36 / base32 codec ------------------------------------------------

// Encode raw bytes into the lower-base36 alphabet defined by §5.6.
// 7 input bytes -> 11 output chars; tails handled per the spec table.
QByteArray encodeBase36(const QByteArray &raw);

// Decode an ASCII (case-insensitive) lower-base36 string. Returns
// std::nullopt for invalid lengths (mod-11 ∈ {1, 3, 6, 9}) or invalid
// characters.
std::optional<QByteArray> decodeBase36(const QByteArray &encoded);

// Lower-base32 alternative. Alphabet is `abcdefghijklmnopqrstuvwxyz234567`.
// 5 input bytes -> 8 output chars; tails handled per RFC 4648 (no padding,
// since DNS labels have no padding character). Implemented because §5.5
// advertises it as a selectable encoder; the wire format is otherwise
// identical to base36.
QByteArray encodeBase32(const QByteArray &raw);
std::optional<QByteArray> decodeBase32(const QByteArray &encoded);

// ---- DNS query construction ----------------------------------------------

// Build the wire bytes for a DNS query whose QNAME is `<encoded>.<domain>`,
// QTYPE=TXT, QCLASS=IN. Includes the EDNS(0) OPT pseudo-RR (UDP size
// 4096) per §2.1.
//
// `transactionId` is echoed by the server; allocate a fresh one per
// outstanding query so concurrent responses can be paired up. Returns the
// raw bytes ready to send via QUdpSocket::writeDatagram().
//
// The label run is constructed by splitting the encoded frame into ≤ 63
// byte chunks. The full QNAME (encoded frame + domain + length bytes) must
// not exceed 253 bytes — encoding logic exposed via maxFrameBytes().
QByteArray buildQuery(quint16 transactionId,
                      const QByteArray &encodedFrame,
                      const QString &domain);

// Maximum number of *raw* (pre-encoding) bytes that fit into one DNS query
// envelope, given the operator's tunnel domain. Useful for the MTU layer.
//
// `useBase32` selects the upstream-supported alternative codec (otherwise
// the default base36 is assumed).
int maxFrameBytes(const QString &domain, bool useBase32 = false);

// ---- DNS response parsing ------------------------------------------------

struct DnsResponse {
    quint16 transactionId = 0;

    // The reassembled tunnel frame, with the chunk header (`0x00 <total>` for
    // multi-chunk responses) already stripped. Empty if the response had no
    // payload (e.g. RCODE != NOERROR).
    QByteArray frame;

    // Raw RCODE — non-zero means the server returned an error rather than a
    // tunnel frame. Caller decides whether to retry / abandon.
    quint8 rcode = 0;
};

// Parse a DNS response packet received on the UDP socket. Returns
// std::nullopt for structurally-invalid responses (truncated header, bad
// section counts, etc.). On parse success but RCODE != 0, `frame` is empty
// and `rcode` is populated; caller decides what to do.
//
// `wasBase64Mode` selects between the two response-mode encodings the
// server offers (§2.2 + §7.1 byte 0). When true, each chunk's payload is
// base64-decoded before being appended to the reassembled frame.
std::optional<DnsResponse> parseResponse(const QByteArray &wire, bool wasBase64Mode);

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_DNSFRAMING_H
