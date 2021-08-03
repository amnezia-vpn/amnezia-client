/*
* KDFs defined in NIST SP 800-108
* (C) 2016 Kai Michaelis
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SP800_108_H_
#define BOTAN_SP800_108_H_

#include <botan/kdf.h>
#include <botan/mac.h>

BOTAN_FUTURE_INTERNAL_HEADER(sp800_108.h)

namespace Botan {

/**
 * NIST SP 800-108 KDF in Counter Mode (5.1)
 */
class BOTAN_PUBLIC_API(2,0) SP800_108_Counter final : public KDF
   {
   public:
      std::string name() const override { return "SP800-108-Counter(" + m_prf->name() + ")"; }

      KDF* clone() const override { return new SP800_108_Counter(m_prf->clone()); }

      /**
      * Derive a key using the SP800-108 KDF in Counter mode.
      *
      * The implementation hard codes the length of [L]_2
      * and [i]_2 (the value r) to 32 bits.
      *
      * @param key resulting keying material
      * @param key_len the desired output length in bytes
      * @param secret K_I
      * @param secret_len size of K_I in bytes
      * @param salt Context
      * @param salt_len size of Context in bytes
      * @param label Label
      * @param label_len size of Label in bytes
      *
      * @throws Invalid_Argument key_len > 2^32
      */
      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param mac MAC algorithm to use
      */
      explicit SP800_108_Counter(MessageAuthenticationCode* mac) : m_prf(mac) {}
   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
 * NIST SP 800-108 KDF in Feedback Mode (5.2)
 */
class BOTAN_PUBLIC_API(2,0) SP800_108_Feedback final : public KDF
   {
   public:
      std::string name() const override { return "SP800-108-Feedback(" + m_prf->name() + ")"; }

      KDF* clone() const override { return new SP800_108_Feedback(m_prf->clone()); }

      /**
      * Derive a key using the SP800-108 KDF in Feedback mode.
      *
      * The implementation uses the optional counter i and hard
      * codes the length of [L]_2 and [i]_2 (the value r) to 32 bits.
      *
      * @param key resulting keying material
      * @param key_len the desired output length in bytes
      * @param secret K_I
      * @param secret_len size of K_I in bytes
      * @param salt IV || Context
      * @param salt_len size of Context plus IV in bytes
      * @param label Label
      * @param label_len size of Label in bytes
      *
      * @throws Invalid_Argument key_len > 2^32
      */
      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      explicit SP800_108_Feedback(MessageAuthenticationCode* mac) : m_prf(mac) {}
   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
 * NIST SP 800-108 KDF in Double Pipeline Mode (5.3)
 */
class BOTAN_PUBLIC_API(2,0) SP800_108_Pipeline final : public KDF
   {
   public:
      std::string name() const override { return "SP800-108-Pipeline(" + m_prf->name() + ")"; }

      KDF* clone() const override { return new SP800_108_Pipeline(m_prf->clone()); }

      /**
      * Derive a key using the SP800-108 KDF in Double Pipeline mode.
      *
      * The implementation uses the optional counter i and hard
      * codes the length of [L]_2 and [i]_2 (the value r) to 32 bits.
      *
      * @param key resulting keying material
      * @param key_len the desired output length in bytes
      * @param secret K_I
      * @param secret_len size of K_I in bytes
      * @param salt Context
      * @param salt_len size of Context in bytes
      * @param label Label
      * @param label_len size of Label in bytes
      *
      * @throws Invalid_Argument key_len > 2^32
      */
      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      explicit SP800_108_Pipeline(MessageAuthenticationCode* mac) : m_prf(mac) {}

   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

}

#endif
