/* Adapted from OpenSSL's rsa_pss.c from OpenSSL 3.0.1 */

/*
 * Copyright 2005-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "jni.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include <array>

static const unsigned char zeroes[] = {0, 0, 0, 0, 0, 0, 0, 0};

static char opensslerr[1024];
extern "C" jbyteArray Java_de_blinkt_openvpn_core_NativeUtils_rsapss(JNIEnv *env,
								     jclass,
								     jint hashtype,
								     jint MSBits,
								     jint rsa_size,
								     jbyteArray from) {

  /*
  unsigned char *EM,
				     const unsigned char *mHash,
				     const EVP_MD *Hash, const EVP_MD *mgf1Hash,
				     int sLen)
*/

  jbyte *data = env->GetByteArrayElements(from, nullptr);
  int datalen = env->GetArrayLength(from);

  const auto *mHash = reinterpret_cast<const unsigned char *>(data);

  const EVP_MD *Hash;

  if (hashtype == 0) {
    Hash = EVP_md5();
  } else if (hashtype == 1) {
    Hash = EVP_sha1();
  } else if (hashtype == 2) {
    Hash = EVP_sha224();
  } else if (hashtype == 3) {
    Hash = EVP_sha256();
  } else if (hashtype == 4) {
    Hash = EVP_sha384();
  } else if (hashtype == 5) {
    Hash = EVP_sha512();
  }

  const EVP_MD *mgf1Hash = Hash;

  int ret = 0;
  int maskedDBLen, emLen;
  unsigned char *H, *salt = nullptr, *p;
  EVP_MD_CTX *ctx = nullptr;

  int hLen = EVP_MD_get_size(Hash);
  int sLen = hLen; /* RSA_PSS_SALTLEN_DIGEST */

  std::array<unsigned char, 2048> buf{};
  unsigned char *EM = buf.data();

  if (hLen < 0)
    goto err;

  emLen = rsa_size;
  if (MSBits == 0) {
    *EM++ = 0;
    emLen--;
  }
  if (emLen < hLen + 2) {
    goto err;
  }
  if (sLen == RSA_PSS_SALTLEN_MAX) {
    sLen = emLen - hLen - 2;
  } else if (sLen > emLen - hLen - 2) {
    goto err;
  }

  if (sLen > 0) {
    salt = (unsigned char *) OPENSSL_malloc(sLen);
    if (salt == nullptr) {
      goto err;
    }
    if (RAND_bytes_ex(nullptr, salt, sLen, 0) <= 0)
      goto err;
  }
  maskedDBLen = emLen - hLen - 1;
  H = EM + maskedDBLen;
  ctx = EVP_MD_CTX_new();
  if (ctx == nullptr)
    goto err;
  if (!EVP_DigestInit_ex(ctx, Hash, nullptr)
      || !EVP_DigestUpdate(ctx, zeroes, sizeof(zeroes))
      || !EVP_DigestUpdate(ctx, mHash, hLen))
    goto err;
  if (sLen && !EVP_DigestUpdate(ctx, salt, sLen))
    goto err;
  if (!EVP_DigestFinal_ex(ctx, H, nullptr))
    goto err;

  /* Generate dbMask in place then perform XOR on it */
  if (PKCS1_MGF1(EM, maskedDBLen, H, hLen, mgf1Hash))
    goto err;

  p = EM;

  /*
   * Initial PS XORs with all zeroes which is a NOP so just update pointer.
   * Note from a test above this value is guaranteed to be non-negative.
   */
  p += emLen - sLen - hLen - 2;
  *p++ ^= 0x1;
  if (sLen > 0) {
    for (int i = 0; i < sLen; i++)
      *p++ ^= salt[i];
  }
  if (MSBits)
    EM[0] &= 0xFF >> (8 - MSBits);

  /* H is already in place so just set final 0xbc */

  EM[emLen - 1] = 0xbc;

  ret = 1;

  err:
  EVP_MD_CTX_free(ctx);
  OPENSSL_clear_free(salt, (size_t) sLen); /* salt != NULL implies sLen > 0 */


  jbyteArray jb;

  jb = env->NewByteArray(emLen);

  env->SetByteArrayRegion(jb, 0, emLen, (jbyte *) EM);

  return jb;
}