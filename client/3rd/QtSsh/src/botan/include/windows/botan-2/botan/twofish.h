/*
* Twofish
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TWOFISH_H_
#define BOTAN_TWOFISH_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(twofish.h)

namespace Botan {

/**
* Twofish, an AES finalist
*/
class BOTAN_PUBLIC_API(2,0) Twofish final : public Block_Cipher_Fixed_Params<16, 16, 32, 8>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "Twofish"; }
      BlockCipher* clone() const override { return new Twofish; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      static const uint32_t MDS0[256];
      static const uint32_t MDS1[256];
      static const uint32_t MDS2[256];
      static const uint32_t MDS3[256];
      static const uint8_t Q0[256];
      static const uint8_t Q1[256];
      static const uint8_t RS[32];
      static const uint8_t EXP_TO_POLY[255];
      static const uint8_t POLY_TO_EXP[255];

      secure_vector<uint32_t> m_SB, m_RK;
   };

}

#endif
