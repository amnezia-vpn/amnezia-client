/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SODIUM_COMPAT_H_
#define BOTAN_SODIUM_COMPAT_H_

#include <botan/types.h>

namespace Botan {

/**
* The Sodium namespace contains a partial implementation of the
* libsodium API.
*/
namespace Sodium {

// sodium/randombytes.h
enum Sodium_Constants : size_t {
   SODIUM_SIZE_MAX = 0xFFFFFFFF,

   crypto_aead_chacha20poly1305_ABYTES = 16,
   crypto_aead_chacha20poly1305_KEYBYTES = 32,
   crypto_aead_chacha20poly1305_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_aead_chacha20poly1305_NPUBBYTES = 8,
   crypto_aead_chacha20poly1305_NSECBYTES = 0,

   crypto_aead_chacha20poly1305_ietf_ABYTES = 16,
   crypto_aead_chacha20poly1305_ietf_KEYBYTES = 32,
   crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_aead_chacha20poly1305_ietf_NPUBBYTES = 12,
   crypto_aead_chacha20poly1305_ietf_NSECBYTES = 0,

   crypto_aead_xchacha20poly1305_ietf_ABYTES = 16,
   crypto_aead_xchacha20poly1305_ietf_KEYBYTES = 32,
   crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_aead_xchacha20poly1305_ietf_NPUBBYTES = 24,
   crypto_aead_xchacha20poly1305_ietf_NSECBYTES = 0,

   crypto_auth_hmacsha256_BYTES = 32,
   crypto_auth_hmacsha256_KEYBYTES = 32,
   crypto_auth_hmacsha512256_BYTES = 32,
   crypto_auth_hmacsha512256_KEYBYTES = 32,
   crypto_auth_hmacsha512_BYTES = 64,
   crypto_auth_hmacsha512_KEYBYTES = 32,

   crypto_auth_BYTES = crypto_auth_hmacsha512256_BYTES,
   crypto_auth_KEYBYTES = crypto_auth_hmacsha512256_KEYBYTES,

   crypto_box_curve25519xsalsa20poly1305_BEFORENMBYTES = 32,
   crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES = 16,
   crypto_box_curve25519xsalsa20poly1305_MACBYTES = 16,
   crypto_box_curve25519xsalsa20poly1305_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_box_curve25519xsalsa20poly1305_NONCEBYTES = 24,
   crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES = 32,
   crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES = 32,
   crypto_box_curve25519xsalsa20poly1305_SEEDBYTES = 32,
   crypto_box_curve25519xsalsa20poly1305_ZEROBYTES = crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES + crypto_box_curve25519xsalsa20poly1305_MACBYTES,

   crypto_box_BEFORENMBYTES = crypto_box_curve25519xsalsa20poly1305_BEFORENMBYTES,
   crypto_box_BOXZEROBYTES = crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES,
   crypto_box_MACBYTES = crypto_box_curve25519xsalsa20poly1305_MACBYTES,
   crypto_box_MESSAGEBYTES_MAX = crypto_box_curve25519xsalsa20poly1305_MESSAGEBYTES_MAX,
   crypto_box_NONCEBYTES = crypto_box_curve25519xsalsa20poly1305_NONCEBYTES,
   crypto_box_PUBLICKEYBYTES = crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES,
   crypto_box_SECRETKEYBYTES = crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES,
   crypto_box_SEEDBYTES = crypto_box_curve25519xsalsa20poly1305_SEEDBYTES,
   crypto_box_ZEROBYTES = crypto_box_curve25519xsalsa20poly1305_ZEROBYTES,

   crypto_core_hchacha20_CONSTBYTES = 16,
   crypto_core_hchacha20_INPUTBYTES = 16,
   crypto_core_hchacha20_KEYBYTES = 32,
   crypto_core_hchacha20_OUTPUTBYTES = 32,

   crypto_core_hsalsa20_CONSTBYTES = 16,
   crypto_core_hsalsa20_INPUTBYTES = 16,
   crypto_core_hsalsa20_KEYBYTES = 32,
   crypto_core_hsalsa20_OUTPUTBYTES = 32,

   crypto_hash_sha256_BYTES = 32,
   crypto_hash_sha512_BYTES = 64,
   crypto_hash_BYTES = crypto_hash_sha512_BYTES,

   crypto_onetimeauth_poly1305_BYTES = 16,
   crypto_onetimeauth_poly1305_KEYBYTES = 32,
   crypto_onetimeauth_BYTES = crypto_onetimeauth_poly1305_BYTES,
   crypto_onetimeauth_KEYBYTES = crypto_onetimeauth_poly1305_KEYBYTES,

   crypto_scalarmult_curve25519_BYTES = 32,
   crypto_scalarmult_curve25519_SCALARBYTES = 32,
   crypto_scalarmult_BYTES = crypto_scalarmult_curve25519_BYTES,
   crypto_scalarmult_SCALARBYTES = crypto_scalarmult_curve25519_SCALARBYTES,

   crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES = 16,
   crypto_secretbox_xsalsa20poly1305_KEYBYTES = 32,
   crypto_secretbox_xsalsa20poly1305_MACBYTES = 16,
   crypto_secretbox_xsalsa20poly1305_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_secretbox_xsalsa20poly1305_NONCEBYTES = 24,
   crypto_secretbox_xsalsa20poly1305_ZEROBYTES = crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES + crypto_secretbox_xsalsa20poly1305_MACBYTES,

   crypto_secretbox_BOXZEROBYTES = crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES,
   crypto_secretbox_KEYBYTES = crypto_secretbox_xsalsa20poly1305_KEYBYTES,
   crypto_secretbox_MACBYTES = crypto_secretbox_xsalsa20poly1305_MACBYTES,
   crypto_secretbox_MESSAGEBYTES_MAX = crypto_secretbox_xsalsa20poly1305_MESSAGEBYTES_MAX,
   crypto_secretbox_NONCEBYTES = crypto_secretbox_xsalsa20poly1305_NONCEBYTES,
   crypto_secretbox_ZEROBYTES = crypto_secretbox_xsalsa20poly1305_ZEROBYTES,

   crypto_shorthash_siphash24_BYTES = 8,
   crypto_shorthash_siphash24_KEYBYTES = 16,
   crypto_shorthash_BYTES = crypto_shorthash_siphash24_BYTES,
   crypto_shorthash_KEYBYTES = crypto_shorthash_siphash24_KEYBYTES,

   crypto_sign_ed25519_BYTES = 64,
   crypto_sign_ed25519_MESSAGEBYTES_MAX = (SODIUM_SIZE_MAX - crypto_sign_ed25519_BYTES),
   crypto_sign_ed25519_PUBLICKEYBYTES = 32,
   crypto_sign_ed25519_SECRETKEYBYTES = (32 + 32),
   crypto_sign_ed25519_SEEDBYTES = 32,
   crypto_sign_BYTES = crypto_sign_ed25519_BYTES,
   crypto_sign_MESSAGEBYTES_MAX = crypto_sign_ed25519_MESSAGEBYTES_MAX,
   crypto_sign_PUBLICKEYBYTES = crypto_sign_ed25519_PUBLICKEYBYTES,
   crypto_sign_SECRETKEYBYTES = crypto_sign_ed25519_SECRETKEYBYTES,
   crypto_sign_SEEDBYTES = crypto_sign_ed25519_SEEDBYTES,

   crypto_stream_chacha20_KEYBYTES = 32,
   crypto_stream_chacha20_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_stream_chacha20_NONCEBYTES = 8,
   crypto_stream_chacha20_ietf_KEYBYTES = 32,
   crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_stream_chacha20_ietf_NONCEBYTES = 12,
   crypto_stream_salsa20_KEYBYTES = 32,
   crypto_stream_salsa20_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_stream_salsa20_NONCEBYTES = 8,
   crypto_stream_xchacha20_KEYBYTES = 32,
   crypto_stream_xchacha20_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_stream_xchacha20_NONCEBYTES = 24,
   crypto_stream_xsalsa20_KEYBYTES = 32,
   crypto_stream_xsalsa20_MESSAGEBYTES_MAX = SODIUM_SIZE_MAX,
   crypto_stream_xsalsa20_NONCEBYTES = 24,
   crypto_stream_KEYBYTES = crypto_stream_xsalsa20_KEYBYTES,
   crypto_stream_MESSAGEBYTES_MAX = crypto_stream_xsalsa20_MESSAGEBYTES_MAX,
   crypto_stream_NONCEBYTES = crypto_stream_xsalsa20_NONCEBYTES,

   crypto_verify_16_BYTES = 16,
   crypto_verify_32_BYTES = 32,
   crypto_verify_64_BYTES = 64,

   randombytes_SEEDBYTES = 32,
};

inline const char* sodium_version_string() { return "Botan Sodium Compat"; }

inline int sodium_library_version_major() { return 0; }

inline int sodium_library_version_minor() { return 0; }

inline int sodium_library_minimal() { return 0; }

inline int sodium_init() { return 0; }

// sodium/crypto_verify_{16,32,64}.h

BOTAN_PUBLIC_API(2,11)
int crypto_verify_16(const uint8_t x[16], const uint8_t y[16]);

BOTAN_PUBLIC_API(2,11)
int crypto_verify_32(const uint8_t x[32], const uint8_t y[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_verify_64(const uint8_t x[64], const uint8_t y[64]);

// sodium/utils.h
BOTAN_PUBLIC_API(2,11)
void sodium_memzero(void* ptr, size_t len);

BOTAN_PUBLIC_API(2,11)
int sodium_memcmp(const void* x, const void* y, size_t len);

BOTAN_PUBLIC_API(2,11)
int sodium_compare(const uint8_t x[], const uint8_t y[], size_t len);

BOTAN_PUBLIC_API(2,11)
int sodium_is_zero(const uint8_t nonce[], size_t nlen);

BOTAN_PUBLIC_API(2,11)
void sodium_increment(uint8_t n[], size_t nlen);

BOTAN_PUBLIC_API(2,11)
void sodium_add(uint8_t a[], const uint8_t b[], size_t len);

BOTAN_PUBLIC_API(2,11)
void* sodium_malloc(size_t size);

BOTAN_PUBLIC_API(2,11)
void* sodium_allocarray(size_t count, size_t size);

BOTAN_PUBLIC_API(2,11)
void sodium_free(void* ptr);

BOTAN_PUBLIC_API(2,11)
int sodium_mprotect_noaccess(void* ptr);

BOTAN_PUBLIC_API(2,11)
int sodium_mprotect_readwrite(void* ptr);

// sodium/randombytes.h

inline size_t randombytes_seedbytes() { return randombytes_SEEDBYTES; }

BOTAN_PUBLIC_API(2,11)
void randombytes_buf(void* buf, size_t size);

BOTAN_PUBLIC_API(2,11)
void randombytes_buf_deterministic(void* buf, size_t size,
                                   const uint8_t seed[randombytes_SEEDBYTES]);

BOTAN_PUBLIC_API(2,11)
uint32_t randombytes_uniform(uint32_t upper_bound);

inline uint32_t randombytes_random()
   {
   uint32_t x = 0;
   randombytes_buf(&x, 4);
   return x;
   }

inline void randombytes_stir() {}

inline int randombytes_close() { return 0; }

inline const char* randombytes_implementation_name()
   {
   return "botan";
   }

inline void randombytes(uint8_t buf[], size_t buf_len)
   {
   return randombytes_buf(buf, buf_len);
   }

// sodium/crypto_secretbox_xsalsa20poly1305.h

inline size_t crypto_secretbox_xsalsa20poly1305_keybytes()
   {
   return crypto_secretbox_xsalsa20poly1305_KEYBYTES;
   }

inline size_t crypto_secretbox_xsalsa20poly1305_noncebytes()
   {
   return crypto_secretbox_xsalsa20poly1305_NONCEBYTES;
   }

inline size_t crypto_secretbox_xsalsa20poly1305_macbytes()
   {
   return crypto_secretbox_xsalsa20poly1305_MACBYTES;
   }

inline size_t crypto_secretbox_xsalsa20poly1305_messagebytes_max()
   {
   return crypto_secretbox_xsalsa20poly1305_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_secretbox_xsalsa20poly1305(uint8_t ctext[],
                                      const uint8_t ptext[],
                                      size_t ptext_len,
                                      const uint8_t nonce[],
                                      const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_secretbox_xsalsa20poly1305_open(uint8_t ptext[],
                                           const uint8_t ctext[],
                                           size_t ctext_len,
                                           const uint8_t nonce[],
                                           const uint8_t key[]);

inline void crypto_secretbox_xsalsa20poly1305_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

inline size_t crypto_secretbox_xsalsa20poly1305_boxzerobytes()
   {
   return crypto_secretbox_xsalsa20poly1305_BOXZEROBYTES;
   }

inline size_t crypto_secretbox_xsalsa20poly1305_zerobytes()
   {
   return crypto_secretbox_xsalsa20poly1305_ZEROBYTES;
   }

// sodium/crypto_secretbox.h

inline size_t crypto_secretbox_keybytes() { return crypto_secretbox_KEYBYTES; }

inline size_t crypto_secretbox_noncebytes() { return crypto_secretbox_NONCEBYTES; }

inline size_t crypto_secretbox_macbytes() { return crypto_secretbox_MACBYTES; }

inline size_t crypto_secretbox_messagebytes_max() { return crypto_secretbox_xsalsa20poly1305_MESSAGEBYTES_MAX; }

inline const char* crypto_secretbox_primitive() { return "xsalsa20poly1305"; }

BOTAN_PUBLIC_API(2,11)
int crypto_secretbox_detached(uint8_t ctext[], uint8_t mac[],
                              const uint8_t ptext[],
                              size_t ptext_len,
                              const uint8_t nonce[],
                              const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_secretbox_open_detached(uint8_t ptext[],
                                   const uint8_t ctext[],
                                   const uint8_t mac[],
                                   size_t ctext_len,
                                   const uint8_t nonce[],
                                   const uint8_t key[]);

inline int crypto_secretbox_easy(uint8_t ctext[], const uint8_t ptext[],
                                 size_t ptext_len, const uint8_t nonce[],
                                 const uint8_t key[])
   {
   return crypto_secretbox_detached(ctext + crypto_secretbox_MACBYTES, ctext,
                                    ptext, ptext_len, nonce, key);
   }

inline int crypto_secretbox_open_easy(uint8_t out[], const uint8_t ctext[], size_t ctext_len,
                                      const uint8_t nonce[], const uint8_t key[])
   {
   if(ctext_len < crypto_secretbox_MACBYTES)
      {
      return -1;
      }

   return crypto_secretbox_open_detached(out, ctext + crypto_secretbox_MACBYTES,
                                         ctext, ctext_len - crypto_secretbox_MACBYTES,
                                         nonce, key);
   }

inline void crypto_secretbox_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

inline size_t crypto_secretbox_zerobytes()
   {
   return crypto_secretbox_ZEROBYTES;
   }

inline size_t crypto_secretbox_boxzerobytes()
   {
   return crypto_secretbox_BOXZEROBYTES;
   }

inline int crypto_secretbox(uint8_t ctext[], const uint8_t ptext[],
                            size_t ptext_len, const uint8_t nonce[],
                            const uint8_t key[])
   {
   return crypto_secretbox_xsalsa20poly1305(ctext, ptext, ptext_len, nonce, key);
   }

inline int crypto_secretbox_open(uint8_t ptext[], const uint8_t ctext[],
                                 size_t ctext_len, const uint8_t nonce[],
                                 const uint8_t key[])
   {
   return crypto_secretbox_xsalsa20poly1305_open(ptext, ctext, ctext_len, nonce, key);
   }

// sodium/crypto_aead_xchacha20poly1305.h

inline size_t crypto_aead_chacha20poly1305_ietf_keybytes()
   {
   return crypto_aead_chacha20poly1305_ietf_KEYBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_ietf_nsecbytes()
   {
   return crypto_aead_chacha20poly1305_ietf_NSECBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_ietf_npubbytes()
   {
   return crypto_aead_chacha20poly1305_ietf_NPUBBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_ietf_abytes()
   {
   return crypto_aead_chacha20poly1305_ietf_ABYTES;
   }

inline size_t crypto_aead_chacha20poly1305_ietf_messagebytes_max()
   {
   return crypto_aead_chacha20poly1305_ietf_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_ietf_encrypt(uint8_t ctext[],
                                              unsigned long long* ctext_len,
                                              const uint8_t ptext[],
                                              size_t ptext_len,
                                              const uint8_t ad[],
                                              size_t ad_len,
                                              const uint8_t unused_secret_nonce[],
                                              const uint8_t nonce[],
                                              const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_ietf_decrypt(uint8_t ptext[],
                                              unsigned long long* ptext_len,
                                              uint8_t unused_secret_nonce[],
                                              const uint8_t ctext[],
                                              size_t ctext_len,
                                              const uint8_t ad[],
                                              size_t ad_len,
                                              const uint8_t nonce[],
                                              const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_ietf_encrypt_detached(uint8_t ctext[],
                                                       uint8_t mac[],
                                                       unsigned long long* mac_len,
                                                       const uint8_t ptext[],
                                                       size_t ptext_len,
                                                       const uint8_t ad[],
                                                       size_t ad_len,
                                                       const uint8_t unused_secret_nonce[],
                                                       const uint8_t nonce[],
                                                       const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_ietf_decrypt_detached(uint8_t m[],
                                                       uint8_t unused_secret_nonce[],
                                                       const uint8_t ctext[],
                                                       size_t ctext_len,
                                                       const uint8_t mac[],
                                                       const uint8_t ad[],
                                                       size_t ad_len,
                                                       const uint8_t nonce[],
                                                       const uint8_t key[]);

inline void crypto_aead_chacha20poly1305_ietf_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, crypto_aead_chacha20poly1305_ietf_KEYBYTES);
   }

inline size_t crypto_aead_chacha20poly1305_keybytes()
   {
   return crypto_aead_chacha20poly1305_KEYBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_nsecbytes()
   {
   return crypto_aead_chacha20poly1305_NSECBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_npubbytes()
   {
   return crypto_aead_chacha20poly1305_NPUBBYTES;
   }

inline size_t crypto_aead_chacha20poly1305_abytes()
   {
   return crypto_aead_chacha20poly1305_ABYTES;
   }

inline size_t crypto_aead_chacha20poly1305_messagebytes_max()
   {
   return crypto_aead_chacha20poly1305_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_encrypt(uint8_t ctext[],
                                         unsigned long long* ctext_len,
                                         const uint8_t ptext[],
                                         size_t ptext_len,
                                         const uint8_t ad[],
                                         size_t ad_len,
                                         const uint8_t unused_secret_nonce[],
                                         const uint8_t nonce[],
                                         const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_decrypt(uint8_t m[],
                                         unsigned long long* ptext_len,
                                         uint8_t unused_secret_nonce[],
                                         const uint8_t ctext[],
                                         size_t ctext_len,
                                         const uint8_t ad[],
                                         size_t ad_len,
                                         const uint8_t nonce[],
                                         const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_encrypt_detached(uint8_t ctext[],
                                                  uint8_t mac[],
                                                  unsigned long long* mac_len,
                                                  const uint8_t ptext[],
                                                  size_t ptext_len,
                                                  const uint8_t ad[],
                                                  size_t ad_len,
                                                  const uint8_t unused_secret_nonce[],
                                                  const uint8_t nonce[],
                                                  const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_chacha20poly1305_decrypt_detached(uint8_t m[],
                                                  uint8_t unused_secret_nonce[],
                                                  const uint8_t ctext[],
                                                  size_t ctext_len,
                                                  const uint8_t mac[],
                                                  const uint8_t ad[],
                                                  size_t ad_len,
                                                  const uint8_t nonce[],
                                                  const uint8_t key[]);

inline void crypto_aead_chacha20poly1305_keygen(uint8_t k[32])
   {
   randombytes_buf(k, 32);
   }

// sodium/crypto_aead_xchacha20poly1305.h

inline size_t crypto_aead_xchacha20poly1305_ietf_keybytes()
   {
   return crypto_aead_xchacha20poly1305_ietf_KEYBYTES;
   }

inline size_t crypto_aead_xchacha20poly1305_ietf_nsecbytes()
   {
   return crypto_aead_xchacha20poly1305_ietf_NSECBYTES;
   }

inline size_t crypto_aead_xchacha20poly1305_ietf_npubbytes()
   {
   return crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
   }

inline size_t crypto_aead_xchacha20poly1305_ietf_abytes()
   {
   return crypto_aead_xchacha20poly1305_ietf_ABYTES;
   }

inline size_t crypto_aead_xchacha20poly1305_ietf_messagebytes_max()
   {
   return crypto_aead_xchacha20poly1305_ietf_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_aead_xchacha20poly1305_ietf_encrypt(uint8_t ctext[],
                                               unsigned long long* ctext_len,
                                               const uint8_t ptext[],
                                               size_t ptext_len,
                                               const uint8_t ad[],
                                               size_t ad_len,
                                               const uint8_t unused_secret_nonce[],
                                               const uint8_t nonce[],
                                               const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_xchacha20poly1305_ietf_decrypt(uint8_t ptext[],
                                               unsigned long long* ptext_len,
                                               uint8_t unused_secret_nonce[],
                                               const uint8_t ctext[],
                                               size_t ctext_len,
                                               const uint8_t ad[],
                                               size_t ad_len,
                                               const uint8_t nonce[],
                                               const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_xchacha20poly1305_ietf_encrypt_detached(uint8_t ctext[],
                                                        uint8_t mac[],
                                                        unsigned long long* mac_len,
                                                        const uint8_t ptext[],
                                                        size_t ptext_len,
                                                        const uint8_t ad[],
                                                        size_t ad_len,
                                                        const uint8_t unused_secret_nonce[],
                                                        const uint8_t nonce[],
                                                        const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_aead_xchacha20poly1305_ietf_decrypt_detached(uint8_t ptext[],
                                                        uint8_t unused_secret_nonce[],
                                                        const uint8_t ctext[],
                                                        size_t ctext_len,
                                                        const uint8_t mac[],
                                                        const uint8_t ad[],
                                                        size_t ad_len,
                                                        const uint8_t nonce[],
                                                        const uint8_t key[]);

inline void crypto_aead_xchacha20poly1305_ietf_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_box_curve25519xsalsa20poly1305.h

inline size_t crypto_box_curve25519xsalsa20poly1305_seedbytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_SEEDBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_publickeybytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_secretkeybytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_beforenmbytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_BEFORENMBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_noncebytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_NONCEBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_macbytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_MACBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_messagebytes_max()
   {
   return crypto_box_curve25519xsalsa20poly1305_MESSAGEBYTES_MAX;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_boxzerobytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES;
   }

inline size_t crypto_box_curve25519xsalsa20poly1305_zerobytes()
   {
   return crypto_box_curve25519xsalsa20poly1305_ZEROBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_box_curve25519xsalsa20poly1305_seed_keypair(uint8_t pk[32],
                                                       uint8_t sk[32],
                                                       const uint8_t seed[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_box_curve25519xsalsa20poly1305_keypair(uint8_t pk[32],
                                                  uint8_t sk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_box_curve25519xsalsa20poly1305_beforenm(uint8_t key[],
                                                   const uint8_t pk[32],
                                                   const uint8_t sk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_box_curve25519xsalsa20poly1305(uint8_t ctext[],
                                          const uint8_t ptext[],
                                          size_t ptext_len,
                                          const uint8_t nonce[],
                                          const uint8_t pk[32],
                                          const uint8_t sk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_box_curve25519xsalsa20poly1305_open(uint8_t ptext[],
                                               const uint8_t ctext[],
                                               size_t ctext_len,
                                               const uint8_t nonce[],
                                               const uint8_t pk[32],
                                               const uint8_t sk[32]);

inline int crypto_box_curve25519xsalsa20poly1305_afternm(uint8_t ctext[],
                                                         const uint8_t ptext[],
                                                         size_t ptext_len,
                                                         const uint8_t nonce[],
                                                         const uint8_t key[])
   {
   return crypto_secretbox_xsalsa20poly1305(ctext, ptext, ptext_len, nonce, key);
   }

inline int crypto_box_curve25519xsalsa20poly1305_open_afternm(uint8_t ptext[],
                                                              const uint8_t ctext[],
                                                              size_t ctext_len,
                                                              const uint8_t nonce[],
                                                              const uint8_t key[])
   {
   return crypto_secretbox_xsalsa20poly1305_open(ptext, ctext, ctext_len, nonce, key);
   }

// sodium/crypto_box.h

inline size_t crypto_box_seedbytes()
   {
   return crypto_box_SEEDBYTES;
   }

inline size_t crypto_box_publickeybytes()
   {
   return crypto_box_PUBLICKEYBYTES;
   }

inline size_t crypto_box_secretkeybytes()
   {
   return crypto_box_SECRETKEYBYTES;
   }

inline size_t crypto_box_noncebytes()
   {
   return crypto_box_NONCEBYTES;
   }

inline size_t crypto_box_macbytes()
   {
   return crypto_box_MACBYTES;
   }

inline size_t crypto_box_messagebytes_max()
   {
   return crypto_box_MESSAGEBYTES_MAX;
   }

inline size_t crypto_box_beforenmbytes()
   {
   return crypto_box_BEFORENMBYTES;
   }

inline const char* crypto_box_primitive() { return "curve25519xsalsa20poly1305"; }

inline int crypto_box_seed_keypair(uint8_t pk[32], uint8_t sk[32],
                                   const uint8_t seed[])
   {
   return crypto_box_curve25519xsalsa20poly1305_seed_keypair(pk, sk, seed);
   }

inline int crypto_box_keypair(uint8_t pk[32], uint8_t sk[32])
   {
   return crypto_box_curve25519xsalsa20poly1305_keypair(pk, sk);
   }

BOTAN_PUBLIC_API(2,11)
int crypto_box_detached(uint8_t ctext[], uint8_t mac[],
                        const uint8_t ptext[], size_t ptext_len,
                        const uint8_t nonce[], const uint8_t pk[32],
                        const uint8_t sk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_box_open_detached(uint8_t ptext[], const uint8_t ctext[],
                             const uint8_t mac[],
                             size_t ctext_len,
                             const uint8_t nonce[],
                             const uint8_t pk[32],
                             const uint8_t sk[32]);

inline int crypto_box_easy(uint8_t ctext[], const uint8_t ptext[],
                           size_t ptext_len, const uint8_t nonce[],
                           const uint8_t pk[32], const uint8_t sk[32])
   {
   return crypto_box_detached(ctext + crypto_box_MACBYTES, ctext, ptext, ptext_len, nonce, pk, sk);
   }

inline int crypto_box_open_easy(uint8_t ptext[], const uint8_t ctext[],
                                size_t ctext_len, const uint8_t nonce[],
                                const uint8_t pk[32], const uint8_t sk[32])
   {
   if(ctext_len < crypto_box_MACBYTES)
      {
      return -1;
      }

   return crypto_box_open_detached(ptext, ctext + crypto_box_MACBYTES,
                                   ctext, ctext_len - crypto_box_MACBYTES,
                                   nonce, pk, sk);
   }

inline int crypto_box_beforenm(uint8_t key[], const uint8_t pk[32],
                               const uint8_t sk[32])
   {
   return crypto_box_curve25519xsalsa20poly1305_beforenm(key, pk, sk);
   }

inline int crypto_box_afternm(uint8_t ctext[], const uint8_t ptext[],
                              size_t ptext_len, const uint8_t nonce[],
                              const uint8_t key[])
   {
   return crypto_box_curve25519xsalsa20poly1305_afternm(ctext, ptext, ptext_len, nonce, key);
   }

inline int crypto_box_open_afternm(uint8_t ptext[], const uint8_t ctext[],
                                   size_t ctext_len, const uint8_t nonce[],
                                   const uint8_t key[])
   {
   return crypto_box_curve25519xsalsa20poly1305_open_afternm(ptext, ctext, ctext_len, nonce, key);
   }

inline int crypto_box_open_detached_afternm(uint8_t ptext[], const uint8_t ctext[],
                                            const uint8_t mac[],
                                            size_t ctext_len, const uint8_t nonce[],
                                            const uint8_t key[])
   {
   return crypto_secretbox_open_detached(ptext, ctext, mac, ctext_len, nonce, key);
   }

inline int crypto_box_open_easy_afternm(uint8_t ptext[], const uint8_t ctext[],
                                        size_t ctext_len, const uint8_t nonce[],
                                        const uint8_t key[])
   {
   if(ctext_len < crypto_box_MACBYTES)
      {
      return -1;
      }

   return crypto_box_open_detached_afternm(ptext, ctext + crypto_box_MACBYTES,
                                           ctext, ctext_len - crypto_box_MACBYTES,
                                           nonce, key);
   }

inline int crypto_box_detached_afternm(uint8_t ctext[], uint8_t mac[],
                                       const uint8_t ptext[], size_t ptext_len,
                                       const uint8_t nonce[], const uint8_t key[])
   {
   return crypto_secretbox_detached(ctext, mac, ptext, ptext_len, nonce, key);
   }

inline int crypto_box_easy_afternm(uint8_t ctext[], const uint8_t ptext[],
                                   size_t ptext_len, const uint8_t nonce[],
                                   const uint8_t key[])
   {
   return crypto_box_detached_afternm(ctext + crypto_box_MACBYTES, ctext, ptext, ptext_len, nonce, key);
   }

inline size_t crypto_box_zerobytes() { return crypto_box_ZEROBYTES; }

inline size_t crypto_box_boxzerobytes() { return crypto_box_BOXZEROBYTES; }

inline int crypto_box(uint8_t ctext[], const uint8_t ptext[],
                      size_t ptext_len, const uint8_t nonce[],
                      const uint8_t pk[32], const uint8_t sk[32])
   {
   return crypto_box_curve25519xsalsa20poly1305(ctext, ptext, ptext_len, nonce, pk, sk);
   }

inline int crypto_box_open(uint8_t ptext[], const uint8_t ctext[],
                           size_t ctext_len, const uint8_t nonce[],
                           const uint8_t pk[32], const uint8_t sk[32])
   {
   return crypto_box_curve25519xsalsa20poly1305_open(ptext, ctext, ctext_len, nonce, pk, sk);
   }

// sodium/crypto_hash_sha512.h

inline size_t crypto_hash_sha512_bytes() { return crypto_hash_sha512_BYTES; }

BOTAN_PUBLIC_API(2,11)
int crypto_hash_sha512(uint8_t out[64], const uint8_t in[], size_t in_len);

// sodium/crypto_auth_hmacsha512.h

inline size_t crypto_auth_hmacsha512_bytes() { return crypto_auth_hmacsha512_BYTES; }

inline size_t crypto_auth_hmacsha512_keybytes() { return crypto_auth_hmacsha512_KEYBYTES; }

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha512(uint8_t out[],
                           const uint8_t in[],
                           size_t in_len,
                           const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha512_verify(const uint8_t h[],
                                  const uint8_t in[],
                                  size_t in_len,
                                  const uint8_t key[]);

inline void crypto_auth_hmacsha512_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_auth_hmacsha512256.h

inline size_t crypto_auth_hmacsha512256_bytes()
   {
   return crypto_auth_hmacsha512256_BYTES;
   }

inline size_t crypto_auth_hmacsha512256_keybytes()
   {
   return crypto_auth_hmacsha512256_KEYBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha512256(uint8_t out[],
                              const uint8_t in[],
                              size_t in_len,
                              const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha512256_verify(const uint8_t h[],
                                     const uint8_t in[],
                                     size_t in_len,
                                     const uint8_t key[]);

inline void crypto_auth_hmacsha512256_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_auth.h

inline size_t crypto_auth_bytes() { return crypto_auth_BYTES; }

inline size_t crypto_auth_keybytes() { return crypto_auth_KEYBYTES; }

inline const char* crypto_auth_primitive() { return "hmacsha512256"; }

inline int crypto_auth(uint8_t out[], const uint8_t in[],
                       size_t in_len, const uint8_t key[])
   {
   return crypto_auth_hmacsha512256(out, in, in_len, key);
   }

inline int crypto_auth_verify(const uint8_t mac[], const uint8_t in[],
                              size_t in_len, const uint8_t key[])
   {
   return crypto_auth_hmacsha512256_verify(mac, in, in_len, key);
   }

inline void crypto_auth_keygen(uint8_t k[])
   {
   return randombytes_buf(k, crypto_auth_KEYBYTES);
   }

// sodium/crypto_hash_sha256.h

inline size_t crypto_hash_sha256_bytes()
   {
   return crypto_hash_sha256_BYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_hash_sha256(uint8_t out[], const uint8_t in[], size_t in_len);

// sodium/crypto_auth_hmacsha256.h

inline size_t crypto_auth_hmacsha256_bytes()
   {
   return crypto_auth_hmacsha256_BYTES;
   }

inline size_t crypto_auth_hmacsha256_keybytes()
   {
   return crypto_auth_hmacsha256_KEYBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha256(uint8_t out[],
                           const uint8_t in[],
                           size_t in_len,
                           const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_auth_hmacsha256_verify(const uint8_t h[],
                                  const uint8_t in[],
                                  size_t in_len,
                                  const uint8_t key[]);

inline void crypto_auth_hmacsha256_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_stream_xsalsa20.h

inline size_t crypto_stream_xsalsa20_keybytes()
   {
   return crypto_stream_xsalsa20_KEYBYTES;
   }

inline size_t crypto_stream_xsalsa20_noncebytes()
   {
   return crypto_stream_xsalsa20_NONCEBYTES;
   }

inline size_t crypto_stream_xsalsa20_messagebytes_max()
   {
   return crypto_stream_xsalsa20_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xsalsa20(uint8_t out[], size_t ctext_len,
                           const uint8_t nonce[], const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xsalsa20_xor(uint8_t out[], const uint8_t ptext[],
                               size_t ptext_len, const uint8_t nonce[],
                               const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xsalsa20_xor_ic(uint8_t out[], const uint8_t ptext[],
                                  size_t ptext_len,
                                  const uint8_t nonce[], uint64_t ic,
                                  const uint8_t key[]);

inline void crypto_stream_xsalsa20_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_core_hsalsa20.h

inline size_t crypto_core_hsalsa20_outputbytes()
   {
   return crypto_core_hsalsa20_OUTPUTBYTES;
   }

inline size_t crypto_core_hsalsa20_inputbytes()
   {
   return crypto_core_hsalsa20_INPUTBYTES;
   }

inline size_t crypto_core_hsalsa20_keybytes()
   {
   return crypto_core_hsalsa20_KEYBYTES;
   }

inline size_t crypto_core_hsalsa20_constbytes()
   {
   return crypto_core_hsalsa20_CONSTBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_core_hsalsa20(uint8_t out[], const uint8_t in[],
                         const uint8_t key[], const uint8_t c[]);

// sodium/crypto_hash.h

inline size_t crypto_hash_bytes()
   {
   return crypto_hash_BYTES;
   }

inline int crypto_hash(uint8_t out[], const uint8_t in[], size_t in_len)
   {
   return crypto_hash_sha512(out, in, in_len);
   }

inline const char* crypto_hash_primitive() { return "sha512"; }

// sodium/crypto_onetimeauth_poly1305.h

inline size_t crypto_onetimeauth_poly1305_bytes()
   {
   return crypto_onetimeauth_poly1305_BYTES;
   }

inline size_t crypto_onetimeauth_poly1305_keybytes()
   {
   return crypto_onetimeauth_poly1305_KEYBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_onetimeauth_poly1305(uint8_t out[],
                                const uint8_t in[],
                                size_t in_len,
                                const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_onetimeauth_poly1305_verify(const uint8_t h[],
                                       const uint8_t in[],
                                       size_t in_len,
                                       const uint8_t key[]);

inline void crypto_onetimeauth_poly1305_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_onetimeauth.h

inline size_t crypto_onetimeauth_bytes() { return crypto_onetimeauth_BYTES; }

inline size_t crypto_onetimeauth_keybytes() { return crypto_onetimeauth_KEYBYTES; }

inline const char* crypto_onetimeauth_primitive() { return "poly1305"; }

inline int crypto_onetimeauth(uint8_t out[], const uint8_t in[],
                              size_t in_len, const uint8_t key[])
   {
   return crypto_onetimeauth_poly1305(out, in, in_len, key);
   }

inline int crypto_onetimeauth_verify(const uint8_t h[], const uint8_t in[],
                                     size_t in_len, const uint8_t key[])
   {
   return crypto_onetimeauth_poly1305_verify(h, in, in_len, key);
   }

inline void crypto_onetimeauth_keygen(uint8_t k[32])
   {
   return crypto_onetimeauth_poly1305_keygen(k);
   }

// sodium/crypto_scalarmult_curve25519.h

inline size_t crypto_scalarmult_curve25519_bytes()
   {
   return crypto_scalarmult_curve25519_BYTES;
   }

inline size_t crypto_scalarmult_curve25519_scalarbytes()
   {
   return crypto_scalarmult_curve25519_SCALARBYTES;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_scalarmult_curve25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t basepoint[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_scalarmult_curve25519_base(uint8_t out[32], const uint8_t scalar[32]);

// sodium/crypto_scalarmult.h

inline size_t crypto_scalarmult_bytes() { return crypto_scalarmult_curve25519_bytes(); }

inline size_t crypto_scalarmult_scalarbytes() { return crypto_scalarmult_curve25519_scalarbytes(); }

inline const char* crypto_scalarmult_primitive() { return "curve25519"; }

inline int crypto_scalarmult_base(uint8_t out[], const uint8_t scalar[])
   {
   return crypto_scalarmult_curve25519_base(out, scalar);
   }

inline int crypto_scalarmult(uint8_t out[], const uint8_t scalar[], const uint8_t base[])
   {
   return crypto_scalarmult_curve25519(out, scalar, base);
   }

// sodium/crypto_stream_chacha20.h

inline size_t crypto_stream_chacha20_keybytes()
   {
   return crypto_stream_chacha20_KEYBYTES;
   }

inline size_t crypto_stream_chacha20_noncebytes()
   {
   return crypto_stream_chacha20_NONCEBYTES;
   }

inline size_t crypto_stream_chacha20_messagebytes_max()
   {
   return crypto_stream_chacha20_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20(uint8_t out[], size_t ctext_len,
                           const uint8_t nonce[], const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20_xor(uint8_t out[], const uint8_t ptext[],
                               size_t ptext_len, const uint8_t nonce[],
                               const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20_xor_ic(uint8_t out[], const uint8_t ptext[],
                                  size_t ptext_len,
                                  const uint8_t nonce[], uint64_t ic,
                                  const uint8_t key[]);

inline void crypto_stream_chacha20_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

inline size_t crypto_stream_chacha20_ietf_keybytes()
   {
   return crypto_stream_chacha20_ietf_KEYBYTES;
   }

inline size_t crypto_stream_chacha20_ietf_noncebytes()
   {
   return crypto_stream_chacha20_ietf_NONCEBYTES;
   }

inline size_t crypto_stream_chacha20_ietf_messagebytes_max()
   {
   return crypto_stream_chacha20_ietf_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20_ietf(uint8_t out[], size_t ctext_len,
                                const uint8_t nonce[], const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20_ietf_xor(uint8_t out[], const uint8_t ptext[],
                                    size_t ptext_len, const uint8_t nonce[],
                                    const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_chacha20_ietf_xor_ic(uint8_t out[], const uint8_t ptext[],
                                       size_t ptext_len,
                                       const uint8_t nonce[], uint32_t ic,
                                       const uint8_t key[]);

inline void crypto_stream_chacha20_ietf_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_stream_xchacha20.h

inline size_t crypto_stream_xchacha20_keybytes()
   {
   return crypto_stream_xchacha20_KEYBYTES;
   }

inline size_t crypto_stream_xchacha20_noncebytes()
   {
   return crypto_stream_xchacha20_NONCEBYTES;
   }

inline size_t crypto_stream_xchacha20_messagebytes_max()
   {
   return crypto_stream_xchacha20_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xchacha20(uint8_t out[], size_t ctext_len,
                            const uint8_t nonce[], const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xchacha20_xor(uint8_t out[], const uint8_t ptext[],
                                size_t ptext_len, const uint8_t nonce[],
                                const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_xchacha20_xor_ic(uint8_t out[], const uint8_t ptext[],
                                   size_t ptext_len,
                                   const uint8_t nonce[], uint64_t ic,
                                   const uint8_t key[]);

inline void crypto_stream_xchacha20_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, crypto_stream_xchacha20_KEYBYTES);
   }

// sodium/crypto_stream_salsa20.h

inline size_t crypto_stream_salsa20_keybytes()
   {
   return crypto_stream_xsalsa20_KEYBYTES;
   }

inline size_t crypto_stream_salsa20_noncebytes()
   {
   return crypto_stream_salsa20_NONCEBYTES;
   }

inline size_t crypto_stream_salsa20_messagebytes_max()
   {
   return crypto_stream_salsa20_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_stream_salsa20(uint8_t out[], size_t ctext_len,
                          const uint8_t nonce[], const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_salsa20_xor(uint8_t out[], const uint8_t ptext[],
                              size_t ptext_len, const uint8_t nonce[],
                              const uint8_t key[]);

BOTAN_PUBLIC_API(2,11)
int crypto_stream_salsa20_xor_ic(uint8_t out[], const uint8_t ptext[],
                                 size_t ptext_len,
                                 const uint8_t nonce[], uint64_t ic,
                                 const uint8_t key[]);

inline void crypto_stream_salsa20_keygen(uint8_t k[32])
   {
   return randombytes_buf(k, 32);
   }

// sodium/crypto_stream.h

inline size_t crypto_stream_keybytes() { return crypto_stream_xsalsa20_keybytes(); }

inline size_t crypto_stream_noncebytes() { return crypto_stream_xsalsa20_noncebytes(); }

inline size_t crypto_stream_messagebytes_max() { return crypto_stream_MESSAGEBYTES_MAX; }

inline const char* crypto_stream_primitive() { return "xsalsa20"; }

inline int crypto_stream(uint8_t out[], size_t out_len,
                         const uint8_t nonce[24], const uint8_t key[32])
   {
   return crypto_stream_xsalsa20(out, out_len, nonce, key);
   }

inline int crypto_stream_xor(uint8_t out[], const uint8_t in[], size_t in_len,
                             const uint8_t nonce[24], const uint8_t key[32])
   {
   return crypto_stream_xsalsa20_xor(out, in, in_len, nonce, key);
   }

inline void crypto_stream_keygen(uint8_t key[32])
   {
   return randombytes_buf(key, 32);
   }

// sodium/crypto_shorthash_siphash24.h

inline size_t crypto_shorthash_siphash24_bytes() { return crypto_shorthash_siphash24_BYTES; }

inline size_t crypto_shorthash_siphash24_keybytes() { return crypto_shorthash_siphash24_KEYBYTES; }

BOTAN_PUBLIC_API(2,11)
int crypto_shorthash_siphash24(uint8_t out[8], const uint8_t in[], size_t in_len, const uint8_t key[16]);

// sodium/crypto_shorthash.h

inline size_t crypto_shorthash_bytes() { return crypto_shorthash_siphash24_bytes(); }

inline size_t crypto_shorthash_keybytes() { return crypto_shorthash_siphash24_keybytes(); }

inline const char* crypto_shorthash_primitive() { return "siphash24"; }

inline int crypto_shorthash(uint8_t out[], const uint8_t in[],
                            size_t in_len, const uint8_t k[16])
   {
   return crypto_shorthash_siphash24(out, in, in_len, k);
   }

inline void crypto_shorthash_keygen(uint8_t k[16])
   {
   randombytes_buf(k, crypto_shorthash_siphash24_KEYBYTES);
   }

// sodium/crypto_sign_ed25519.h

inline size_t crypto_sign_ed25519_bytes()
   {
   return crypto_sign_ed25519_BYTES;
   }

inline size_t crypto_sign_ed25519_seedbytes()
   {
   return crypto_sign_ed25519_SEEDBYTES;
   }

inline size_t crypto_sign_ed25519_publickeybytes()
   {
   return crypto_sign_ed25519_PUBLICKEYBYTES;
   }

inline size_t crypto_sign_ed25519_secretkeybytes()
   {
   return crypto_sign_ed25519_SECRETKEYBYTES;
   }

inline size_t crypto_sign_ed25519_messagebytes_max()
   {
   return crypto_sign_ed25519_MESSAGEBYTES_MAX;
   }

BOTAN_PUBLIC_API(2,11)
int crypto_sign_ed25519_detached(uint8_t sig[],
                                 unsigned long long* sig_len,
                                 const uint8_t msg[],
                                 size_t msg_len,
                                 const uint8_t sk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_sign_ed25519_verify_detached(const uint8_t sig[],
                                        const uint8_t msg[],
                                        size_t msg_len,
                                        const uint8_t pk[32]);

BOTAN_PUBLIC_API(2,11)
int crypto_sign_ed25519_keypair(uint8_t pk[32], uint8_t sk[64]);

BOTAN_PUBLIC_API(2,11)
int crypto_sign_ed25519_seed_keypair(uint8_t pk[], uint8_t sk[],
                                     const uint8_t seed[]);

// sodium/crypto_sign.h

inline size_t crypto_sign_bytes()
   {
   return crypto_sign_BYTES;
   }

inline size_t crypto_sign_seedbytes()
   {
   return crypto_sign_SEEDBYTES;
   }

inline size_t crypto_sign_publickeybytes()
   {
   return crypto_sign_PUBLICKEYBYTES;
   }

inline size_t crypto_sign_secretkeybytes()
   {
   return crypto_sign_SECRETKEYBYTES;
   }

inline size_t crypto_sign_messagebytes_max()
   {
   return crypto_sign_MESSAGEBYTES_MAX;
   }

inline const char* crypto_sign_primitive()
   {
   return "ed25519";
   }

inline int crypto_sign_seed_keypair(uint8_t pk[32], uint8_t sk[32],
                                    const uint8_t seed[])
   {
   return crypto_sign_ed25519_seed_keypair(pk, sk, seed);
   }

inline int crypto_sign_keypair(uint8_t pk[32], uint8_t sk[32])
   {
   return crypto_sign_ed25519_keypair(pk, sk);
   }

inline int crypto_sign_detached(uint8_t sig[], unsigned long long* sig_len,
                                const uint8_t msg[], size_t msg_len,
                                const uint8_t sk[32])
   {
   return crypto_sign_ed25519_detached(sig, sig_len, msg, msg_len, sk);
   }

inline int crypto_sign_verify_detached(const uint8_t sig[],
                                       const uint8_t in[],
                                       size_t in_len,
                                       const uint8_t pk[32])
   {
   return crypto_sign_ed25519_verify_detached(sig, in, in_len, pk);
   }

}

}

#endif
