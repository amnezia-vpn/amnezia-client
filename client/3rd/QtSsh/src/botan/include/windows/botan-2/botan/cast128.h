/*
* CAST-128
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CAST128_H_
#define BOTAN_CAST128_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cast128.h)

namespace Botan {

/**
* CAST-128
*/
class BOTAN_PUBLIC_API(2,0) CAST_128 final : public Block_Cipher_Fixed_Params<8, 11, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "CAST-128"; }
      BlockCipher* clone() const override { return new CAST_128; }

   private:
      void key_schedule(const uint8_t[], size_t) override;

      static void cast_ks(secure_vector<uint32_t>& ks,
                          secure_vector<uint32_t>& user_key);

      secure_vector<uint32_t> m_MK;
      secure_vector<uint8_t> m_RK;
   };

}

#endif
