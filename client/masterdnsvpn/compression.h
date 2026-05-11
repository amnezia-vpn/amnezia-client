// SPDX-License-Identifier: GPL-3.0-or-later
//
// Spec §8 compression — three codec paths gated by the compression-pair
// byte in SESSION_ACCEPT and carried in the per-packet compression
// extension. Ported from upstream's `internal/compression/types.go`.
//
// Wire-level mapping (matches upstream constants exactly — do not renumber):
//
//   * TypeOff  (0) — payload is plain bytes
//   * TypeZSTD (1) — payload is a Zstandard frame
//   * TypeLZ4  (2) — `[4B LE original_size][LZ4 block]` (Python lz4.block
//                    `store_size=True` layout; upstream's `compressLZ4`
//                    prepends this header at types.go:269-287)
//   * TypeZLIB (3) — RAW deflate stream (windowBits = -15). The name is
//                    "ZLIB" by upstream convention but there is NO zlib
//                    wrapper or adler32 — just deflate-compressed bytes.
//
// Only packet types with the `compression` extension (§3.4 — i.e. the
// `kSNFC` group: STREAM_DATA, STREAM_RESEND, DNS_QUERY_REQ, DNS_QUERY_RES,
// MTU_UP_REQ, MTU_DOWN_RES, PACKED_CONTROL_BLOCKS) can carry compressed
// payloads. `prepareOutgoingPayload()` enforces this and gracefully falls
// back to TypeOff when the codec is unavailable, the payload is too small
// to benefit, or the compressed result is no smaller than the input.
//
// Decompression is similarly defensive: oversized output (over 10 MiB)
// is rejected as a decompression-bomb guard, matching upstream's
// `maxDecompressedSize = 10 * 1024 * 1024` cap.

#ifndef MASTERDNSVPN_COMPRESSION_H
#define MASTERDNSVPN_COMPRESSION_H

#include "wireframing.h"

#include <QByteArray>
#include <QtGlobal>
#include <optional>

namespace amnezia::masterdnsvpn::compression {

// Wire-stable codec identifiers (do not renumber — these are the bytes
// that travel in the per-packet compression extension).
constexpr quint8 TypeOff  = 0;
constexpr quint8 TypeZSTD = 1;
constexpr quint8 TypeLZ4  = 2;
constexpr quint8 TypeZLIB = 3;

// Upstream `internal/compression/types.go:21`.
constexpr int    DefaultMinSize       = 100;
constexpr int    MaxDecompressedSize  = 10 * 1024 * 1024;

// `compType` outside [Off..Zlib] becomes Off, matching upstream
// `NormalizeAvailableType`. Used to defend against malformed received
// compression bytes.
quint8 normalize(quint8 compType);

// Pack/split the (upload, download) pair carried in SESSION_INIT byte 1
// and the SESSION_ACCEPT compression byte. Layout is `upload<<4 | download`,
// where each nibble is one of the codec ids above.
quint8 packPair(quint8 upload, quint8 download);
std::pair<quint8, quint8> splitPair(quint8 packed);

// Mirrors upstream `PreparePayload` (internal/vpnproto/payload.go:19-31).
// Returns (payload, used_codec) — falls back to (input, TypeOff) when:
//   * `packetType` is not in the compression-extension group, or
//   * payload is empty, or
//   * payload size <= minSize (use 0 to mean DefaultMinSize), or
//   * the codec is unavailable / not built in, or
//   * the compressed output is not smaller than the input.
// The caller writes the returned codec id into `packet.compression`.
std::pair<QByteArray, quint8> prepareOutgoingPayload(PacketType packetType,
                                                     const QByteArray &payload,
                                                     quint8 requestedUploadType,
                                                     int minSize);

// Mirrors upstream `TryDecompressPayload` (types.go:166-192). Returns
// std::nullopt on any error (corrupt stream, oversized decompressed
// output, codec-init failure). TypeOff is a pass-through.
std::optional<QByteArray> tryDecompressPayload(const QByteArray &payload, quint8 compType);

// Roundtrip helpers — exposed for tests; production callers prefer the
// higher-level prepare/decompress above.
std::optional<QByteArray> compressZstd(const QByteArray &input);
std::optional<QByteArray> decompressZstd(const QByteArray &input);
std::optional<QByteArray> compressLz4(const QByteArray &input);
std::optional<QByteArray> decompressLz4(const QByteArray &input);
std::optional<QByteArray> compressZlibRaw(const QByteArray &input);
std::optional<QByteArray> decompressZlibRaw(const QByteArray &input);

} // namespace amnezia::masterdnsvpn::compression

#endif // MASTERDNSVPN_COMPRESSION_H
