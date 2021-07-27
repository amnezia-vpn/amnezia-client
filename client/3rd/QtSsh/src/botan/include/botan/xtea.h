/*
* XTEA
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_XTEA_H_
#define BOTAN_XTEA_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(xtea.h)

namespace Botan {

/**
* XTEA
*/
class BOTAN_PUBLIC_API(2,0) XTEA final : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "XTEA"; }
      BlockCipher* clone() const override { return new XTEA; }

   private:
      void key_schedule(const uint8_t[], size_t) override;
      secure_vector<uint32_t> m_EK;
   };

}

#endif
