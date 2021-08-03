/*
* KDF defined in NIST SP 800-56a revision 2 (Single-step key-derivation function)
*
* (C) 2017 Ribose Inc. Written by Krzysztof Kwiatkowski.
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SP800_56A_H_
#define BOTAN_SP800_56A_H_

#include <botan/kdf.h>
#include <botan/hash.h>
#include <botan/mac.h>

BOTAN_FUTURE_INTERNAL_HEADER(sp800_56a.h)

namespace Botan {

/**
 * NIST SP 800-56A KDF using hash function
 * @warning This KDF ignores the provided salt value
 */
class BOTAN_PUBLIC_API(2,2) SP800_56A_Hash final : public KDF
   {
   public:
      std::string name() const override { return "SP800-56A(" + m_hash->name() + ")"; }

      KDF* clone() const override { return new SP800_56A_Hash(m_hash->clone()); }

      /**
      * Derive a key using the SP800-56A KDF.
      *
      * The implementation hard codes the context value for the
      * expansion step to the empty string.
      *
      * @param key derived keying material K_M
      * @param key_len the desired output length in bytes
      * @param secret shared secret Z
      * @param secret_len size of Z in bytes
      * @param salt ignored
      * @param salt_len ignored
      * @param label label for the expansion step
      * @param label_len size of label in bytes
      *
      * @throws Invalid_Argument key_len > 2^32
      */
      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param hash the hash function to use as the auxiliary function
      */
      explicit SP800_56A_Hash(HashFunction* hash) : m_hash(hash) {}
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

/**
 * NIST SP 800-56A KDF using HMAC
 */
class BOTAN_PUBLIC_API(2,2) SP800_56A_HMAC final : public KDF
   {
   public:
      std::string name() const override { return "SP800-56A(" + m_mac->name() + ")"; }

      KDF* clone() const override { return new SP800_56A_HMAC(m_mac->clone()); }

      /**
      * Derive a key using the SP800-56A KDF.
      *
      * The implementation hard codes the context value for the
      * expansion step to the empty string.
      *
      * @param key derived keying material K_M
      * @param key_len the desired output length in bytes
      * @param secret shared secret Z
      * @param secret_len size of Z in bytes
      * @param salt ignored
      * @param salt_len ignored
      * @param label label for the expansion step
      * @param label_len size of label in bytes
      *
      * @throws Invalid_Argument key_len > 2^32 or MAC is not a HMAC
      */
      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param mac the HMAC to use as the auxiliary function
      */
      explicit SP800_56A_HMAC(MessageAuthenticationCode* mac);
   private:
      std::unique_ptr<MessageAuthenticationCode> m_mac;
   };

}

#endif
