/*
* KDF1 from ISO 18033-2
* (C) 2016 Philipp Weber
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_KDF1_18033_H_
#define BOTAN_KDF1_18033_H_

#include <botan/kdf.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(kdf1_iso18033.h)

namespace Botan {

/**
* KDF1, from ISO 18033-2
*/
class BOTAN_PUBLIC_API(2,0) KDF1_18033 final : public KDF
   {
   public:
      std::string name() const override { return "KDF1-18033(" + m_hash->name() + ")"; }

      KDF* clone() const override { return new KDF1_18033(m_hash->clone()); }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      /**
      * @param h hash function to use
      */
      explicit KDF1_18033(HashFunction* h) : m_hash(h) {}
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
