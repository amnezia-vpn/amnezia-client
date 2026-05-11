# MasterDnsVPN Wire Protocol Specification

This document is a clean-room, language-neutral description of the wire protocol used by the open-source Go project `masterking32/MasterDnsVPN`. It is written for re-implementation in C++ (or any other language) without re-reading the Go source. Where a value or layout is fixed, the exact byte/bit/byte-order is given. Where the upstream behaviour is parameterised, the constants the implementer must obey are noted. Open questions and ambiguities are flagged in the final section.

The protocol has three logical layers, processed in this order on transmit and the reverse order on receive:

1. **DNS layer** — RFC 1035 question/response framing, with the tunnel payload riding inside a question label sub-domain (client → server) or inside one or more `TXT` resource record `RDATA` blobs (server → client).
2. **Encryption + base codec** — the tunnel "frame" is symmetrically encrypted (or pass-through), then base32/base36-encoded into DNS-label-safe ASCII.
3. **Inner VPN packet** — a compact, variable-length header with a 1-byte session ID, a 1-byte packet type, optional stream/sequence/fragment/compression extensions, a 1-byte session cookie, and a 1-byte integrity check.

A reliable transport (ARQ) and a session/stream multiplexer ride on top of layer 3. Local traffic (TCP/SOCKS5/DNS) is carried inside ARQ-managed streams.

---

## 1. Overview

### Roles

* **Client** — runs locally on a user's machine. It exposes one of: a SOCKS5 proxy, a raw TCP forwarder, or a local DNS server. It treats a configurable list of recursive DNS resolvers ("the resolver pool") as transports. For each user request it builds DNS queries whose QNAME encodes the tunnel payload, fans them out across resolvers (with optional duplication for loss resilience), and reassembles the answers.
* **Server** — speaks like an authoritative DNS server for one or more configured DOMAINs (e.g. `v.example.com`). Recursive resolvers forward client queries to it. The server decodes the QNAME, executes the inner stream/SOCKS5/DNS work, and returns the response inside `TXT` answer records on the same DNS transaction.
* **Recursive resolver** (intermediary) — generic DNS infrastructure. The protocol is designed so that no resolver needs to be cooperative; the resolver just sees a long QNAME and a `TXT` answer.

### Tunnel "envelope" (one client request, one server response)

For every byte the client wants to send, it emits *one* DNS query of `QTYPE = TXT`, `QCLASS = IN`, where the QNAME has the form

```text
<encoded-frame>.<DOMAIN>
```

The `<encoded-frame>` portion is the lower-base36 (default) or lower-base32 encoding of the (optionally encrypted) inner VPN packet. It is split into 63-byte labels because RFC 1035 limits each label to 63 bytes and a full QNAME to 253 bytes.

The server replies with `RCODE = NOERROR`, the question echoed back, and one or more `TXT` answer records. Each `TXT` RDATA is one or more length-prefixed strings (each ≤ 255 bytes). Multi-RR responses are tagged with a 2-byte chunking header so the client can reassemble them before decryption.

### Session model

A session is a 1-byte ID + 1-byte cookie pair, established once via a 2-message handshake (`SESSION_INIT` / `SESSION_ACCEPT`). Within a session, the client multiplexes:

* **Stream 0** — a permanently-open meta-stream for pings, packed control blocks, DNS-tunnel queries, and other session-wide control.
* **Streams 1..65535** — opened on demand for SOCKS5, raw TCP, or other per-flow traffic.

Each stream has its own ARQ instance with independent send/receive sequence numbers (uint16, wrap-around).

### Out-of-band channels

* **MTU probes** carry their own packet types and use a fake `SessionID = 0xFF` and `SessionCookie = 0xFF`; they are independent of any session and are answered immediately by the server.
* **Pings/pongs** are stream-0 packets tagged with type `PING` / `PONG`, used to keep NAT bindings alive and to detect liveness.
* **PACKED_CONTROL_BLOCKS** is a piggyback packet type that bundles many small ACK-only or status control packets into one fragment, drastically reducing overhead.

---

## 2. DNS-layer framing

### 2.1 Client → server query (data and control)

* **DNS header** (12 bytes, big-endian throughout):

  | Field    | Width | Value                                                  |
  |----------|-------|--------------------------------------------------------|
  | ID       | 2 B   | Random 16-bit, monotonic counter seeded from `getrandom()`. The server echoes this verbatim. |
  | Flags    | 2 B   | `0x0100` — QR=0, OpCode=0 (QUERY), AA=0, TC=0, RD=1, RA=0, Z=0, RCODE=0. |
  | QDCount  | 2 B   | `0x0001`                                               |
  | ANCount  | 2 B   | `0x0000`                                               |
  | NSCount  | 2 B   | `0x0000`                                               |
  | ARCount  | 2 B   | `0x0001` if EDNS(0) OPT is included, else `0x0000`. The reference client always sends OPT with `udp_size = 4096`. |

* **Question section**:

  * **QNAME** — `<encoded-frame>.<DOMAIN>`, RFC-1035 length-prefixed labels, terminated by a single `0x00`. The encoded frame is split into runs of up to 63 bytes per label. Total wire length must be ≤ 253 bytes for the entire QNAME (header `0x00` excluded). The reference encoder validates these limits explicitly.
  * **QTYPE** — `0x0010` (TXT) for tunnel traffic. (The decoder accepts other tunnel-supported types: `A`, `AAAA`, `CNAME`, `MX`, `NS`, `PTR`, `SRV`, `SVCB`, `CAA`, `NAPTR`, `SOA`, `HTTPS`, `TLSA`. The active client only emits TXT for tunnel envelopes.)
  * **QCLASS** — `0x0001` (IN). Other classes are rejected.

* **Additional section** (optional EDNS(0) OPT pseudo-RR, 11 bytes):

  ```
  00                       ; root name
  00 29                    ; TYPE = OPT (41)
  10 00                    ; UDP payload size = 4096
  00 00 00 00              ; extended RCODE + version + flags
  00 00                    ; RDLEN = 0
  ```

Truncation: the server **never** sets `TC=1`. If the answer is too big for one TXT record, it splits across multiple `TXT` answer RRs (see §2.2). The server also never relies on TCP fallback.

### 2.2 Server → client response

* **DNS header** — `ID` = client's ID; `Flags` = QR=1, OpCode echoed, AA=0, TC=0, RD echoed, RA=1, RCODE=0 for normal answers. `AA` and `TC` are explicitly cleared. Response codes used elsewhere: `FORMAT_ERROR`, `SERVER_FAILURE`, `NOT_IMPLEMENTED`, `REFUSED`.
* **Question section** — copied verbatim from the request.
* **Answer section** — one or more `TXT` RRs:

  | Field    | Width | Value                                                  |
  |----------|-------|--------------------------------------------------------|
  | NAME     | var   | Same QNAME as the question. The 2nd, 3rd, … RRs use a name-pointer compression to the first answer's NAME at offset `0xC0\|<offset>`. |
  | TYPE     | 2 B   | `0x0010` (TXT)                                         |
  | CLASS    | 2 B   | `0x0001` (IN)                                          |
  | TTL      | 4 B   | `0x00000000` always                                    |
  | RDLEN    | 2 B   | length of RDATA in bytes                               |
  | RDATA    | RDLEN | one or more `<len:1><bytes:len>` "character-strings"; `len` ≤ 255. If a single chunk exceeds 255 bytes it is split into multiple back-to-back `<len><bytes>` strings inside the same RDATA. |

* If the request had an OPT in the additional section, the response copies that OPT verbatim into the additional section and sets `ARCount = 1`.

Up to **256 TXT answer chunks** per response (chunk index is a single byte). The server fragments large frames into chunks as follows:

* Each chunk is **either** a "raw byte" payload (`baseEncode = false`) of ≤ **255 bytes**, **or** a base64-encoded payload (`baseEncode = true`) where the encoded length is ≤ **191** characters (so the original chunk is ≤ ~143 bytes pre-encoding). Whether base64 is used is requested by the client during `SESSION_INIT` via the response-mode byte (see §6).
* The **first chunk** is a header chunk: byte `0x00` followed by `<total_chunks:1>` (1..255), followed by the inner VPN packet header bytes followed by as many payload bytes as fit.
* **Subsequent chunks** start with `<chunk_index:1>` (1..total-1) followed by raw payload bytes.

The `<len><bytes>` strings of each chunk are concatenated into the TXT RDATA. If any chunk's length-prefixed form would exceed 255 bytes, it is split into multiple `<len><bytes>` segments within the same RDATA (each ≤ 255). When the client reassembles, it concatenates all `<bytes>` segments per RR, then orders RRs by chunk index, decodes (base64 if applicable), and parses the inner VPN packet.

Special case: a single-chunk response (frame ≤ 255 bytes raw, or ≤ 191 chars base64) is **not** prefixed with `0x00 <total>` — it is just the raw VPN frame (or its base64 encoding) wrapped in a TXT RR.

### 2.3 Maximum sizes

* **Query (client → server)**: limited by the QNAME hard-cap of 253 bytes. After base36 encoding, the inner frame budget is ≈ 158 bytes minus the domain length. With base32 it would be ≈ 158 bytes minus the domain length. The exact maximum is the largest payload `N` such that `encoded_len(N) + label_separators + 1 + len(domain) ≤ 253`.
* **Response (server → client)**: `chunk_count × per_chunk_payload`, capped at 256 chunks. Practical limit ≈ 256 × 255 = 65 280 bytes raw, or ≈ 256 × 143 ≈ 36 600 bytes base64. The EDNS UDP-size advertisement is `4096`, but the server happily exceeds it (relying on resolver IP fragmentation or upstream TCP retry). MTU discovery (§9.2) ultimately bounds *useful* response size.

### 2.4 Control vs data DNS queries

There is **no** structural difference. The DNS-layer framing is identical for handshake, MTU probes, pings, data, and ACKs. The packet type lives inside the inner VPN packet (§3). Some packet types are sent without an established session (`SESSION_INIT`, `MTU_UP_REQ`, `MTU_DOWN_REQ`); the client uses a fixed `SessionID = 0` (or `0xFF` for MTU probes) for those.

---

## 3. Inner packet format

The inner VPN packet is the **decrypted** binary frame that lives inside the encoded label run / the TXT RDATA. It has a base header, a set of optional extensions enabled by the packet type, an integrity footer, and a payload.

### 3.1 Layout

All multi-byte fields are **big-endian**, no alignment padding.

```
+--------+--------+ ... +--------+--------+ payload ...
| SessID | PType  | optional extensions   | Cookie  | Check  |
+--------+--------+ ... +--------+--------+
   1B       1B    (variable)                  1B       1B
```

**Base header** (always present, 2 bytes):

| Offset | Field        | Width | Notes                                       |
|--------|--------------|-------|---------------------------------------------|
| 0      | Session ID   | 1 B   | 0 for SESSION_INIT, 0xFF for MTU probes, otherwise the value the server returned in SESSION_ACCEPT. |
| 1      | Packet Type  | 1 B   | One of the values in §3.4.                  |

**Extensions** (in this fixed order, presence determined by packet type):

| Order | Extension     | Width | Description |
|-------|---------------|-------|-------------|
| 1     | Stream ID     | 2 B   | Big-endian uint16. Stream 0 is the meta-stream. |
| 2     | Sequence Num  | 2 B   | Big-endian uint16, per-stream, wraps at 65 535. Used by ARQ. |
| 3     | Fragment ID   | 1 B   | 0-based fragment index. |
| 3     | Total Frags   | 1 B   | Total number of fragments for this logical message (1 means "no fragmentation"). |
| 4     | Compression   | 1 B   | Compression type byte (see §8). |

The Fragment extension is two bytes packed together (FragmentID then TotalFragments). The Compression extension is one byte.

**Integrity footer** (always present, 2 bytes):

| Offset (from base) | Field          | Width | Notes |
|--------------------|----------------|-------|-------|
| -2                 | Session Cookie | 1 B   | The cookie returned in SESSION_ACCEPT. 0 for pre-session packets. |
| -1                 | Header Check   | 1 B   | A "rolling" check byte over **all** preceding bytes (`SessID`, `PType`, all enabled extensions, `Cookie`). Algorithm: see §3.2. |

**Payload** — everything after the header check byte. May be compressed if the Compression extension is present and non-zero (§8).

### 3.2 Header check algorithm

The check is a single byte computed over the bytes from offset 0 up to and including the `Cookie` byte (i.e. the last non-check byte). Pseudocode (matching the Go reference):

```
acc = (header_len * 17 + 0x5D) mod 256
for idx, value in enumerate(header_bytes):
    acc = (acc + value + idx) mod 256
    acc = acc XOR ((value << (idx & 0x03)) mod 256)
return acc
```

`header_len` is the length of `header_bytes` (i.e. excluding the check byte itself). The shift amount cycles through 0, 1, 2, 3 across successive bytes. Verification compares the computed value against the trailing byte and rejects on mismatch.

### 3.3 Header length per packet type

Each packet type has a fixed set of "enabled" extensions. The header length is `2 (base) + sum(enabled extension widths) + 2 (footer)`. Typical sizes:

* No extensions: 4 bytes.
* Stream + Sequence: 8 bytes (4 + 4).
* Stream + Sequence + Fragment: 10 bytes.
* Stream + Sequence + Fragment + Compression: 11 bytes.

The maximum header is 11 bytes (PACKET_STREAM_DATA, PACKET_STREAM_RESEND, PACKET_DNS_QUERY_REQ, PACKET_DNS_QUERY_RES, PACKET_MTU_UP_REQ, PACKET_MTU_DOWN_RES). The packet types and their extensions are listed in §3.4.

### 3.4 Packet type catalogue

The packet type is the second byte of the base header. Numeric values are exact (matched to the Go enum). Extensions are abbreviated: **S**=stream, **N**=sequence, **F**=fragment, **C**=compression.

| Hex   | Dec | Name                                | Extensions   | Notes |
|-------|-----|-------------------------------------|--------------|-------|
| 0x01  | 1   | PACKET_MTU_UP_REQ                   | S, N, F, C   | Upload MTU probe; client → server. Payload contains a request-mode byte + 4-byte challenge code + filler. |
| 0x02  | 2   | PACKET_MTU_UP_RES                   | (none)       | Server's reply to upload probe; payload echoes challenge + 2-byte received-size. |
| 0x03  | 3   | PACKET_MTU_DOWN_REQ                 | (none)       | Download MTU probe request; payload = mode + challenge + 2-byte requested response size. |
| 0x04  | 4   | PACKET_MTU_DOWN_RES                 | S, N, F, C   | Server's reply with payload padded to the requested size. |
| 0x05  | 5   | PACKET_SESSION_INIT                 | (none)       | Client's session bootstrap. |
| 0x06  | 6   | PACKET_SESSION_ACCEPT               | (none)       | Server's session-accept response. |
| 0x07  | 7   | PACKET_PING                         | (none)       | Stream-0 keepalive (carries 7-byte payload `'P','O',':',<4 random>`). |
| 0x08  | 8   | PACKET_PONG                         | (none)       | Reply to PING. |
| 0x09  | 9   | PACKET_STREAM_SYN                   | S, N         | Open a new stream (raw TCP mode). |
| 0x0A  | 10  | PACKET_STREAM_SYN_ACK               | S, N         | ACK a stream open. |
| 0x0B  | 11  | PACKET_STREAM_CONNECTED             | S, N         | Server informs client the upstream TCP connect succeeded. |
| 0x0C  | 12  | PACKET_STREAM_CONNECTED_ACK         | S, N         | ACK for the above. |
| 0x0D  | 13  | PACKET_STREAM_CONNECT_FAIL          | S, N         | Upstream connect failed. |
| 0x0E  | 14  | PACKET_STREAM_CONNECT_FAIL_ACK      | S, N         |   |
| 0x0F  | 15  | PACKET_STREAM_DATA                  | S, N, F, C   | The bread-and-butter TCP-payload carrier. |
| 0x10  | 16  | PACKET_STREAM_DATA_ACK              | S, N         | Per-segment cumulative ACK (sequence-number specific). |
| 0x11  | 17  | PACKET_STREAM_DATA_NACK             | S, N         | Selective NACK: "I am missing sequence X". |
| 0x12  | 18  | PACKET_STREAM_RESEND                | S, N, F, C   | Same payload as STREAM_DATA, but tagged so the receiver knows it's a retransmit (allows different prioritisation). |
| 0x13  | 19  | PACKET_PACKED_CONTROL_BLOCKS        | C            | Piggyback container: payload is N×7-byte control blocks (see §4). |
| 0x14  | 20  | PACKET_STREAM_CLOSE_WRITE           | S, N         | Half-close: the sender will send no more data. |
| 0x15  | 21  | PACKET_STREAM_CLOSE_WRITE_ACK       | S, N         |   |
| 0x16  | 22  | PACKET_STREAM_CLOSE_READ            | S, N         | Half-close: the sender will read no more data. |
| 0x17  | 23  | PACKET_STREAM_CLOSE_READ_ACK        | S, N         |   |
| 0x18  | 24  | PACKET_STREAM_RST                   | S, N         | Hard reset of stream. |
| 0x19  | 25  | PACKET_STREAM_RST_ACK               | S, N         |   |
| 0x1A  | 26  | PACKET_SOCKS5_SYN                   | S, N, F      | Open a stream + carry the SOCKS5 target address (see §10.2). |
| 0x1B  | 27  | PACKET_SOCKS5_SYN_ACK               | S, N         |   |
| 0x1C..0x2F | 28..47 | PACKET_SOCKS5_*                | S, N         | One pair per SOCKS5 reply code (CONNECT_FAIL, RULESET_DENIED, NETWORK_UNREACHABLE, HOST_UNREACHABLE, CONNECTION_REFUSED, TTL_EXPIRED, COMMAND_UNSUPPORTED, ADDRESS_TYPE_UNSUPPORTED, AUTH_FAILED, UPSTREAM_UNAVAILABLE, plus their `_ACK` siblings). |
| 0x30  | 48  | PACKET_SOCKS5_CONNECTED             | S, N         | SOCKS5 success notification. |
| 0x31  | 49  | PACKET_SOCKS5_CONNECTED_ACK         | S, N         |   |
| 0x32  | 50  | PACKET_DNS_QUERY_REQ                | S, N, F, C   | Local DNS service: client tunnels a raw DNS query to the server. |
| 0x33  | 51  | PACKET_DNS_QUERY_RES                | S, N, F, C   | Server's tunnelled DNS reply. |
| 0x34  | 52  | PACKET_DNS_QUERY_REQ_ACK            | S, N         |   |
| 0x35  | 53  | PACKET_DNS_QUERY_RES_ACK            | S, N         |   |
| 0x36  | 54  | PACKET_SESSION_CLOSE                | (none)       | Client (typically) terminates the session. |
| 0x37  | 55  | PACKET_SESSION_BUSY                 | (none)       | Server: "session table full, retry later". Payload = 4-byte verify code echoed back. |
| 0xFF  | 255 | PACKET_ERROR_DROP                   | (none)       | Generic invalid-cookie / unknown-session marker. Payload = 8 bytes (`'I','N','V'` + 5 nonce bytes). |

Any other byte value rejects the packet at parse time.

### 3.5 Endianness, sequence semantics

* All multi-byte integers in the inner header are **big-endian**.
* Sequence numbers are **per-stream**, **per-direction**. Wrap is full uint16 (`(a - b) & 0xFFFF`); the receiver uses the lexicographic 2's-complement-style "is-ahead" check (`diff < 32768` means ahead of `rcvNxt`).

---

## 4. Packed control blocks (PACKET_PACKED_CONTROL_BLOCKS)

The piggyback container. Used to amortise DNS-envelope overhead across many small ACKs.

* **Outer wrap**: an inner VPN packet of type `0x13` with **only** the Compression extension enabled. SessionID is the active session, Cookie likewise. StreamID/SeqNum/Frag fields are **not** present in the header; they are inside each block.
* **Payload**: a concatenation of fixed **7-byte** blocks (no terminator). The packet's payload length must be a multiple of 7. Receivers iterate while `offset + 7 <= len(payload)`; trailing partial block is silently ignored.

Each block is laid out as:

| Offset | Field          | Width | Notes |
|--------|----------------|-------|-------|
| 0      | PacketType     | 1 B   | Must be a "packable" control type (see below). |
| 1..2   | StreamID       | 2 B   | Big-endian uint16. |
| 3..4   | Sequence Num   | 2 B   | Big-endian uint16. |
| 5      | FragmentID     | 1 B   |   |
| 6      | TotalFragments | 1 B   |   |

### 4.1 Packable types

A control packet is eligible for packing iff (a) its payload is **empty** (`payloadLen == 0`) and (b) its type is one of:

```
PACKET_STREAM_DATA_ACK             PACKET_STREAM_DATA_NACK
PACKET_STREAM_SYN_ACK              PACKET_STREAM_CLOSE_WRITE_ACK
PACKET_STREAM_CLOSE_READ_ACK       PACKET_STREAM_RST_ACK
PACKET_SOCKS5_SYN_ACK              PACKET_STREAM_CONNECTED
PACKET_STREAM_CONNECTED_ACK        PACKET_STREAM_CONNECT_FAIL
PACKET_STREAM_CONNECT_FAIL_ACK     PACKET_SOCKS5_*  (all reply types
                                    + their _ACK forms, including
                                    PACKET_SOCKS5_CONNECTED and ACK)
PACKET_DNS_QUERY_REQ_ACK           PACKET_DNS_QUERY_RES_ACK
```

### 4.2 Capacity heuristic

The number of blocks per outer packet is bounded by:

```
effective = max(MTU * 30%, 7) / 7      ; never less than 1
max_blocks = clamp(effective, 1, MAX_PACKETS_PER_BATCH)
```

The 30% lower bound is hard-coded; the percent argument the function accepts is clamped to `>= 30`.

When packing, the dispatcher uses round-robin across streams and the orphan queue, popping any packet whose type is packable and whose payload is empty, and de-duplicating by `(streamID, packetType, seqNum, fragID)`.

### 4.3 Compression of packed packets

The PACKED_CONTROL_BLOCKS packet has the Compression extension enabled. The compression byte/payload encoding works exactly as for any other packet (§8). When packing aggregates ≥ 2 blocks, the dispatcher uses the session's negotiated upload compression. If only 1 block was eligible, the dispatcher emits the original single packet instead.

---

## 5. Encryption (DATA_ENCRYPTION_METHOD)

The shared `ENCRYPTION_KEY` is a UTF-8 string (typically a hex-encoded random blob) shared between client and server. The "encryption" wraps the **entire decrypted inner VPN packet** (header + footer + payload). The encrypted blob is then base32/base36-encoded for label safety. The encryption type is configured statically — there is no in-band negotiation of encryption methods.

The 6 supported methods are:

| Method | Name        | Key derivation            | IV/Nonce          | Tag           |
|--------|-------------|---------------------------|-------------------|---------------|
| 0      | NONE        | (key not used)            | none              | none          |
| 1      | XOR         | raw bytes, 32 B fixed buf | none              | none          |
| 2      | ChaCha20    | SHA-256 of raw key (32 B) | 16 B random       | none          |
| 3      | AES-128-GCM | MD5 of raw key (16 B)     | 12 B random       | 16 B (GCM)    |
| 4      | AES-192-GCM | raw key padded to 24 B    | 12 B random       | 16 B (GCM)    |
| 5      | AES-256-GCM | SHA-256 of raw key (32 B) | 12 B random       | 16 B (GCM)    |

The "raw key padded" method (1, 4) right-pads the UTF-8 key bytes with NULs to the required length. If the supplied key is shorter, the missing bytes are zero. If longer, only the first N are used.

### 5.1 Method 0 — NONE

The plaintext frame is base-encoded directly. Decryption is the identity. There is **no** authentication.

### 5.2 Method 1 — XOR

Streaming XOR of the plaintext against the **raw key bytes** (zero-padded to 32 bytes if shorter). The keystream is the key repeated; the byte-position into the key cycles `pos % key_len`. There is no IV — the same key bytes are used every packet. Decryption is identical (XOR is involutive).

### 5.3 Method 2 — ChaCha20 (unauthenticated)

* **Key**: SHA-256 of the raw key (32 bytes).
* **Nonce**: 16 random bytes generated per packet, **prepended** to the ciphertext: the wire form is `nonce(16) || ciphertext(N)`.
* **Internals**: the upstream uses Go's `golang.org/x/crypto/chacha20.NewUnauthenticatedCipher` with a 12-byte nonce (`nonce[4:]`) and the **first 4 bytes** as the initial **block counter** (`SetCounter(little_endian_uint32(nonce[0:4]))`). This is non-standard and must be reproduced exactly.
* No MAC. Tampering is silently accepted.

### 5.4 Methods 3–5 — AES-{128,192,256}-GCM

* **Key derivation**: as in §5.
* **Nonce**: 12 random bytes per packet, **prepended** to the ciphertext: wire form is `nonce(12) || ciphertext(N) || tag(16)`. Standard `crypto/cipher.AEAD.Seal` semantics.
* **AAD**: empty (no associated data).
* **Tag**: 16 bytes, appended by `Seal`. Verified by `Open`.
* Decryption failure returns `ErrInvalidCiphertext` and drops the packet silently.

### 5.5 Wire wrapping

After encryption, the ciphertext (with prepended nonce, with AEAD tag where applicable) is base-encoded:

* **Default (lower-base36)** — see §5.6.
* The codec module also includes a **lower-base32** encoder (RFC 4648 alphabet `abcdefghijklmnopqrstuvwxyz234567`, no padding) which is selectable by editing one line in the codec module. The default is base36.

The encoded ASCII is then split into 63-byte DNS labels.

### 5.6 Lower-base36 encoder

Alphabet (in order): `0123456789abcdefghijklmnopqrstuvwxyz`. It packs **7 input bytes** into **11 base-36 characters**. The 7 bytes are read big-endian into a uint64; the 11 base-36 digits are written most-significant first.

Tail handling for non-multiples-of-7:

| Tail bytes | Encoded chars |
|-----------|----------------|
| 1         | 2              |
| 2         | 4              |
| 3         | 5              |
| 4         | 7              |
| 5         | 8              |
| 6         | 10             |
| 7 (next block) | 11        |

The decoder reverses this. Encoded length 9, 6, 3, or 1 modulo 11 is invalid. The decoder is case-insensitive (uppercase ASCII is normalised).

---

## 6. ARQ reliability layer

ARQ is a per-stream reliable overlay that sits above the inner-packet layer. It provides cumulative + selective NACK acknowledgements, adaptive RTO, retransmission, half-close handshakes, hard reset, and inactivity timeouts.

### 6.1 Sequence number space

* Width: **uint16**.
* Per-stream, per-direction. Send sequences (`sndNxt`) and receive sequences (`rcvNxt`) are independent.
* Wrap: full 16-bit. "Ahead-of-rcvNxt" check uses the modular distance `diff = (sn - rcvNxt) & 0xFFFF; if (diff >= 32768) treat as in-the-past`.
* Initial value on a new stream is implementation-defined (the reference uses 0 and increments before send / on receive).

### 6.2 Send window

* Configurable via `ARQ_WINDOW_SIZE`. Floor is **300**; the receive window is set to `2 × window_size`.
* Backpressure trigger: if `len(sndBuf) >= max(window * 0.8, 50)`, the writer blocks until ACKs free slots (timer-based 200 ms re-poll).
* No window negotiation on the wire — server policy in `SESSION_ACCEPT` (`MaxARQWindowSize`, see §7) caps what the client requests.

### 6.3 Acknowledgement strategy

* **Cumulative-style ACK**: every received `STREAM_DATA` (or `STREAM_RESEND`) triggers a `STREAM_DATA_ACK` carrying that **specific** sequence number. The receiver does not currently send range-cumulative ACKs.
* **Selective NACK**: when a gap is detected (i.e. `sn > rcvNxt + 1`), the receiver may emit `STREAM_DATA_NACK` packets for missing sequence numbers, throttled by `ARQ_DATA_NACK_REPEAT_SECONDS` and bounded by `ARQ_DATA_NACK_MAX_GAP`.

  The probe pattern depends on whether `(sn - rcvNxt) <= dataNackMaxGap`:
  * Within the gap window: NACK every missing sequence number from `rcvNxt` up to `sn - 1`.
  * Outside the window: NACK the first ~5% of missing seqs from `rcvNxt`, plus the single missing seq nearest the current-window frontier (`rcvNxt + dataNackMaxGap - 1`). This bounds bandwidth on bursty losses.
* The first NACK for a given sequence is delayed by `ARQ_DATA_NACK_INITIAL_DELAY_SECONDS`. Subsequent NACKs for the same seq must be at least `ARQ_DATA_NACK_REPEAT_SECONDS` apart.

ACKs and NACKs are payload-empty stream packets eligible for packing into PACKED_CONTROL_BLOCKS (§4).

### 6.4 Retransmission timing (RTO)

There are **two independent adaptive RTO state machines** per stream: one for data, one for control.

* **Initial RTO**: `ARQ_INITIAL_RTO_SECONDS` (data) and `ARQ_CONTROL_INITIAL_RTO_SECONDS` (control). Both clamped to `[0.05s, MaxRTO]`.
* **Max RTO**: `ARQ_MAX_RTO_SECONDS` (data) and `ARQ_CONTROL_MAX_RTO_SECONDS` (control). Floor 0.05s.
* **Sample on success**: when an ACK arrives for a packet that was *not* a retransmit (and was actually dispatched, i.e. `sampleEligible == true`), the RTT sample updates the SRTT/RTTVAR pair using TCP-style EWMA:
  ```
  delta   = |srtt - sample|
  rttvar  = (3*rttvar + delta) / 4
  srtt    = (7*srtt + sample) / 8
  base    = clamp(srtt + 4*rttvar, initialRTO, maxRTO)
  ```
* **Backoff on retransmit**: when a packet times out (`now - lastSent >= currentRTO`), it is requeued, `Retries++`, `SampleEligible := false`, and `currentRTO := clamp(currentRTO * GROWTH_FACTOR, base, maxRTO)`.
  * Data retransmit growth factor: **1.35**.
  * Control retransmit growth factor: **1.25**.
  * Setup-control growth factor (PACKET_STREAM_SYN, PACKET_SOCKS5_SYN): **1.15**.
* **Drain RTO cap**: when the stream is in a draining/deferred-close state, `effectiveRTO` is capped at `clamp(2s, initialRTO, maxRTO)` to accelerate teardown.

### 6.5 Retransmit selection

When multiple packets are due for retransmission, the implementation prioritises a small "front budget" of the **oldest** sequence numbers (i.e. those farthest behind `sndNxt`). These are emitted as `STREAM_RESEND` (so the receiver can treat them with retry priority); the rest are re-emitted as `STREAM_DATA`. The budget is `min(max(window/10, 1), 64, len(jobs))`.

### 6.6 Stream lifecycle states

States map directly to the on-the-wire packet exchange:

| State              | Trigger                            |
|--------------------|------------------------------------|
| OPEN               | Created                            |
| HALF_CLOSED_LOCAL  | Sent CLOSE_READ                    |
| HALF_CLOSED_REMOTE | Received CLOSE_READ                |
| CLOSING            | Both sides have CLOSE_READ pending |
| DRAINING           | Awaiting drain of sndBuf before terminal packet |
| TIME_WAIT          | Post-FIN reserve                   |
| RESET              | RST sent or received               |
| CLOSED             | Terminal                           |

Half-close uses two pairs:
* `STREAM_CLOSE_READ` / `_ACK` — "I will read no more data".
* `STREAM_CLOSE_WRITE` / `_ACK` — "I will send no more data".

`STREAM_RST` is hard reset; `STREAM_RST_ACK` confirms.

Terminal packets are sent only after `sndBuf` is empty (or after `TerminalDrainTimeout`, default 60s). Final-ACK watchdog: `TerminalAckWaitTimeout` (default 30s).

### 6.7 Out-of-order packets

Held in `rcvBuf` (map keyed by sequence). When the contiguous prefix at `rcvNxt` is non-empty, the writer loop drains them in order. The receive window is `2 × send_window`. Packets whose sequence is **behind** `rcvNxt` (i.e. duplicate) trigger an immediate ACK but are otherwise dropped.

### 6.8 Inactivity / max-retries

* `ARQ_INACTIVITY_TIMEOUT_SECONDS` (floor 120s) — terminates the stream if no activity within that window.
* `ARQ_MAX_DATA_RETRIES` (floor 60) — terminates with RST after that many retransmissions of a single sequence.
* `ARQ_MAX_CONTROL_RETRIES` (floor 5) — same, for control packets.
* `ARQ_DATA_PACKET_TTL_SECONDS` — drop the packet if it has lived in sndBuf longer than this.

---

## 7. Session handshake

A session is identified by a **(SessionID:1B, SessionCookie:1B)** pair. Both fields use the inner-packet header slots; only `SESSION_INIT` is sent with `SessionID = 0`.

### 7.1 SESSION_INIT (client → server)

Inner packet: `PacketType = 0x05`, no extensions, payload = 10 bytes:

| Offset | Width | Field                    |
|--------|-------|--------------------------|
| 0      | 1 B   | Response mode: `0` = raw TXT chunks; `1` = base64-encoded TXT chunks. |
| 1      | 1 B   | Compression pair (upload<<4 | download), values 0..3 per nibble. |
| 2..3   | 2 B   | Client's max upload MTU (uint16, BE) — the MTU it discovered for client → server. |
| 4..5   | 2 B   | Client's max download MTU (uint16, BE). |
| 6..9   | 4 B   | Verify code: 4 cryptographically-random bytes. |

The client picks the response mode based on its `BASE_ENCODE_DATA` config (default false). The server echoes the verify code in `SESSION_ACCEPT` (and in `SESSION_BUSY`); the client refuses any reply whose verify code does not match exactly.

The client sends 3 parallel duplicates of `SESSION_INIT` to a single chosen resolver, staggered 100 ms apart, and accepts the first valid `SESSION_ACCEPT`.

### 7.2 SESSION_ACCEPT (server → client)

Inner packet: `PacketType = 0x06`, no extensions, payload = **7** bytes (legacy) or **20** bytes (with policy sync; the reference always sends 20).

| Offset | Width | Field |
|--------|-------|-------|
| 0      | 1 B   | Granted SessionID (1..255; never 0). |
| 1      | 1 B   | Granted SessionCookie. |
| 2      | 1 B   | Granted compression pair (upload<<4 | download) — server may downgrade either nibble. |
| 3..6   | 4 B   | Verify code (echoes the client's). |
| 7      | 1 B   | High nibble = MaxSetupDuplicationCount, low nibble = MaxPacketDuplicationCount (clamped 0..15 each). |
| 8      | 1 B   | Server's MaxUploadMTU cap (uint8). |
| 9..10  | 2 B   | Server's MaxDownloadMTU cap (uint16, BE). |
| 11     | 1 B   | Server's MaxRxTxWorkers cap (uint8). |
| 12     | 1 B   | Server's MinPingAggressiveInterval, scaled byte (see §7.4). |
| 13     | 1 B   | Server's MaxPacketsPerBatch cap (uint8). |
| 14..15 | 2 B   | Server's MaxARQWindowSize cap (uint16, BE). |
| 16     | 1 B   | Server's MaxARQDataNackMaxGap cap (uint8). |
| 17..18 | 2 B   | Server's MinCompressionMinSize floor (uint16, BE). |
| 19     | 1 B   | Server's MinARQInitialRTOSeconds, scaled byte (see §7.4). |

### 7.3 SESSION_BUSY (server → client)

Inner packet: `PacketType = 0x37`, no extensions, payload = **4 bytes** = the verify code echoed back. The client treats this as "back off, try later" and respects `SESSION_INIT_BUSY_RETRY_INTERVAL_SECONDS` before re-attempting.

### 7.4 Scaled-byte fields

Two policy fields use a 1-byte scaled encoding for floating-point seconds in the range `[0.05, 1.00]`:

```
scaled = round( (clamp(value, 0.05, 1.00) - 0.05) / 0.95 * 255 )
value  = 0.05 + (scaled / 255) * 0.95
```

This applies to `MinPingAggressiveInterval` and `MinARQInitialRTOSeconds` in `SESSION_ACCEPT`. Clients clamp their requested values to the server's policy after parsing.

### 7.5 Session lifecycle

* The session is alive until a `PACKET_SESSION_CLOSE` is sent in either direction, or until inactivity / max-retries terminate the underlying stream-0.
* On `SESSION_CLOSE`, the client emits a "burst" of up to 10 close packets in 3 staggered rounds spread over a configurable timeout, fanned out across the best resolvers.
* Re-init is the same dance as initial init; the new session gets a new cookie.

### 7.6 INVALID_COOKIE handling

If a client sends a non-pre-session packet with a wrong cookie or an unknown SessionID, the server replies with `PACKET_ERROR_DROP` (type 0xFF) carrying an 8-byte payload (`'I','N','V'` + 5-byte nonce) using either raw or base64 mode (alternated round-robin).

The server tracks repeat offenders via an `invalid_cookie_tracker`: per `(sessionID, expectedCookie, receivedCookie, sessionState)` key, it counts attempts in a sliding `INVALID_COOKIE_WINDOW_SECONDS` window (default 2.0s). After the threshold (configurable) is reached and at most once per window, it logs and may trigger a session drop. The threshold is `MAX_INVALID_COOKIE_THRESHOLD` (configurable on the server side).

The client interprets `PACKET_ERROR_DROP` as a request to re-init.

---

## 8. Compression

Compression is **per-payload**, signalled by the 1-byte Compression extension in the inner packet header. Three algorithms plus a passthrough are supported:

| Type | Algorithm              | Notes                                       |
|------|------------------------|---------------------------------------------|
| 0    | OFF                    | Payload is plaintext.                       |
| 1    | ZSTD                   | Zstandard, "fastest" preset.                |
| 2    | LZ4                    | LZ4 block format, with a 4-byte little-endian original-size prefix prepended to the LZ4 block (this matches the Python `lz4.block` library's `store_size=True` mode). |
| 3    | ZLIB                   | Raw deflate (no zlib header), as produced by Go's `compress/flate`. |

Other values are normalised to OFF. Decompression caps the output at **10 MiB** as a safety guard against decompression bombs.

### 8.1 Per-packet compression policy

A packet is compressed iff:
* Its packet type has the Compression extension (DATA, RESEND, PACKED_CONTROL_BLOCKS, DNS_QUERY_REQ/RES, MTU_UP_REQ, MTU_DOWN_RES).
* The negotiated direction's compression type is non-zero.
* The payload length **exceeds** `COMPRESSION_MIN_SIZE` (default 100, server-policy-floored).
* The compressed output is strictly **smaller** than the original; otherwise the implementation reverts to OFF (size guard).

### 8.2 Negotiation

The compression types for each direction are exchanged in the `SESSION_INIT` payload byte 1 (high nibble = upload type, low nibble = download type) and confirmed in the `SESSION_ACCEPT` payload byte 2 (server may downgrade). There is **no per-packet renegotiation** — the type is fixed for the session, but each individual payload can independently choose to be compressed (Compression byte non-zero) or not (Compression byte 0).

### 8.3 PackPair / SplitPair encoding

Both client and server pack/unpack a (uploadType, downloadType) pair into one byte: `(upload << 4) | download`. Each nibble is normalised — values > 3 are coerced to OFF.

---

## 9. Resolver pool, packet duplication, and MTU discovery

### 9.1 Resolver pool

The client reads `client_resolvers` (one resolver per line, format `IP:PORT|DOMAIN[:WEIGHT]`). Each entry becomes a `Connection` with: resolver IP, port (default 53), tunnel domain, tracked health stats. Connections are partitioned into "active" (suitable for use) and "inactive" (rejected during MTU testing or auto-disabled).

### 9.2 MTU discovery

For each connection the client probes upload and download MTU in two phases.

**Upload probe (PACKET_MTU_UP_REQ → MTU_UP_RES)**:

* Payload: 1 byte response-mode, 4 bytes monotonic challenge code, then up to (probed_size - 5) random filler.
* The packet uses fake `SessionID = 255`, `SessionCookie = 255`, `StreamID = 1`, `SeqNum = 1`, `FragmentID = 0`, `TotalFragments = 1`.
* The server replies with `MTU_UP_RES` carrying a 6-byte payload: `<challenge:4 BE>` + `<received_size:2 BE>`. The client verifies both the echoed challenge and that `received_size == probed_size`.

**Download probe (PACKET_MTU_DOWN_REQ → MTU_DOWN_RES)**:

* Request payload: 1-byte mode, 4-byte challenge, 2-byte requested response payload size (BE), then filler to fill the upload MTU.
* The server replies with a `MTU_DOWN_RES` packet whose payload starts `<challenge:4 BE>` + `<received_size:2 BE>` and is then padded with filler to exactly the requested length.
* The client verifies challenge match, length match, and that the parsed payload byte count equals the request.

The MTU search uses an exponential-then-binary-search algorithm bounded by configured floors (`MIN_UPLOAD_MTU_FLOOR = 10`, `MIN_DOWNLOAD_MTU_FLOOR = 20`) and ceilings.

### 9.3 Resolver balancing strategies

`RESOLVER_BALANCING_STRATEGY` accepts these integer values:

| Value | Name                         | Algorithm |
|-------|------------------------------|-----------|
| 0     | Default (Round Robin)        | Treated identically to value 2. |
| 1     | Random                       | Uniform random over active set. Independent xorshift64 RNG; no replacement for multi-pick. |
| 2     | Round Robin                  | Atomic counter modulo active-set length; each pick advances the counter. |
| 3     | Least Loss                   | Score = `(lost * 1000) / sent` if `sent ≥ 5`, else 200 (probation). Pick the lowest score. Fall back to round-robin if no active connection has ≥ 5 sends. |
| 4     | Lowest Latency               | Score = average RTT in microseconds (`rttSum / rttCount`) if `count ≥ 5`, else 999000. Pick lowest. Fall back to round-robin without signal. |
| 5     | Hybrid Score                 | Score = `lossScore * 8 + latencyPenalty`, where `latencyPenalty = clamp(latencyMillis, 0, 1000)` (with 200 for unknown). |
| 6     | Loss-Then-Latency            | (a) Pick all candidates whose loss score is within `bestLoss + 25` (or no tolerance if bestLoss ≥ 200). (b) From those, keep candidates within `bestLatency + tolerance(bestLatency)` (tolerance = `latency / 4` clamped to [2, 25] ms when latency < 200 ms; else 0). (c) Random pick from the survivors; failure → fall back to round-robin. |
| 7     | Least-Loss Top-Random        | Sort by loss; take the top `max(2, ⌈N/10⌉)`; random pick. |
| 8     | Least-Loss Top-Round-Robin   | Same shortlist as (7); round-robin within. |

### 9.4 Per-connection health stats

Atomic counters per connection:
* `sent`, `acked`, `lost`, `rttMicrosSum`, `rttCount`.
* "Half-life": when any counter exceeds 1000, all five are halved (CAS loop). Provides exponential decay.
* Sliding "window" counters (`windowStarted`, `windowSent`, `windowLost`) for the auto-disable feature, reset every `AUTO_DISABLE_TIMEOUT_WINDOW_SECONDS` (default 30s).

### 9.5 Auto-disable

A connection is moved from active → inactive when, within the auto-disable window, **all** observations were timeouts (no successful acks). The exact threshold is governed by `AUTO_DISABLE_TIMEOUT_SERVERS` and `AUTO_DISABLE_TIMEOUT_WINDOW_SECONDS`. Auto-recovery: if `RECHECK_INACTIVE_SERVERS_ENABLED`, inactive connections get periodic background MTU re-probes and may be re-promoted.

### 9.6 Packet duplication and dedup

* Each outgoing tunnel packet is sent over **N** resolver connections in parallel, where `N = PACKET_DUPLICATION_COUNT` (default 3, clamped 1..10) for normal packets and `SETUP_PACKET_DUPLICATION_COUNT` (≥ N, ≤ 12) for `STREAM_SYN` / `SOCKS5_SYN`.
* The server is stateless for ingress dedup at the DNS layer — it relies on the inner sequence number + stream ID to deduplicate. Receivers in ARQ ack every received `STREAM_DATA` (so duplicates produce duplicate ACKs, which the sender silently treats as "first wins").
* On receive, ARQ's `rcvBuf` is keyed by sequence; duplicates simply overwrite (no-op effectively).

---

## 10. Stream multiplexing

### 10.1 Stream IDs

* Width: **uint16** (2 bytes in the Stream extension).
* `StreamID = 0` is reserved for the meta-stream (pings, packed control blocks, DNS-query control). Stream 0 is created at session start and never destroyed during a session.
* Stream IDs 1..65535 are allocated for application flows. The reference allocator increments `last_stream_id` and skips collisions with the (small) set of recently-closed-but-still-draining streams (`recentlyClosedStreams`).
* Every stream gets its own ARQ state machine, send/receive sequence space, and TX queue.

### 10.2 SOCKS5 stream setup

To open a SOCKS5 stream, the client sends `PACKET_SOCKS5_SYN` with:

* Stream extension = newly-allocated `streamID`.
* Sequence num = 1 (or the per-stream initial value).
* Fragment extension supported (the SOCKS5 target may be split if it exceeds MTU; in practice it always fits in one fragment).
* Compression supported.
* **Payload** = SOCKS5 target encoded inline:

  | Offset | Width | Field |
  |--------|-------|-------|
  | 0      | 1 B   | Address type: `0x01` (IPv4), `0x03` (Domain), `0x04` (IPv6). |
  | 1..M   | M     | If IPv4: 4 bytes BE address. If IPv6: 16 bytes BE address. If Domain: 1 byte length, then `length` bytes ASCII. |
  | M+1..M+2 | 2 B | Port (BE uint16). |

The server replies with `PACKET_SOCKS5_SYN_ACK` (empty payload, packable), then attempts the upstream connect. On success the server sends `PACKET_SOCKS5_CONNECTED` (empty payload, packable); the client replies with `PACKET_SOCKS5_CONNECTED_ACK` and starts streaming `STREAM_DATA`. On failure, the server sends one of the failure-type packets (`SOCKS5_CONNECT_FAIL`, `SOCKS5_RULESET_DENIED`, `SOCKS5_NETWORK_UNREACHABLE`, `SOCKS5_HOST_UNREACHABLE`, `SOCKS5_CONNECTION_REFUSED`, `SOCKS5_TTL_EXPIRED`, `SOCKS5_COMMAND_UNSUPPORTED`, `SOCKS5_ADDRESS_TYPE_UNSUPPORTED`, `SOCKS5_AUTH_FAILED`, `SOCKS5_UPSTREAM_UNAVAILABLE`); each has a corresponding `_ACK`. Failure packets close the stream.

### 10.3 Raw TCP stream setup

In `PROTOCOL_TYPE = "TCP"` mode, the client uses `PACKET_STREAM_SYN` (no payload — destination is configured in advance on the server side or implied by the listener) plus `_SYN_ACK`, `_CONNECTED`/_ACK, `_CONNECT_FAIL`/_ACK pairs as in SOCKS5.

### 10.4 Stream setup ACK TTL

`STREAM_SETUP_ACK_TTL_SECONDS` (server config, default modest seconds) governs how long the server retains a SYN-ACK to retransmit if the client's first ACK is lost. After the TTL the setup is considered abandoned and resources are released.

### 10.5 Half-close and RST

See §6.6.

---

## 11. Local DNS service

Enabled by `LOCAL_DNS_ENABLED = true` on the client. Exposes a UDP DNS listener on `LOCAL_DNS_IP:LOCAL_DNS_PORT`.

### 11.1 Local cache

A bounded TTL cache keyed on `(name, type, class)`. On a SOCKS5-UDP DNS request:

1. The client lite-parses the DNS query, extracts transaction ID + first question (`name`, `type`, `class`).
2. Looks up `(name, type, class)` in the cache.
3. **Cache hit** — patches the cached response's transaction ID to match the new query and returns it immediately.
4. **Cache pending** (already in flight) — drops the second request silently (the in-flight tunnel response will populate the cache; the next retry hits).
5. **Cache miss** — dispatches to the tunnel and tracks the in-flight query.

Optional persistence: `LOCAL_DNS_CACHE_PERSIST_TO_FILE`, flushed every `LOCAL_DNS_CACHE_FLUSH_INTERVAL_SECONDS`.

### 11.1a Cache-miss response semantics

There are two valid client-side designs for what to do on cache miss:

* **Upstream (Go) `masterking32/MasterDnsVPN`**: dispatch to tunnel, then *close the SOCKS5 UDP association* — forcing the local app to retry. The retry hits the cache (now populated by the tunnel response) and returns instantly.
* **C++ port (this implementation)**: dispatch to tunnel, *track the in-flight query by wire seq*, send the tunnel response back to the SOCKS5 client directly when it arrives. No association teardown.

**Wire-protocol behavior is byte-for-byte identical** between the two — DNS_QUERY_REQ fragmentation, encryption, resolver fan-out, retries, all match. The divergence is purely at the SOCKS5 loopback boundary.

The C++ port deliberately diverges here for OPSEC: upstream's close-on-miss pattern is unique to MasterDnsVPN (no normal DNS resolver — `dnsmasq`, `systemd-resolved`, `unbound` — closes its listening socket on cache miss). A local observer watching loopback can fingerprint the resolver as "MasterDnsVPN-shaped" purely from the socket-churn pattern. The C++ port's behavior matches a standard cache-then-forward DNS resolver and is indistinguishable at the loopback layer.

Both designs share the same cache layer; the divergence only matters on the cache-miss path.

### 11.2 Tunnel dispatch (PACKET_DNS_QUERY_REQ)

The client wraps the **raw DNS query bytes** in one or more `PACKET_DNS_QUERY_REQ` packets (fragmentable, compressible). Stream = 0. Sequence = a unique `mtuProbeCounter`-based uint16 per query (the same atomic counter that's used for MTU probes). FragmentID/TotalFragments split as needed by `syncedUploadMTU`.

The server reassembles fragments via a fragment store keyed by `(sessionID, sequenceNum)` with a `DNS_FRAGMENT_TIMEOUT` deadline, then runs the query against its DNS upstream and sends back `PACKET_DNS_QUERY_RES` (also fragmentable) with the same sequence number.

The server side uses an **inflight manager** to deduplicate concurrent identical queries: a single resolver lookup is shared across all clients hitting the same `(name, type, class)` while the request is pending.

The server uses an internal DNS cache (similar to the client's), keyed by the same tuple.

### 11.3 Per-fragment ACKs

`PACKET_DNS_QUERY_REQ_ACK` and `PACKET_DNS_QUERY_RES_ACK` are control ACKs (Stream + Sequence extensions only, no fragment field) used by the ARQ control reliability layer to confirm receipt of each fragment. They are eligible for packing in PACKED_CONTROL_BLOCKS.

---

## 12. Ping / keepalive

### 12.1 Tiered intervals

The client's ping manager keeps four timestamps per session:
* `lastPingSentAt` — last outbound `PING`.
* `lastPongReceivedAt` — last inbound `PONG`.
* `lastNonPingSentAt` — last outbound packet that wasn't a `PING`.
* `lastNonPongReceivedAt` — last inbound packet that wasn't a `PONG`.

Define `idle = min(now - lastNonPingSent, now - lastNonPongRecv)`.

| Tier        | Trigger                                                | Interval                                                  |
|-------------|--------------------------------------------------------|-----------------------------------------------------------|
| Aggressive  | `idle < PING_WARM_THRESHOLD_SECONDS`                   | `PING_AGGRESSIVE_INTERVAL_SECONDS` (default 0.100 s).     |
| Lazy        | `idle ∈ [WARM, PING_COOL_THRESHOLD)`                   | `PING_LAZY_INTERVAL_SECONDS` (default 0.750 s).           |
| Cooldown    | `idle ∈ [COOL, PING_COLD_THRESHOLD)`                   | `PING_COOLDOWN_INTERVAL_SECONDS` (default 2.0 s).         |
| Cold        | `idle ≥ PING_COLD_THRESHOLD_SECONDS`                   | `PING_COLD_INTERVAL_SECONDS` (default 15.0 s).            |

Defaults: WARM = 8 s, COOL = 20 s, COLD = 30 s.

The loop wakes on a wakeable channel whenever real (non-PING/PONG) traffic moves, or when the timer fires (timer interval = `min(max(currentInterval/2, 100 ms), 1 s)` to keep the loop responsive).

### 12.2 PING/PONG packet format

Both have no extensions, a 7-byte payload:

| Offset | Field         |
|--------|---------------|
| 0..2   | ASCII `'P','O',':'`. |
| 3..6   | 4 random bytes (PRNG nonce). |

The server's `PONG` payload uses `'P','O','N'` followed by a 4-byte rolling-xorshift nonce.

Pings are stream-0 packets; the ping manager pushes them into stream-0's ARQ TX queue via `PushTXPacket`. The `nextPingSeq` counter (uint32 atomic, truncated to uint16 on the wire) is independent of stream-0's data sequence space — collisions are tolerated because PING/PONG don't go through ARQ retransmission (they are idle-priority).

---

## 13. Resolved interop notes

The following points were grounded against the upstream Go reference at the corresponding source locations. Each is normative for the C++ port; the citations are the file paths inside `masterking32/MasterDnsVPN`'s repo.

1. **Initial ARQ sequence numbers — 0.**
   `ARQ.sndNxt` and `ARQ.rcvNxt` are `uint16` and are left at their zero value by the `NewARQ` constructor (`internal/arq/arq.go:332-415` does not touch them; the struct fields at `:135-136` default to 0). The data-plane sender uses `sn := a.sndNxt; a.sndNxt++` (`:1168-1169`), so the first data packet on each stream carries `SequenceNum = 0`. The setup-side `STREAM_SYN` is sent through the control-reliability path with an explicit `sequenceNum = 0` (`internal/client/tcp_stream.go:47-57`). Receivers therefore start with `rcvNxt = 0` and accept the first packet as in-order.

2. **Cookie 0 in pre-session traffic — zero-filled, never omitted.**
   The cookie byte sits in the integrity footer and is always written, regardless of which extensions the packet type carries (`internal/vpnproto/builder.go:69-70`). For `SESSION_INIT`, `MTU_UP_REQ`, `MTU_UP_RES`, `MTU_DOWN_REQ`, `MTU_DOWN_RES` and any pre-handshake traffic, the caller passes `SessionCookie = 0` and the byte is laid down as `0x00`. The header check byte (`computeHeaderCheckByte`) covers the zero, so the C++ side must also zero-fill, not omit.

3. **`PACKET_SESSION_BUSY` carries no StreamID at all.**
   `SESSION_BUSY` appears only in the `validOnly` set in `internal/vpnproto/parser.go:225` — no `streamAndSeq`, no `frag`, no `comp`. The serialiser therefore writes a 4-byte header (SessionID + PacketType + Cookie + check) followed straight by the 4-byte verify-code payload. Earlier spec drafts that referred to "StreamID = 0" in `SESSION_BUSY` were misleading: the field is absent, not zero.

4. **`ARQ_DATA_NACK_INITIAL_DELAY_SECONDS` default is 0.1s on the client, 0.3s on the server.**
   Client default at `internal/config/client.go:213`, server default at `internal/config/server.go:172`. Both are passed through `defaultFloatAtMostZero(..., default)` then `clampFloat(value, 0.01, 60.0)` for the client / `clampFloat(value, 0.01, 30.0)` for the server (`client.go:411`, `server.go:453`). The first NACK is delayed by this amount after gap detection; the C++ engine encodes this as `dataNackInitialDelayMs = 100` in `ArqConfig` and is correct.

5. **Compression is applied per packet, after fragmentation.**
   `BuildRawAuto`/`BuildEncodedAuto` call `PreparePayload` immediately before each packet is serialised (`internal/vpnproto/payload.go:65-81`), so a fragmented `STREAM_DATA` message has each fragment compressed independently. The compression-extension byte sits in the per-packet header, not a logical-message header, which is consistent with this ordering.

6. **TXT chunking reserves more than one byte on chunk 0.**
   Reference at `internal/dnsparser/transport.go:479-553`. `maxChunk = 255` when `baseEncode = false` (`maxTXTAnswerPayload`) and `maxChunk = 191` when `baseEncode = true` (`maxTXTEncodedChunk`, sized so 191 bytes raw fits in 256 bytes encoded after base64). Chunk 0's prefix is **2 bytes** (`0x00` marker + total-chunk count) **plus the full inner VPN header**, so the usable data on chunk 0 is `maxChunk - 2 - headerLen`. Chunks 1+ reserve a single chunk-index byte, so usable data is `maxChunk - 1`. C++ implementations must mirror exactly this when emitting (server-side) or assembling (client-side parser).

7. **The client always emits EDNS(0) OPT with a 4096-byte UDP buffer.**
   `EDnsSafeUDPSize = 4096` (`internal/client/client.go:32`), passed into `BuildTunnelTXTQuestionPacket*` on every outbound query (`internal/client/tunnel_query.go:24`, `internal/client/async_runtime.go:726,741`). The server mirrors the OPT record back in its response (`internal/dnsparser/response.go:85-112`). Clients that omit OPT are not exercised by the reference; the C++ client must always include OPT.

8. **Name compression in multi-RR answers uses the 14-bit pointer `0xC000 | firstAnswerNameOffset`.**
   `internal/dnsparser/transport.go:213-247`: only when more than one TXT RR is emitted, and only if the first answer's name offset fits in 14 bits. The first RR's NAME is written in full; subsequent RRs emit a 2-byte pointer in its place. C++ parsers must follow `0xC0`-prefixed length bytes as pointers (already handled in `client/masterdnsvpn/dnsframing.cpp`'s `skipName`).

9. **Verify code is persistent across re-attempts; no race.**
   The session-init builder caches `sessionInitPayload`, `sessionInitBase64`, and `sessionInitVerify` behind `sessionInitReady` (`internal/client/session.go:380-409`). Subsequent `nextSessionInitAttempt` calls reuse the same verify code until a successful `SESSION_ACCEPT` triggers `resetSessionInitStateLocked()` at `:174`. A `SESSION_BUSY` does not clear the cache, so back-to-back retries always present the same verify code — there is no "previous (cancelled) attempt" with a stale code. Earlier wording about a race was incorrect.

10. **Server-policy fields in `SESSION_ACCEPT` clamp unilaterally on the client; no on-the-wire renegotiation.**
    `applySessionClientPolicy` runs in-line with the accept-path and overwrites the local cfg from `VpnProto.ApplySessionAcceptClientPolicy(before, policy)` (`internal/client/session.go:182-240`). The server then enforces its own configured limits on incoming packets independently — anything outside the bounds is simply discarded server-side. C++ port must apply the clamp at SESSION_ACCEPT time and not attempt to renegotiate.

11. **MTU probe `effectiveDownloadMTUProbeSize` is additive, not subtractive.**
    Formula at `internal/client/mtu.go:1528-1534`: `effectiveDownloadMTUProbeSize(downloadMTU) = downloadMTU + reserve`, where `reserve = max(0, MaxHeaderRawSize() - HeaderRawSize(PACKET_MTU_DOWN_RES))` (`:41-47`). For the current packet-type catalogue, `MaxHeaderRawSize() = 11` (the `S,N,F,C` types) and `HeaderRawSize(PACKET_MTU_DOWN_RES) = 11`, so `reserve = 0` and the effective probe size equals the negotiated download MTU. The formula generalises if a future packet type increases the maximum.

12. **Round-robin counters are unbounded `atomic.Uint64`.**
    The dispatcher reads `counter.Add(1) % len(set)` per request; uint64 wrap is irrelevant on any realistic uptime. C++ port can use `std::atomic<uint64_t>` identically.

13. **Default base codec is lowercase base36.**
    `internal/basecodec/codec.go:23-32` aliases `Encode`/`Decode`/`DecodeString` directly onto `EncodeLowerBase36*` / `DecodeLowerBase36*`. The comment on line 22 reading "default: LowerBase32" is upstream documentation rot — the actual dispatch is base36. C++ port must use base36 to interop with the stock server. The comment also documents how to flip to base32 by editing the alias body, so the codec choice is a build-time switch on the server.

14. **`PACKET_ERROR_DROP` payload is `'I','N','V'` + 4-byte big-endian xorshift nonce + 1-byte `byte(nonce)` = 8 bytes total.**
    Built at `internal/udpserver/server_session.go:608-621`: `payload[0..2] = "INV"`, `binary.BigEndian.PutUint32(payload[3:7], nonce)` where `nonce = s.pongNonce.Add(1)` xorshifted with shifts (13, 17, 5), then `payload[7] = byte(nonce)`. C++ clients should recognise the type byte and the 3-byte `INV` prefix, then re-init the session; the nonce is anti-replay seasoning and need not be parsed.

---

*End of specification.*
