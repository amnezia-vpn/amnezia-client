/*
* SM4
* (C) 2017 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SM4_H_
#define BOTAN_SM4_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(sm4.h)

namespace Botan {

/**
* SM4
*/
class BOTAN_PUBLIC_API(2,2) SM4 final : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "SM4"; }
      BlockCipher* clone() const override { return new SM4; }

      std::string provider() const override;
      size_t parallelism() const override;
   private:
      void key_schedule(const uint8_t[], size_t) override;

#if defined(BOTAN_HAS_SM4_ARMV8)
      void sm4_armv8_encrypt(const uint8_t in[], uint8_t out[], size_t blocks) const;
      void sm4_armv8_decrypt(const uint8_t in[], uint8_t out[], size_t blocks) const;
#endif

      secure_vector<uint32_t> m_RK;
   };

}

#endif
