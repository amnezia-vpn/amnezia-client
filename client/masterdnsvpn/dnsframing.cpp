// SPDX-License-Identifier: GPL-3.0-or-later

#include "dnsframing.h"

#include <QByteArray>
#include <QDebug>
#include <QtEndian>
#include <array>
#include <cstring>

namespace amnezia::masterdnsvpn {

// ---------------------------------------------------------------------------
// base36 codec (§5.6)
// ---------------------------------------------------------------------------

namespace {

constexpr char kBase36Alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";

// Lookup table: ASCII char -> base36 digit (0..35) or 0xFF for invalid.
// Built once at startup; the spec is case-insensitive.
const std::array<quint8, 256> kBase36DecodeTable = []() {
    std::array<quint8, 256> t {};
    t.fill(0xFF);
    for (int i = 0; i < 36; ++i) {
        t[static_cast<unsigned char>(kBase36Alphabet[i])] = static_cast<quint8>(i);
    }
    // Case-insensitive — uppercase letters map to the same digit as lowercase.
    for (int i = 'A'; i <= 'Z'; ++i) {
        t[i] = t[i - ('A' - 'a')];
    }
    return t;
}();

// Tail-byte → encoded-char count table. 0/8/9/10 indicate "block" lengths
// the spec doesn't define; the encoder never produces them.
constexpr std::array<int, 8> kBase36TailEncodedLen = { 0, 2, 4, 5, 7, 8, 10, 11 };

// Maximum decimal value representable by k base-36 digits, used as a parse
// guard — not currently consulted because the writer/reader are paired,
// but kept for completeness.

// Pack 7 input bytes into 11 base-36 chars. `bytesIn` may be 1..7. The
// reverse table above is the size table.
void packChunk(const quint8 *bytes, int bytesIn, char *out, int charsOut)
{
    Q_ASSERT(bytesIn >= 1 && bytesIn <= 7);
    Q_ASSERT(charsOut == kBase36TailEncodedLen[bytesIn]);

    quint64 value = 0;
    for (int i = 0; i < bytesIn; ++i) {
        value = (value << 8) | bytes[i];
    }
    // Emit chars most-significant first.
    for (int i = charsOut - 1; i >= 0; --i) {
        out[i] = kBase36Alphabet[value % 36];
        value /= 36;
    }
}

// Inverse: read `charsIn` base-36 digits, produce `bytesOut` output bytes.
// Returns false if any digit is non-base36.
bool unpackChunk(const char *chars, int charsIn, quint8 *out, int bytesOut)
{
    quint64 value = 0;
    for (int i = 0; i < charsIn; ++i) {
        const quint8 d = kBase36DecodeTable[static_cast<unsigned char>(chars[i])];
        if (d == 0xFF) {
            return false;
        }
        value = value * 36 + d;
    }
    for (int i = bytesOut - 1; i >= 0; --i) {
        out[i] = static_cast<quint8>(value & 0xFFu);
        value >>= 8;
    }
    return true;
}

int base36CharsForTail(int tailBytes)
{
    if (tailBytes < 0 || tailBytes > 7) {
        return -1;
    }
    return kBase36TailEncodedLen[tailBytes];
}

int base36TailForChars(int charsMod11)
{
    // Inverse map. Values 1, 3, 6, 9 are invalid per §5.6.
    switch (charsMod11) {
    case 0: return 7; // wraps to next-block; means "no tail" if block is full
    case 2: return 1;
    case 4: return 2;
    case 5: return 3;
    case 7: return 4;
    case 8: return 5;
    case 10: return 6;
    default: return -1;
    }
}

} // namespace

QByteArray encodeBase36(const QByteArray &raw)
{
    QByteArray out;
    int blocks = raw.size() / 7;
    int tail = raw.size() - blocks * 7;
    out.reserve(blocks * 11 + base36CharsForTail(tail));

    const quint8 *p = reinterpret_cast<const quint8 *>(raw.constData());
    for (int b = 0; b < blocks; ++b) {
        char buf[11];
        packChunk(p + b * 7, 7, buf, 11);
        out.append(buf, 11);
    }
    if (tail > 0) {
        const int outLen = base36CharsForTail(tail);
        QByteArray tailBuf(outLen, '\0');
        packChunk(p + blocks * 7, tail, tailBuf.data(), outLen);
        out.append(tailBuf);
    }
    return out;
}

std::optional<QByteArray> decodeBase36(const QByteArray &encoded)
{
    const int len = encoded.size();
    if (len == 0) {
        return QByteArray();
    }
    const int blocks = len / 11;
    const int tailChars = len - blocks * 11;
    const int tailBytes = base36TailForChars(tailChars);
    if (tailBytes < 0) {
        return std::nullopt;
    }

    // The encoder treats tail==7 as "next block" but we already accounted
    // for whole blocks above; tailChars==0 → no tail.
    const int extraBytes = (tailChars == 0) ? 0 : tailBytes;

    QByteArray out;
    out.resize(blocks * 7 + extraBytes);

    quint8 *q = reinterpret_cast<quint8 *>(out.data());
    const char *src = encoded.constData();
    for (int b = 0; b < blocks; ++b) {
        if (!unpackChunk(src + b * 11, 11, q + b * 7, 7)) {
            return std::nullopt;
        }
    }
    if (extraBytes > 0) {
        if (!unpackChunk(src + blocks * 11, tailChars, q + blocks * 7, extraBytes)) {
            return std::nullopt;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// base32 codec (§5.5 alternative)
// ---------------------------------------------------------------------------

namespace {

constexpr char kBase32Alphabet[] = "abcdefghijklmnopqrstuvwxyz234567";

const std::array<quint8, 256> kBase32DecodeTable = []() {
    std::array<quint8, 256> t {};
    t.fill(0xFF);
    for (int i = 0; i < 32; ++i) {
        t[static_cast<unsigned char>(kBase32Alphabet[i])] = static_cast<quint8>(i);
    }
    for (int i = 'A'; i <= 'Z'; ++i) {
        t[i] = t[i - ('A' - 'a')];
    }
    return t;
}();

constexpr std::array<int, 6> kBase32TailEncodedLen = { 0, 2, 4, 5, 7, 8 };

} // namespace

QByteArray encodeBase32(const QByteArray &raw)
{
    QByteArray out;
    const int blocks = raw.size() / 5;
    const int tail = raw.size() - blocks * 5;
    out.reserve(blocks * 8 + kBase32TailEncodedLen[tail]);

    const quint8 *p = reinterpret_cast<const quint8 *>(raw.constData());

    auto emitBlock = [&out](const quint8 *src, int srcLen, int outLen) {
        // Pack srcLen bytes into a 40-bit accumulator (big-endian); take the
        // top `outLen * 5` bits as base32 digits, MSB first.
        quint64 acc = 0;
        for (int i = 0; i < srcLen; ++i) {
            acc = (acc << 8) | src[i];
        }
        // Pad on the right so the digit count equals `outLen`.
        const int padBits = outLen * 5 - srcLen * 8;
        Q_ASSERT(padBits >= 0 && padBits < 5);
        acc <<= padBits;
        for (int i = outLen - 1; i >= 0; --i) {
            out.append(kBase32Alphabet[acc & 0x1Fu]);
            acc >>= 5;
        }
        // Output is built right-to-left into the *next* outLen bytes — reverse.
        const int newSize = out.size();
        std::reverse(out.data() + newSize - outLen, out.data() + newSize);
    };

    for (int b = 0; b < blocks; ++b) {
        emitBlock(p + b * 5, 5, 8);
    }
    if (tail > 0) {
        emitBlock(p + blocks * 5, tail, kBase32TailEncodedLen[tail]);
    }
    return out;
}

std::optional<QByteArray> decodeBase32(const QByteArray &encoded)
{
    const int len = encoded.size();
    if (len == 0) {
        return QByteArray();
    }
    const int blocks = len / 8;
    const int tail = len - blocks * 8;

    int tailBytes = 0;
    switch (tail) {
    case 0: tailBytes = 0; break;
    case 2: tailBytes = 1; break;
    case 4: tailBytes = 2; break;
    case 5: tailBytes = 3; break;
    case 7: tailBytes = 4; break;
    default: return std::nullopt;
    }

    QByteArray out;
    out.resize(blocks * 5 + tailBytes);
    quint8 *q = reinterpret_cast<quint8 *>(out.data());
    const char *src = encoded.constData();

    auto decodeBlock = [&](const char *in, int charsIn, quint8 *o, int bytesOut) {
        quint64 acc = 0;
        for (int i = 0; i < charsIn; ++i) {
            const quint8 d = kBase32DecodeTable[static_cast<unsigned char>(in[i])];
            if (d == 0xFF) {
                return false;
            }
            acc = (acc << 5) | d;
        }
        const int padBits = charsIn * 5 - bytesOut * 8;
        if (padBits < 0 || padBits >= 5) {
            return false;
        }
        acc >>= padBits;
        for (int i = bytesOut - 1; i >= 0; --i) {
            o[i] = static_cast<quint8>(acc & 0xFFu);
            acc >>= 8;
        }
        return true;
    };

    for (int b = 0; b < blocks; ++b) {
        if (!decodeBlock(src + b * 8, 8, q + b * 5, 5)) {
            return std::nullopt;
        }
    }
    if (tailBytes > 0) {
        if (!decodeBlock(src + blocks * 8, tail, q + blocks * 5, tailBytes)) {
            return std::nullopt;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// DNS query construction (§2.1)
// ---------------------------------------------------------------------------

namespace {

// kDnsRecordTypeTxt / kDnsQClassIn are exported from dnsframing.h —
// referenced here directly without re-declaring.
constexpr quint16 kFlagsClientStdQueryRd = 0x0100;

// Append a length-prefixed label sequence for `domain` followed by the
// terminating null label. Throws (returns false) if any individual label
// exceeds 63 bytes or if the cumulative length would exceed 253.
//
// We intentionally don't unicode-normalise the domain — operators give us
// plain ASCII. IDN handling is outside the scope of this protocol.
bool appendLabelsForDomain(QByteArray &out, const QString &domain)
{
    int totalNameLen = 0;
    const QStringList parts = domain.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    for (const QString &label : parts) {
        const QByteArray asciiLabel = label.toLatin1();
        if (asciiLabel.size() > 63) {
            qWarning("masterdnsvpn::dnsframing: domain label too long");
            return false;
        }
        // +1 for the length byte itself.
        if (totalNameLen + 1 + asciiLabel.size() + 1 > 253) {
            qWarning("masterdnsvpn::dnsframing: QNAME too long");
            return false;
        }
        out.append(static_cast<char>(asciiLabel.size()));
        out.append(asciiLabel);
        totalNameLen += 1 + asciiLabel.size();
    }
    out.append('\0');
    return true;
}

void appendU16(QByteArray &out, quint16 v)
{
    char buf[2];
    qToBigEndian<quint16>(v, buf);
    out.append(buf, 2);
}

} // namespace

int maxFrameBytes(const QString &domain, bool useBase32)
{
    // Compute the budget left for the encoded payload after subtracting the
    // domain labels and their length bytes (and the terminating root null).
    // QNAME hard cap = 253 bytes; one length byte per label; one extra byte
    // separates the encoded run from the domain.
    const QStringList parts = domain.split(QLatin1Char('.'), Qt::SkipEmptyParts);
    int domainLen = 1; // terminating null
    for (const QString &p : parts) {
        domainLen += 1 + p.toLatin1().size();
    }
    // Reserve one length byte for the encoded-run separator and account for
    // the per-63-byte label length bytes we'll add when chunking the encoded
    // string. Approximation: ceil(encoded/63) label-length bytes.
    const int encodedBudget = 253 - domainLen;
    if (encodedBudget <= 0) {
        return 0;
    }
    // Each 63-byte label needs 1 length byte. Solve N + ceil(N/63) <= budget
    // for N (encoded chars). Approximation: N ≈ budget * 63/64.
    const int encodedChars = (encodedBudget * 63) / 64;
    if (useBase32) {
        // 5 bytes -> 8 chars. So bytes ≈ floor(chars * 5 / 8).
        return (encodedChars * 5) / 8;
    }
    // base36: 7 bytes -> 11 chars.
    return (encodedChars * 7) / 11;
}

QByteArray buildQuery(quint16 transactionId,
                      const QByteArray &encodedFrame,
                      const QString &domain)
{
    QByteArray out;
    out.reserve(64 + encodedFrame.size() + domain.size());

    // DNS header
    appendU16(out, transactionId);
    appendU16(out, kFlagsClientStdQueryRd);
    appendU16(out, 1); // QDCount
    appendU16(out, 0); // ANCount
    appendU16(out, 0); // NSCount
    appendU16(out, 1); // ARCount — for the EDNS(0) OPT below

    // QNAME: split encoded frame into 63-byte labels, then append domain.
    int offset = 0;
    while (offset < encodedFrame.size()) {
        const int chunk = std::min<int>(63, encodedFrame.size() - offset);
        out.append(static_cast<char>(chunk));
        out.append(encodedFrame.constData() + offset, chunk);
        offset += chunk;
    }
    if (!appendLabelsForDomain(out, domain)) {
        return {};
    }

    // QTYPE + QCLASS
    appendU16(out, kDnsRecordTypeTxt);
    appendU16(out, kDnsQClassIn);

    // EDNS(0) OPT pseudo-RR (11 bytes total).
    out.append(static_cast<char>(0x00)); // root name
    appendU16(out, 41); // TYPE = OPT
    appendU16(out, 4096); // UDP payload size
    out.append(static_cast<char>(0x00)); // ext RCODE
    out.append(static_cast<char>(0x00)); // version
    appendU16(out, 0); // flags (DO bit clear)
    appendU16(out, 0); // RDLEN

    return out;
}

// ---------------------------------------------------------------------------
// DNS response parsing (§2.2)
// ---------------------------------------------------------------------------

namespace {

// Skip a DNS name in `wire` starting at `pos`. Returns the new position
// (after the terminating null or following a single pointer), or -1 on
// malformed input. Compression pointers are followed but only one level is
// validated; the wire format never uses deeper pointers in this protocol.
int skipName(const QByteArray &wire, int pos)
{
    int p = pos;
    int hops = 0;
    while (p < wire.size()) {
        const quint8 b = static_cast<quint8>(wire[p]);
        if (b == 0) {
            return p + 1;
        }
        if ((b & 0xC0) == 0xC0) {
            // Compression pointer (2 bytes total).
            if (p + 1 >= wire.size()) {
                return -1;
            }
            return p + 2;
        }
        if (b > 63) {
            return -1; // reserved label types
        }
        p += 1 + b;
        if (++hops > 128) {
            return -1; // sanity
        }
    }
    return -1;
}

// Parse a sequence of length-prefixed character-strings inside an RDATA
// blob. Concatenates the bytes in order (length bytes are stripped).
// Returns the concatenated payload, or std::nullopt on malformed input.
std::optional<QByteArray> parseRdataChunk(const QByteArray &rdata)
{
    QByteArray out;
    int p = 0;
    while (p < rdata.size()) {
        const int strLen = static_cast<quint8>(rdata[p]);
        if (p + 1 + strLen > rdata.size()) {
            return std::nullopt;
        }
        out.append(rdata.constData() + p + 1, strLen);
        p += 1 + strLen;
    }
    return out;
}

QByteArray decodeBase64Bytes(const QByteArray &input)
{
    return QByteArray::fromBase64(input);
}

} // namespace

std::optional<DnsResponse> parseResponse(const QByteArray &wire, bool wasBase64Mode)
{
    if (wire.size() < 12) {
        return std::nullopt;
    }
    const quint16 txId = qFromBigEndian<quint16>(wire.constData());
    const quint16 flags = qFromBigEndian<quint16>(wire.constData() + 2);
    const quint16 qdCount = qFromBigEndian<quint16>(wire.constData() + 4);
    const quint16 anCount = qFromBigEndian<quint16>(wire.constData() + 6);

    DnsResponse out;
    out.transactionId = txId;
    out.rcode = static_cast<quint8>(flags & 0x000F);

    if (out.rcode != 0) {
        // Server signalled an error; no payload to parse.
        return out;
    }
    if (qdCount != 1 || anCount == 0) {
        return std::nullopt;
    }

    // Skip the question section: name + 4 bytes (QTYPE+QCLASS).
    int pos = 12;
    pos = skipName(wire, pos);
    if (pos < 0 || pos + 4 > wire.size()) {
        return std::nullopt;
    }
    pos += 4;

    // Collect every TXT chunk; later we order by chunk-index header byte.
    QVector<QByteArray> chunks;
    chunks.reserve(anCount);

    for (int i = 0; i < anCount; ++i) {
        pos = skipName(wire, pos);
        if (pos < 0 || pos + 10 > wire.size()) {
            return std::nullopt;
        }
        const quint16 type = qFromBigEndian<quint16>(wire.constData() + pos);
        // const quint16 cls = qFromBigEndian<quint16>(wire.constData() + pos + 2);
        // const quint32 ttl = qFromBigEndian<quint32>(wire.constData() + pos + 4);
        const quint16 rdLen = qFromBigEndian<quint16>(wire.constData() + pos + 8);
        pos += 10;
        if (pos + rdLen > wire.size()) {
            return std::nullopt;
        }
        if (type != kDnsRecordTypeTxt) {
            // Skip non-TXT — could be OPT or other ARs; not fatal.
            pos += rdLen;
            continue;
        }

        const QByteArray rdata = wire.mid(pos, rdLen);
        pos += rdLen;
        auto payload = parseRdataChunk(rdata);
        if (!payload) {
            return std::nullopt;
        }
        QByteArray chunk = wasBase64Mode ? decodeBase64Bytes(*payload) : *payload;
        chunks.append(chunk);
    }

    if (chunks.isEmpty()) {
        return std::nullopt;
    }

    // Single-chunk response — no chunk header (per §2.2 special case).
    if (chunks.size() == 1) {
        out.frame = chunks[0];
        return out;
    }

    // Multi-chunk: first chunk is `0x00 <total> <payload...>`. Subsequent
    // chunks are `<chunk_index> <payload...>`. Order by index.
    QByteArray first = chunks[0];
    if (first.size() < 2 || static_cast<quint8>(first[0]) != 0x00) {
        return std::nullopt;
    }
    const int total = static_cast<quint8>(first[1]);
    if (total == 0 || total > chunks.size()) {
        return std::nullopt;
    }

    QVector<QByteArray> ordered(total);
    ordered[0] = first.mid(2);

    for (int i = 1; i < chunks.size(); ++i) {
        const QByteArray &c = chunks[i];
        if (c.isEmpty()) {
            return std::nullopt;
        }
        const int idx = static_cast<quint8>(c[0]);
        if (idx <= 0 || idx >= total) {
            return std::nullopt;
        }
        if (!ordered[idx].isEmpty()) {
            return std::nullopt; // duplicate index
        }
        ordered[idx] = c.mid(1);
    }
    for (const QByteArray &part : ordered) {
        out.frame.append(part);
    }
    return out;
}

} // namespace amnezia::masterdnsvpn
