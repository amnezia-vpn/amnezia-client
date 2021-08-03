/*
* Threefish-512
* (C) 2013,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_THREEFISH_512_H_
#define BOTAN_THREEFISH_512_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(threefish_512.h)

namespace Botan {

/**
* Threefish-512
*/
class BOTAN_PUBLIC_API(2,0) Threefish_512 final :
   public Block_Cipher_Fixed_Params<64, 64, 0, 1, Tweakable_Block_Cipher>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void set_tweak(const uint8_t tweak[], size_t len) override;

      void clear() override;
      std::string provider() const override;
      std::string name() const override { return "Threefish-512"; }
      BlockCipher* clone() const override { return new Threefish_512; }
      size_t parallelism() const override;

   private:

#if defined(BOTAN_HAS_THREEFISH_512_AVX2)
      void avx2_encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const;
      void avx2_decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const;
#endif

      void key_schedule(const uint8_t key[], size_t key_len) override;

      // Interface for Skein
      friend class Skein_512;

      void skein_feedfwd(const secure_vector<uint64_t>& M,
                         const secure_vector<uint64_t>& T);

      // Private data
      secure_vector<uint64_t> m_T;
      secure_vector<uint64_t> m_K;
   };

}

#endif
