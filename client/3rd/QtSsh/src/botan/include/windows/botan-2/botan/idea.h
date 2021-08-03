/*
* IDEA
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_IDEA_H_
#define BOTAN_IDEA_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(idea.h)

namespace Botan {

/**
* IDEA
*/
class BOTAN_PUBLIC_API(2,0) IDEA final : public Block_Cipher_Fixed_Params<8, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;

      std::string provider() const override;
      std::string name() const override { return "IDEA"; }
      BlockCipher* clone() const override { return new IDEA; }
      size_t parallelism() const override;

   private:
#if defined(BOTAN_HAS_IDEA_SSE2)
      void sse2_idea_op_8(const uint8_t in[64], uint8_t out[64], const uint16_t EK[52]) const;
#endif

      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint16_t> m_EK, m_DK;
   };

}

#endif
