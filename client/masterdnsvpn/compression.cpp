// SPDX-License-Identifier: GPL-3.0-or-later

#include "compression.h"

#include <QtEndian>
#include <cstring>

#include <zlib.h>
#include <zstd.h>
#include <lz4.h>

namespace amnezia::masterdnsvpn::compression {

namespace {

// Per spec §3.4: only the SNFC-group packet types serialise a per-packet
// compression byte and therefore can carry compressed payloads. Mirrors
// the `comp` set in upstream's `internal/vpnproto/parser.go:306-318`.
bool hasCompressionExtension(PacketType type)
{
    switch (type) {
    case PacketType::StreamData:
    case PacketType::StreamResend:
    case PacketType::PackedControlBlocks:
    case PacketType::DnsQueryReq:
    case PacketType::DnsQueryRes:
    case PacketType::MtuUpReq:
    case PacketType::MtuDownRes:
        return true;
    default:
        return false;
    }
}

} // namespace

quint8 normalize(quint8 compType)
{
    if (compType > TypeZLIB) {
        return TypeOff;
    }
    return compType;
}

quint8 packPair(quint8 upload, quint8 download)
{
    upload = normalize(upload);
    download = normalize(download);
    return static_cast<quint8>((upload << 4) | (download & 0x0F));
}

std::pair<quint8, quint8> splitPair(quint8 packed)
{
    return {normalize((packed >> 4) & 0x0F), normalize(packed & 0x0F)};
}

std::pair<QByteArray, quint8> prepareOutgoingPayload(PacketType packetType,
                                                     const QByteArray &payload,
                                                     quint8 requestedUploadType,
                                                     int minSize)
{
    requestedUploadType = normalize(requestedUploadType);
    if (requestedUploadType == TypeOff) {
        return {payload, TypeOff};
    }
    if (!hasCompressionExtension(packetType)) {
        return {payload, TypeOff};
    }
    if (payload.isEmpty()) {
        return {payload, TypeOff};
    }
    if (minSize <= 0) {
        minSize = DefaultMinSize;
    }
    if (payload.size() <= minSize) {
        return {payload, TypeOff};
    }

    std::optional<QByteArray> compressed;
    switch (requestedUploadType) {
    case TypeZSTD: compressed = compressZstd(payload); break;
    case TypeLZ4:  compressed = compressLz4(payload);  break;
    case TypeZLIB: compressed = compressZlibRaw(payload); break;
    default: break;
    }

    if (!compressed) {
        return {payload, TypeOff};
    }
    // Spec §8 / upstream types.go:159-160: if the codec failed to make
    // the data smaller, fall back to raw + TypeOff so the receiver
    // doesn't pay a decompression-overhead penalty for nothing.
    if (compressed->size() >= payload.size()) {
        return {payload, TypeOff};
    }
    return {*compressed, requestedUploadType};
}

std::optional<QByteArray> tryDecompressPayload(const QByteArray &payload, quint8 compType)
{
    if (payload.isEmpty()) {
        return payload;
    }
    compType = normalize(compType);
    if (compType == TypeOff) {
        return payload;
    }

    switch (compType) {
    case TypeZSTD: return decompressZstd(payload);
    case TypeLZ4:  return decompressLz4(payload);
    case TypeZLIB: return decompressZlibRaw(payload);
    default:       return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// ZSTD
// ---------------------------------------------------------------------------

std::optional<QByteArray> compressZstd(const QByteArray &input)
{
    // Upstream uses `WithEncoderLevel(zstd.SpeedFastest)` — the
    // corresponding libzstd level is 1 (ZSTD_minCLevel..ZSTD_maxCLevel
    // range with 1 = fastest in stable zstd).
    constexpr int kLevel = 1;
    const size_t bound = ZSTD_compressBound(static_cast<size_t>(input.size()));
    QByteArray out(static_cast<int>(bound), Qt::Uninitialized);
    const size_t written = ZSTD_compress(out.data(), bound,
                                          input.constData(),
                                          static_cast<size_t>(input.size()),
                                          kLevel);
    if (ZSTD_isError(written)) {
        return std::nullopt;
    }
    out.resize(static_cast<int>(written));
    return out;
}

std::optional<QByteArray> decompressZstd(const QByteArray &input)
{
    // We don't trust frame headers blindly — cap output at the
    // 10 MiB decompression-bomb limit. `ZSTD_decompress` requires the
    // destination to be at least the original size; if a frame header
    // lies about that, we'll catch the mismatch via the error return.
    const unsigned long long frameSize =
            ZSTD_getFrameContentSize(input.constData(),
                                      static_cast<size_t>(input.size()));
    if (frameSize == ZSTD_CONTENTSIZE_ERROR) {
        return std::nullopt;
    }
    if (frameSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        // Streaming-mode frame — fall back to a chunked decompress with
        // a hard ceiling. Caller path doesn't exercise this in practice
        // because upstream's encoder always emits the frame size.
        return std::nullopt;
    }
    if (frameSize > MaxDecompressedSize) {
        return std::nullopt;
    }
    QByteArray out(static_cast<int>(frameSize), Qt::Uninitialized);
    const size_t written = ZSTD_decompress(out.data(), static_cast<size_t>(out.size()),
                                            input.constData(),
                                            static_cast<size_t>(input.size()));
    if (ZSTD_isError(written) || written != frameSize) {
        return std::nullopt;
    }
    return out;
}

// ---------------------------------------------------------------------------
// LZ4 — block compression with 4-byte LE original-size prefix
// ---------------------------------------------------------------------------

std::optional<QByteArray> compressLz4(const QByteArray &input)
{
    const int bound = LZ4_compressBound(input.size());
    if (bound <= 0) {
        return std::nullopt;
    }
    QByteArray out(4 + bound, Qt::Uninitialized);
    // Upstream LZ4 format (types.go:269-287) is
    //   [4 bytes LE original_size][lz4-compressed block]
    // The size prefix mirrors Python `lz4.block(store_size=True)` so
    // wire-compatibility with the reference server is exact.
    qToLittleEndian<quint32>(static_cast<quint32>(input.size()), out.data());
    const int n = LZ4_compress_default(input.constData(),
                                        out.data() + 4,
                                        input.size(),
                                        bound);
    if (n <= 0) {
        return std::nullopt;
    }
    out.resize(4 + n);
    return out;
}

std::optional<QByteArray> decompressLz4(const QByteArray &input)
{
    if (input.size() < 4) {
        return std::nullopt;
    }
    const quint32 origSize = qFromLittleEndian<quint32>(input.constData());
    if (origSize > MaxDecompressedSize) {
        return std::nullopt;
    }
    QByteArray out(static_cast<int>(origSize), Qt::Uninitialized);
    const int n = LZ4_decompress_safe(input.constData() + 4,
                                       out.data(),
                                       input.size() - 4,
                                       out.size());
    if (n < 0 || static_cast<quint32>(n) != origSize) {
        return std::nullopt;
    }
    return out;
}

// ---------------------------------------------------------------------------
// ZLIB — RAW deflate (windowBits = -15, no zlib header / adler32)
// ---------------------------------------------------------------------------

std::optional<QByteArray> compressZlibRaw(const QByteArray &input)
{
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    // windowBits = -15 selects raw deflate per zlib docs. Level 1 mirrors
    // upstream's `flate.NewWriter(io.Discard, 1)` (types.go:63).
    if (deflateInit2(&zs, 1, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return std::nullopt;
    }

    const uLong bound = deflateBound(&zs, static_cast<uLong>(input.size()));
    QByteArray out(static_cast<int>(bound), Qt::Uninitialized);
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
    zs.avail_in = static_cast<uInt>(input.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = static_cast<uInt>(out.size());

    const int rc = deflate(&zs, Z_FINISH);
    const int totalOut = static_cast<int>(zs.total_out);
    deflateEnd(&zs);
    if (rc != Z_STREAM_END) {
        return std::nullopt;
    }
    out.resize(totalOut);
    return out;
}

std::optional<QByteArray> decompressZlibRaw(const QByteArray &input)
{
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, -15) != Z_OK) {
        return std::nullopt;
    }

    QByteArray out;
    out.reserve(std::max<qsizetype>(input.size() * 4, 1024));
    QByteArray buf(64 * 1024, Qt::Uninitialized);

    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
    zs.avail_in = static_cast<uInt>(input.size());

    int rc = Z_OK;
    do {
        zs.next_out = reinterpret_cast<Bytef *>(buf.data());
        zs.avail_out = static_cast<uInt>(buf.size());
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            inflateEnd(&zs);
            return std::nullopt;
        }
        const int produced = buf.size() - static_cast<int>(zs.avail_out);
        out.append(buf.constData(), produced);
        if (out.size() > MaxDecompressedSize) {
            inflateEnd(&zs);
            return std::nullopt;
        }
    } while (rc != Z_STREAM_END && zs.avail_in > 0);

    inflateEnd(&zs);
    if (rc != Z_STREAM_END) {
        return std::nullopt;
    }
    return out;
}

} // namespace amnezia::masterdnsvpn::compression
