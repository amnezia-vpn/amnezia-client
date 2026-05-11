// SPDX-License-Identifier: GPL-3.0-or-later
//
// DNS message lite-parsing.
//
// RFC 1035 §4 reserves the first 12 bytes of every DNS message as a fixed
// header (transaction-id, flags, four section counts). The body holds the
// question + answer + authority + additional sections in label-compressed
// wire format.
//
// The MasterDnsVPN client only needs two things out of an inbound DNS
// message: the transaction-id (to pair the cached response with a fresh
// query) and the first question (as the cache key). We do NOT need to
// walk answers or honour pointer compression beyond what the question
// section itself uses — questions never reference earlier names since
// they precede everything.
//
// This is a **lite** parser by deliberate design — mirroring upstream's
// `dnsparser.ParseDNSRequestLite` (internal/dnsparser/parser_lite.go).
// Robust full parsing is the DNS resolver's job; we just need a key.

#ifndef MASTERDNSVPN_DNSMSG_H
#define MASTERDNSVPN_DNSMSG_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <optional>

namespace amnezia::masterdnsvpn {

// Header layout (RFC 1035 §4.1.1, network byte order):
//   bytes  0..1 : transaction id
//   bytes  2..3 : flags (QR/Opcode/AA/TC/RD/RA/Z/RCODE)
//   bytes  4..5 : QDCOUNT (question count)
//   bytes  6..7 : ANCOUNT
//   bytes  8..9 : NSCOUNT
//   bytes 10..11: ARCOUNT
constexpr int kDnsHeaderSize = 12;

// A single DNS question. `name` is the lowercase canonical form (labels
// joined with '.', no trailing dot). `type` is the QTYPE (e.g. A=1,
// AAAA=28, TXT=16). `class` is the QCLASS (almost always IN=1).
struct DnsQuestion {
    QString name;
    quint16 type = 0;
    quint16 cls = 0;
};

// Lite-parse result. Successful parse always sets `txid` and either
// `hasQuestion=true` with `firstQuestion` populated, or `hasQuestion=false`
// for malformed/QDCOUNT=0 inputs (the latter is rare but valid — e.g.
// some EDNS-pad probes).
struct DnsLiteParse {
    quint16 txid = 0;
    bool hasQuestion = false;
    DnsQuestion firstQuestion;
};

// Parse just enough of a DNS query/response to extract the transaction
// id and (if present) the first question. Returns std::nullopt for
// inputs shorter than 12 bytes (no header). On structurally invalid
// question sections returns `hasQuestion=false` rather than nullopt —
// caches still benefit from the txid even when keying fails.
std::optional<DnsLiteParse> parseDnsLite(const QByteArray &wire);

// Rewrite the transaction-id (bytes 0..1) of `response` to match
// `targetTxid`. Used by the local cache to patch a stored response so it
// looks like a direct reply to the inbound query. Returns a copy.
QByteArray patchDnsTxid(const QByteArray &response, quint16 targetTxid);

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_DNSMSG_H
