/*
* PBKDF1
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PBKDF1_H_
#define BOTAN_PBKDF1_H_

#include <botan/pbkdf.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(pbkdf1.h)

namespace Botan {

/**
* PKCS #5 v1 PBKDF, aka PBKDF1
* Can only generate a key up to the size of the hash output.
* Unless needed for backwards compatibility, use PKCS5_PBKDF2
*/
class BOTAN_PUBLIC_API(2,0) PKCS5_PBKDF1 final : public PBKDF
   {
   public:
      /**
      * Create a PKCS #5 instance using the specified hash function.
      * @param hash pointer to a hash function object to use
      */
      explicit PKCS5_PBKDF1(HashFunction* hash) : m_hash(hash) {}

      std::string name() const override
         {
         return "PBKDF1(" + m_hash->name() + ")";
         }

      PBKDF* clone() const override
         {
         return new PKCS5_PBKDF1(m_hash->clone());
         }

      size_t pbkdf(uint8_t output_buf[], size_t output_len,
                           const std::string& passphrase,
                           const uint8_t salt[], size_t salt_len,
                           size_t iterations,
                           std::chrono::milliseconds msec) const override;
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
