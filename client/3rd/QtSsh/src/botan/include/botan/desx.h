/*
* DESX
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DESX_H_
#define BOTAN_DESX_H_

#include <botan/des.h>

BOTAN_FUTURE_INTERNAL_HEADER(desx.h)

namespace Botan {

/**
* DESX
*/
class BOTAN_PUBLIC_API(2,0) DESX final : public Block_Cipher_Fixed_Params<8, 24>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "DESX"; }
      BlockCipher* clone() const override { return new DESX; }
   private:
      void key_schedule(const uint8_t[], size_t) override;
      secure_vector<uint8_t> m_K1, m_K2;
      DES m_des;
   };

}

#endif
