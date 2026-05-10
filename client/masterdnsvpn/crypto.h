// SPDX-License-Identifier: GPL-3.0-or-later
//
// Symmetric ciphers for the MasterDnsVPN tunnel. Wraps OpenSSL EVP, exposes
// a tiny seal()/open() pair that takes the operator's pre-shared key and
// the in-band nonce/IV the wire format reserves bytes for.
//
// Methods (matching upstream's DATA_ENCRYPTION_METHOD integer codes):
//
//   None        (0)  — passthrough; no MAC. Useful only for protocol
//                       diagnostics. The Engine refuses to start with this
//                       in production builds (see engine.cpp).
//   Xor         (1)  — repeating-key XOR. Provides obfuscation only.
//   ChaCha20    (2)  — stream cipher, 256-bit key, 96-bit nonce. No MAC.
//   Aes128Gcm   (3)  — AEAD, 128-bit key, 96-bit nonce, 128-bit tag.
//   Aes192Gcm   (4)  — AEAD, 192-bit key, 96-bit nonce, 128-bit tag.
//   Aes256Gcm   (5)  — AEAD, 256-bit key, 96-bit nonce, 128-bit tag.
//
// Key derivation: the operator-supplied `ENCRYPTION_KEY` is a free-form
// string. Each method's `requiredKeyBytes()` reports the derived key size;
// `deriveKey()` runs SHA-256 and truncates / hashes to fit. Matches what
// upstream's Go implementation does at the byte level (see the spec doc).
//
// Threading: Cipher instances are NOT thread-safe. Build one per direction
// (one for inbound, one for outbound) and serialise calls. ChaCha20 and
// AES-GCM use OpenSSL EVP_CIPHER_CTX which carries key schedule state.

#ifndef MASTERDNSVPN_CRYPTO_H
#define MASTERDNSVPN_CRYPTO_H

#include <QByteArray>
#include <QString>
#include <memory>
#include <optional>

namespace amnezia::masterdnsvpn {

enum class CipherMethod : int {
    None = 0,
    Xor = 1,
    ChaCha20 = 2,
    Aes128Gcm = 3,
    Aes192Gcm = 4,
    Aes256Gcm = 5,
};

// Convert from the integer wire value (0..5). Returns std::nullopt for
// unrecognised codes — callers should treat that as a configuration error.
std::optional<CipherMethod> cipherMethodFromInt(int code);

// Required derived-key length (bytes) for a given cipher.
int requiredKeyBytes(CipherMethod m);

// Required nonce/IV length (bytes) for a given cipher.
//   None      — 0
//   Xor       — 0  (key reused as the running XOR stream; no per-packet IV)
//   ChaCha20  — 16  (4-byte little-endian block counter || 12-byte nonce —
//                    matches upstream's golang.org/x/crypto/chacha20 usage,
//                    which OpenSSL's EVP_chacha20 expects in the same order)
//   Aes*Gcm   — 12
int requiredNonceBytes(CipherMethod m);

// AEAD authentication tag length (bytes). 0 for non-AEAD methods (None,
// Xor, ChaCha20). 16 for AES-GCM.
int authTagBytes(CipherMethod m);

// Derive a fixed-length symmetric key from the operator's free-form
// passphrase. Algorithm: SHA-256(passphrase) then take the first
// `requiredKeyBytes(m)` bytes (or hash again if more bytes are needed in
// some future cipher). Deterministic — same passphrase always yields the
// same key, which is the property the wire protocol depends on (operator
// and client must derive identical keys with no out-of-band coordination).
QByteArray deriveKey(CipherMethod m, const QString &passphrase);

// Stateful cipher object. Holds the OpenSSL EVP_CIPHER_CTX (or the XOR
// running offset). Use one instance per direction; do not share across
// threads.
class Cipher
{
public:
    Cipher();
    ~Cipher();
    Q_DISABLE_COPY_MOVE(Cipher)

    // Initialise with a method + derived key. Returns false on bad inputs
    // (wrong key size, OpenSSL allocation failure, …). Safe to call again
    // to reinitialise; previous state is discarded.
    bool init(CipherMethod m, const QByteArray &derivedKey);

    // Encrypt `plaintext` into `out`. Caller supplies a per-packet `nonce`
    // (must be `requiredNonceBytes(m)` long); `aad` is optional additional
    // authenticated data for AEAD modes. The output layout is method-
    // specific — for AEAD, the AEAD tag is appended to the ciphertext.
    // Returns false on failure; `out` is then in an indeterminate state.
    bool seal(const QByteArray &plaintext,
              const QByteArray &nonce,
              const QByteArray &aad,
              QByteArray &out);

    // Inverse of seal. Returns false on tag mismatch (AEAD), bad nonce
    // size, or any OpenSSL error. Decryption never produces partial output:
    // if it returns false the contents of `out` are not meaningful.
    bool open(const QByteArray &ciphertext,
              const QByteArray &nonce,
              const QByteArray &aad,
              QByteArray &out);

    CipherMethod method() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace amnezia::masterdnsvpn

#endif // MASTERDNSVPN_CRYPTO_H
