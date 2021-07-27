/*
* HKDF
* (C) 2013,2015 Jack Lloyd
* (C) 2016 René Korthaus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_HKDF_H_
#define BOTAN_HKDF_H_

#include <botan/mac.h>
#include <botan/kdf.h>

/*
* The definitions of HKDF, HKDF_Extract, HKDF_Expand will be made internal
* in the future. However the function hkdf_expand_label will still be defined.
*/
//BOTAN_FUTURE_INTERNAL_HEADER(hkdf.h)

namespace Botan {

/**
* HKDF from RFC 5869.
*/
class BOTAN_PUBLIC_API(2,0) HKDF final : public KDF
   {
   public:
      /**
      * @param prf MAC algorithm to use
      */
      explicit HKDF(MessageAuthenticationCode* prf) : m_prf(prf) {}

      KDF* clone() const override { return new HKDF(m_prf->clone()); }

      std::string name() const override { return "HKDF(" + m_prf->name() + ")"; }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
* HKDF Extraction Step from RFC 5869.
*/
class BOTAN_PUBLIC_API(2,0) HKDF_Extract final : public KDF
   {
   public:
      /**
      * @param prf MAC algorithm to use
      */
      explicit HKDF_Extract(MessageAuthenticationCode* prf) : m_prf(prf) {}

      KDF* clone() const override { return new HKDF_Extract(m_prf->clone()); }

      std::string name() const override { return "HKDF-Extract(" + m_prf->name() + ")"; }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
* HKDF Expansion Step from RFC 5869.
*/
class BOTAN_PUBLIC_API(2,0) HKDF_Expand final : public KDF
   {
   public:
      /**
      * @param prf MAC algorithm to use
      */
      explicit HKDF_Expand(MessageAuthenticationCode* prf) : m_prf(prf) {}

      KDF* clone() const override { return new HKDF_Expand(m_prf->clone()); }

      std::string name() const override { return "HKDF-Expand(" + m_prf->name() + ")"; }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
* HKDF-Expand-Label from TLS 1.3/QUIC
* @param hash_fn the hash to use
* @param secret the secret bits
* @param secret_len the length of secret
* @param label the full label (no "TLS 1.3, " or "tls13 " prefix
*  is applied)
* @param hash_val the previous hash value (used for chaining, may be empty)
* @param hash_val_len the length of hash_val
* @param length the desired output length
*/
secure_vector<uint8_t>
BOTAN_PUBLIC_API(2,3) hkdf_expand_label(
   const std::string& hash_fn,
   const uint8_t secret[], size_t secret_len,
   const std::string& label,
   const uint8_t hash_val[], size_t hash_val_len,
   size_t length);


}

#endif
