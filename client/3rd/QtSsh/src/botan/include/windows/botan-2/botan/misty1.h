/*
* MISTY1
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MISTY1_H_
#define BOTAN_MISTY1_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(misty1.h)

namespace Botan {

/**
* MISTY1 with 8 rounds
*/
class BOTAN_PUBLIC_API(2,0) MISTY1 final : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "MISTY1"; }
      BlockCipher* clone() const override { return new MISTY1; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint16_t> m_EK, m_DK;
   };

}

#endif
