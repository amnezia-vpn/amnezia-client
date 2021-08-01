/*
* KDF1
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_KDF1_H_
#define BOTAN_KDF1_H_

#include <botan/kdf.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(kdf1.h)

namespace Botan {

/**
* KDF1, from IEEE 1363
*/
class BOTAN_PUBLIC_API(2,0) KDF1 final : public KDF
   {
   public:
      std::string name() const override { return "KDF1(" + m_hash->name() + ")"; }

      KDF* clone() const override { return new KDF1(m_hash->clone()); }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param h hash function to use
      */
      explicit KDF1(HashFunction* h) : m_hash(h) {}
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
