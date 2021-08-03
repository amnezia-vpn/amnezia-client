/*
* SEED
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SEED_H_
#define BOTAN_SEED_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(seed.h)

namespace Botan {

/**
* SEED, a Korean block cipher
*/
class BOTAN_PUBLIC_API(2,0) SEED final : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "SEED"; }
      BlockCipher* clone() const override { return new SEED; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint32_t> m_K;
   };

}

#endif
