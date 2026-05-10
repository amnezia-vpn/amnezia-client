// SPDX-License-Identifier: GPL-3.0-or-later

#include "crypto.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <QDebug>
#include <algorithm>
#include <cstring>

namespace amnezia::masterdnsvpn {

namespace {

// Centralised lookup so the rest of the file doesn't sprinkle EVP_get_cipher
// calls. Returns nullptr for non-AEAD methods (None / Xor) — callers must
// special-case those.
const EVP_CIPHER *evpCipherFor(CipherMethod m)
{
    switch (m) {
    case CipherMethod::ChaCha20:
        return EVP_chacha20();
    case CipherMethod::Aes128Gcm:
        return EVP_aes_128_gcm();
    case CipherMethod::Aes192Gcm:
        return EVP_aes_192_gcm();
    case CipherMethod::Aes256Gcm:
        return EVP_aes_256_gcm();
    case CipherMethod::None:
    case CipherMethod::Xor:
        return nullptr;
    }
    return nullptr;
}

bool isAead(CipherMethod m)
{
    return m == CipherMethod::Aes128Gcm || m == CipherMethod::Aes192Gcm
            || m == CipherMethod::Aes256Gcm;
}

void logOpensslError(const char *what)
{
    const unsigned long e = ERR_get_error();
    char buf[256] = {};
    ERR_error_string_n(e, buf, sizeof(buf));
    qWarning("masterdnsvpn::crypto: %s failed: %s", what, buf);
}

} // namespace

std::optional<CipherMethod> cipherMethodFromInt(int code)
{
    switch (code) {
    case 0:
        return CipherMethod::None;
    case 1:
        return CipherMethod::Xor;
    case 2:
        return CipherMethod::ChaCha20;
    case 3:
        return CipherMethod::Aes128Gcm;
    case 4:
        return CipherMethod::Aes192Gcm;
    case 5:
        return CipherMethod::Aes256Gcm;
    default:
        return std::nullopt;
    }
}

int requiredKeyBytes(CipherMethod m)
{
    switch (m) {
    case CipherMethod::None:
        return 0;
    case CipherMethod::Xor:
        // No "key length" per se for XOR; we still derive 32 bytes so
        // operators get a meaningful entropy floor.
        return 32;
    case CipherMethod::ChaCha20:
        return 32;
    case CipherMethod::Aes128Gcm:
        return 16;
    case CipherMethod::Aes192Gcm:
        return 24;
    case CipherMethod::Aes256Gcm:
        return 32;
    }
    return 0;
}

int requiredNonceBytes(CipherMethod m)
{
    switch (m) {
    case CipherMethod::None:
    case CipherMethod::Xor:
        return 0;
    case CipherMethod::ChaCha20:
        // 16 = 4-byte block counter || 12-byte nonce, the layout OpenSSL's
        // EVP_chacha20 expects directly.
        return 16;
    case CipherMethod::Aes128Gcm:
    case CipherMethod::Aes192Gcm:
    case CipherMethod::Aes256Gcm:
        return 12;
    }
    return 0;
}

int authTagBytes(CipherMethod m)
{
    return isAead(m) ? 16 : 0;
}

QByteArray deriveKey(CipherMethod m, const QString &passphrase)
{
    // Per the wire spec, each cipher derives its key from the operator's
    // UTF-8 ENCRYPTION_KEY string by a *method-specific* rule:
    //
    //   None        — no key
    //   Xor         — raw UTF-8 bytes, zero-padded (or truncated) to 32 B
    //   ChaCha20    — SHA-256 of raw UTF-8 bytes (32 B)
    //   Aes128Gcm   — MD5 of raw UTF-8 bytes (16 B)         <-- yes, MD5
    //   Aes192Gcm   — raw UTF-8 bytes, zero-padded (or truncated) to 24 B
    //   Aes256Gcm   — SHA-256 of raw UTF-8 bytes (32 B)
    //
    // We're not designing the protocol — these are the exact derivations the
    // upstream implementation uses, and bit-for-bit compatibility is required
    // for the tunnel to work at all.
    const QByteArray utf8 = passphrase.toUtf8();

    auto rawPad = [&utf8](int n) {
        QByteArray buf(n, '\0');
        const int copy = std::min<int>(utf8.size(), n);
        if (copy > 0) {
            std::memcpy(buf.data(), utf8.constData(), copy);
        }
        return buf;
    };

    auto sha256Of = [&utf8]() {
        QByteArray buf(SHA256_DIGEST_LENGTH, '\0');
        SHA256(reinterpret_cast<const unsigned char *>(utf8.constData()),
               static_cast<size_t>(utf8.size()),
               reinterpret_cast<unsigned char *>(buf.data()));
        return buf;
    };

    auto md5Of = [&utf8]() {
        QByteArray buf(MD5_DIGEST_LENGTH, '\0');
        MD5(reinterpret_cast<const unsigned char *>(utf8.constData()),
            static_cast<size_t>(utf8.size()),
            reinterpret_cast<unsigned char *>(buf.data()));
        return buf;
    };

    switch (m) {
    case CipherMethod::None:
        return {};
    case CipherMethod::Xor:
        return rawPad(32);
    case CipherMethod::ChaCha20:
    case CipherMethod::Aes256Gcm:
        return sha256Of();
    case CipherMethod::Aes128Gcm:
        return md5Of();
    case CipherMethod::Aes192Gcm:
        return rawPad(24);
    }
    return {};
}

struct Cipher::Impl {
    CipherMethod method = CipherMethod::None;
    QByteArray key;

    // Stream-cipher state for XOR; OpenSSL handles its own state for the
    // EVP variants but we hold an opaque ctx so seal/open can be cheap.
    EVP_CIPHER_CTX *encCtx = nullptr;
    EVP_CIPHER_CTX *decCtx = nullptr;

    ~Impl()
    {
        if (encCtx) {
            EVP_CIPHER_CTX_free(encCtx);
        }
        if (decCtx) {
            EVP_CIPHER_CTX_free(decCtx);
        }
    }
};

Cipher::Cipher() : d(std::make_unique<Impl>()) {}

Cipher::~Cipher() = default;

CipherMethod Cipher::method() const
{
    return d->method;
}

bool Cipher::init(CipherMethod m, const QByteArray &derivedKey)
{
    if (derivedKey.size() != requiredKeyBytes(m)) {
        qWarning("masterdnsvpn::Cipher::init: key size %lld doesn't match "
                 "required %d for method %d",
                 static_cast<long long>(derivedKey.size()),
                 requiredKeyBytes(m),
                 static_cast<int>(m));
        return false;
    }

    d->method = m;
    d->key = derivedKey;

    if (d->encCtx) {
        EVP_CIPHER_CTX_free(d->encCtx);
        d->encCtx = nullptr;
    }
    if (d->decCtx) {
        EVP_CIPHER_CTX_free(d->decCtx);
        d->decCtx = nullptr;
    }

    if (m == CipherMethod::None || m == CipherMethod::Xor) {
        return true;
    }

    d->encCtx = EVP_CIPHER_CTX_new();
    d->decCtx = EVP_CIPHER_CTX_new();
    if (!d->encCtx || !d->decCtx) {
        logOpensslError("EVP_CIPHER_CTX_new");
        return false;
    }
    return true;
}

namespace {

// Run a stream of bytes through repeating-key XOR. Pure helper; matches
// the trivial spec for CipherMethod::Xor on both directions.
void xorStream(const QByteArray &key, const QByteArray &in, QByteArray &out)
{
    out.resize(in.size());
    if (key.isEmpty() || in.isEmpty()) {
        return;
    }
    const int klen = key.size();
    const char *kp = key.constData();
    const char *ip = in.constData();
    char *op = out.data();
    for (int i = 0; i < in.size(); ++i) {
        op[i] = ip[i] ^ kp[i % klen];
    }
}

bool sealAead(EVP_CIPHER_CTX *ctx,
              const EVP_CIPHER *cipher,
              const QByteArray &key,
              const QByteArray &plaintext,
              const QByteArray &nonce,
              const QByteArray &aad,
              QByteArray &out)
{
    if (!EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr)) {
        logOpensslError("EVP_EncryptInit_ex (cipher)");
        return false;
    }
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), nullptr)) {
        logOpensslError("EVP_CIPHER_CTX_ctrl (set IV len)");
        return false;
    }
    if (!EVP_EncryptInit_ex(ctx,
                            nullptr,
                            nullptr,
                            reinterpret_cast<const unsigned char *>(key.constData()),
                            reinterpret_cast<const unsigned char *>(nonce.constData()))) {
        logOpensslError("EVP_EncryptInit_ex (key/nonce)");
        return false;
    }

    int outLen = 0;
    if (!aad.isEmpty()) {
        if (!EVP_EncryptUpdate(ctx,
                               nullptr,
                               &outLen,
                               reinterpret_cast<const unsigned char *>(aad.constData()),
                               aad.size())) {
            logOpensslError("EVP_EncryptUpdate (AAD)");
            return false;
        }
    }

    QByteArray cipherText(plaintext.size() + EVP_CIPHER_block_size(cipher), '\0');
    if (!EVP_EncryptUpdate(ctx,
                           reinterpret_cast<unsigned char *>(cipherText.data()),
                           &outLen,
                           reinterpret_cast<const unsigned char *>(plaintext.constData()),
                           plaintext.size())) {
        logOpensslError("EVP_EncryptUpdate");
        return false;
    }
    int written = outLen;

    int finalLen = 0;
    if (!EVP_EncryptFinal_ex(ctx,
                             reinterpret_cast<unsigned char *>(cipherText.data()) + written,
                             &finalLen)) {
        logOpensslError("EVP_EncryptFinal_ex");
        return false;
    }
    written += finalLen;
    cipherText.resize(written);

    QByteArray tag(16, '\0');
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tag.size(), tag.data())) {
        logOpensslError("EVP_CIPHER_CTX_ctrl (get tag)");
        return false;
    }

    out = cipherText + tag;
    return true;
}

bool openAead(EVP_CIPHER_CTX *ctx,
              const EVP_CIPHER *cipher,
              const QByteArray &key,
              const QByteArray &ciphertextWithTag,
              const QByteArray &nonce,
              const QByteArray &aad,
              QByteArray &out)
{
    constexpr int kTagLen = 16;
    if (ciphertextWithTag.size() < kTagLen) {
        qWarning("masterdnsvpn::Cipher::open: input too short for AEAD tag");
        return false;
    }
    const QByteArray ct = ciphertextWithTag.left(ciphertextWithTag.size() - kTagLen);
    const QByteArray tag = ciphertextWithTag.right(kTagLen);

    if (!EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr)) {
        logOpensslError("EVP_DecryptInit_ex (cipher)");
        return false;
    }
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), nullptr)) {
        logOpensslError("EVP_CIPHER_CTX_ctrl (set IV len)");
        return false;
    }
    if (!EVP_DecryptInit_ex(ctx,
                            nullptr,
                            nullptr,
                            reinterpret_cast<const unsigned char *>(key.constData()),
                            reinterpret_cast<const unsigned char *>(nonce.constData()))) {
        logOpensslError("EVP_DecryptInit_ex (key/nonce)");
        return false;
    }

    int outLen = 0;
    if (!aad.isEmpty()) {
        if (!EVP_DecryptUpdate(ctx,
                               nullptr,
                               &outLen,
                               reinterpret_cast<const unsigned char *>(aad.constData()),
                               aad.size())) {
            logOpensslError("EVP_DecryptUpdate (AAD)");
            return false;
        }
    }

    QByteArray plain(ct.size() + EVP_CIPHER_block_size(cipher), '\0');
    if (!EVP_DecryptUpdate(ctx,
                           reinterpret_cast<unsigned char *>(plain.data()),
                           &outLen,
                           reinterpret_cast<const unsigned char *>(ct.constData()),
                           ct.size())) {
        logOpensslError("EVP_DecryptUpdate");
        return false;
    }
    int written = outLen;

    if (!EVP_CIPHER_CTX_ctrl(ctx,
                             EVP_CTRL_AEAD_SET_TAG,
                             tag.size(),
                             const_cast<char *>(tag.constData()))) {
        logOpensslError("EVP_CIPHER_CTX_ctrl (set tag)");
        return false;
    }

    int finalLen = 0;
    // Authenticated decryption returns 0 here on tag mismatch — that's the
    // signal we surface to the caller as "this packet was tampered with".
    if (!EVP_DecryptFinal_ex(ctx,
                             reinterpret_cast<unsigned char *>(plain.data()) + written,
                             &finalLen)) {
        // Don't log via logOpensslError here — tag mismatches are routine on
        // hostile networks and would spam the log. Caller decides if it's
        // worth surfacing.
        return false;
    }
    written += finalLen;
    plain.resize(written);
    out = std::move(plain);
    return true;
}

bool sealStream(EVP_CIPHER_CTX *ctx,
                const EVP_CIPHER *cipher,
                const QByteArray &key,
                const QByteArray &plaintext,
                const QByteArray &nonce,
                QByteArray &out)
{
    if (!EVP_EncryptInit_ex(ctx,
                            cipher,
                            nullptr,
                            reinterpret_cast<const unsigned char *>(key.constData()),
                            reinterpret_cast<const unsigned char *>(nonce.constData()))) {
        logOpensslError("EVP_EncryptInit_ex (stream)");
        return false;
    }
    out.resize(plaintext.size());
    int outLen = 0;
    if (!EVP_EncryptUpdate(ctx,
                           reinterpret_cast<unsigned char *>(out.data()),
                           &outLen,
                           reinterpret_cast<const unsigned char *>(plaintext.constData()),
                           plaintext.size())) {
        logOpensslError("EVP_EncryptUpdate (stream)");
        return false;
    }
    int finalLen = 0;
    if (!EVP_EncryptFinal_ex(ctx,
                             reinterpret_cast<unsigned char *>(out.data()) + outLen,
                             &finalLen)) {
        logOpensslError("EVP_EncryptFinal_ex (stream)");
        return false;
    }
    out.resize(outLen + finalLen);
    return true;
}

} // namespace

bool Cipher::seal(const QByteArray &plaintext,
                  const QByteArray &nonce,
                  const QByteArray &aad,
                  QByteArray &out)
{
    if (nonce.size() != requiredNonceBytes(d->method)) {
        qWarning("masterdnsvpn::Cipher::seal: nonce size %lld doesn't match "
                 "required %d for method %d",
                 static_cast<long long>(nonce.size()),
                 requiredNonceBytes(d->method),
                 static_cast<int>(d->method));
        return false;
    }
    switch (d->method) {
    case CipherMethod::None:
        out = plaintext;
        return true;
    case CipherMethod::Xor:
        xorStream(d->key, plaintext, out);
        return true;
    case CipherMethod::ChaCha20:
        return sealStream(d->encCtx, EVP_chacha20(), d->key, plaintext, nonce, out);
    case CipherMethod::Aes128Gcm:
    case CipherMethod::Aes192Gcm:
    case CipherMethod::Aes256Gcm:
        return sealAead(d->encCtx, evpCipherFor(d->method), d->key, plaintext, nonce, aad, out);
    }
    return false;
}

bool Cipher::open(const QByteArray &ciphertext,
                  const QByteArray &nonce,
                  const QByteArray &aad,
                  QByteArray &out)
{
    if (nonce.size() != requiredNonceBytes(d->method)) {
        qWarning("masterdnsvpn::Cipher::open: nonce size %lld doesn't match "
                 "required %d for method %d",
                 static_cast<long long>(nonce.size()),
                 requiredNonceBytes(d->method),
                 static_cast<int>(d->method));
        return false;
    }
    switch (d->method) {
    case CipherMethod::None:
        out = ciphertext;
        return true;
    case CipherMethod::Xor:
        xorStream(d->key, ciphertext, out);
        return true;
    case CipherMethod::ChaCha20: {
        // ChaCha20 is a stream cipher: encrypt() and decrypt() are the
        // same operation. EVP_DecryptInit_ex would also work but we keep
        // the symmetric-call path explicit for testability.
        if (!EVP_DecryptInit_ex(d->decCtx,
                                EVP_chacha20(),
                                nullptr,
                                reinterpret_cast<const unsigned char *>(d->key.constData()),
                                reinterpret_cast<const unsigned char *>(nonce.constData()))) {
            logOpensslError("EVP_DecryptInit_ex (chacha20)");
            return false;
        }
        out.resize(ciphertext.size());
        int outLen = 0;
        if (!EVP_DecryptUpdate(d->decCtx,
                               reinterpret_cast<unsigned char *>(out.data()),
                               &outLen,
                               reinterpret_cast<const unsigned char *>(ciphertext.constData()),
                               ciphertext.size())) {
            logOpensslError("EVP_DecryptUpdate (chacha20)");
            return false;
        }
        int finalLen = 0;
        if (!EVP_DecryptFinal_ex(d->decCtx,
                                 reinterpret_cast<unsigned char *>(out.data()) + outLen,
                                 &finalLen)) {
            logOpensslError("EVP_DecryptFinal_ex (chacha20)");
            return false;
        }
        out.resize(outLen + finalLen);
        return true;
    }
    case CipherMethod::Aes128Gcm:
    case CipherMethod::Aes192Gcm:
    case CipherMethod::Aes256Gcm:
        return openAead(d->decCtx, evpCipherFor(d->method), d->key, ciphertext, nonce, aad, out);
    }
    return false;
}

} // namespace amnezia::masterdnsvpn
