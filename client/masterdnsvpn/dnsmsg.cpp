// SPDX-License-Identifier: GPL-3.0-or-later

#include "dnsmsg.h"

#include <QtEndian>

namespace amnezia::masterdnsvpn {

namespace {

// Walk the question section starting at `offset` in `wire`. Reads the
// QNAME (label sequence terminated by 0x00 or by a compression pointer
// 0xC0..0xFF — questions normally don't use pointers but we tolerate
// them by following one indirection), then QTYPE + QCLASS.
//
// Returns the parsed question + the offset *past* it on success.
// Returns nullopt on truncation, malformed labels, or pointer loops.
struct ParsedName {
    QString name;
    int nextOffset;
};

std::optional<ParsedName> readName(const QByteArray &wire, int offset)
{
    if (offset < 0 || offset >= wire.size()) {
        return std::nullopt;
    }
    QString out;
    int cur = offset;
    int returnOffset = -1; // set when we follow a pointer
    int hops = 0;
    constexpr int kMaxHops = 8; // guard against pointer loops

    while (cur < wire.size()) {
        const quint8 b = static_cast<quint8>(wire[cur]);
        if (b == 0) {
            ++cur;
            return ParsedName{ out, returnOffset > 0 ? returnOffset : cur };
        }
        if ((b & 0xC0) == 0xC0) {
            // Pointer: top two bits set, low 14 bits = offset.
            if (cur + 1 >= wire.size()) return std::nullopt;
            const int target = (static_cast<int>(b & 0x3F) << 8)
                    | static_cast<quint8>(wire[cur + 1]);
            // Original return position is right after the 2-byte pointer.
            if (returnOffset < 0) returnOffset = cur + 2;
            if (++hops > kMaxHops) return std::nullopt;
            if (target < 0 || target >= wire.size() || target == cur) {
                return std::nullopt;
            }
            cur = target;
            continue;
        }
        if ((b & 0xC0) != 0) {
            // Reserved label-type bits; reject.
            return std::nullopt;
        }
        const int labelLen = b;
        ++cur;
        if (labelLen > 63 || cur + labelLen > wire.size()) {
            return std::nullopt;
        }
        if (!out.isEmpty()) out.append('.');
        // Lowercase the label so cache keys are canonical.
        for (int i = 0; i < labelLen; ++i) {
            const char ch = wire[cur + i];
            if (ch >= 'A' && ch <= 'Z') {
                out.append(QChar(char(ch + ('a' - 'A'))));
            } else {
                out.append(QChar(ch));
            }
        }
        cur += labelLen;
    }
    return std::nullopt; // ran past end without finding terminator
}

} // namespace

std::optional<DnsLiteParse> parseDnsLite(const QByteArray &wire)
{
    if (wire.size() < kDnsHeaderSize) {
        return std::nullopt;
    }
    DnsLiteParse out;
    out.txid = qFromBigEndian<quint16>(wire.constData());
    const quint16 qdcount = qFromBigEndian<quint16>(wire.constData() + 4);
    if (qdcount == 0) {
        return out; // valid header, no question
    }
    auto parsedName = readName(wire, kDnsHeaderSize);
    if (!parsedName) {
        return out; // header is fine but question is malformed
    }
    const int afterName = parsedName->nextOffset;
    if (afterName + 4 > wire.size()) {
        return out;
    }
    out.firstQuestion.name = parsedName->name;
    out.firstQuestion.type = qFromBigEndian<quint16>(wire.constData() + afterName);
    out.firstQuestion.cls  = qFromBigEndian<quint16>(wire.constData() + afterName + 2);
    out.hasQuestion = true;
    return out;
}

QByteArray patchDnsTxid(const QByteArray &response, quint16 targetTxid)
{
    if (response.size() < 2) {
        return response;
    }
    QByteArray patched = response;
    char be[2];
    qToBigEndian<quint16>(targetTxid, be);
    patched[0] = be[0];
    patched[1] = be[1];
    return patched;
}

} // namespace amnezia::masterdnsvpn
