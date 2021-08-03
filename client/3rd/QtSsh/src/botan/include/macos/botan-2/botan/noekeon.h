/*
* Noekeon
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_NOEKEON_H_
#define BOTAN_NOEKEON_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(noekeon.h)

namespace Botan {

/**
* Noekeon
*/
class BOTAN_PUBLIC_API(2,0) Noekeon final : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      std::string provider() const override;
      void clear() override;
      std::string name() const override { return "Noekeon"; }
      BlockCipher* clone() const override { return new Noekeon; }
      size_t parallelism() const override;

   private:
#if defined(BOTAN_HAS_NOEKEON_SIMD)
      void simd_encrypt_4(const uint8_t in[], uint8_t out[]) const;
      void simd_decrypt_4(const uint8_t in[], uint8_t out[]) const;
#endif

      /**
      * The Noekeon round constants
      */
      static const uint8_t RC[17];

      void key_schedule(const uint8_t[], size_t) override;
      secure_vector<uint32_t> m_EK, m_DK;
   };

}

#endif
