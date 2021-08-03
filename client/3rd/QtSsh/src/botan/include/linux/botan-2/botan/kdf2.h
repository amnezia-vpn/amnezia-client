/*
* KDF2
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_KDF2_H_
#define BOTAN_KDF2_H_

#include <botan/kdf.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(kdf2.h)

namespace Botan {

/**
* KDF2, from IEEE 1363
*/
class BOTAN_PUBLIC_API(2,0) KDF2 final : public KDF
   {
   public:
      std::string name() const override { return "KDF2(" + m_hash->name() + ")"; }

      KDF* clone() const override { return new KDF2(m_hash->clone()); }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param h hash function to use
      */
      explicit KDF2(HashFunction* h) : m_hash(h) {}
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
