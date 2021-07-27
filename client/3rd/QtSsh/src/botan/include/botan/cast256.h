/*
* CAST-256
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CAST256_H_
#define BOTAN_CAST256_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cast256.h)

namespace Botan {

/**
* CAST-256
*/
class BOTAN_PUBLIC_API(2,0) CAST_256 final : public Block_Cipher_Fixed_Params<16, 4, 32, 4>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "CAST-256"; }
      BlockCipher* clone() const override { return new CAST_256; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint32_t> m_MK;
      secure_vector<uint8_t> m_RK;
   };

}

#endif
