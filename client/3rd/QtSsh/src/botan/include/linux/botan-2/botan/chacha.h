/*
* ChaCha20
* (C) 2014,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CHACHA_H_
#define BOTAN_CHACHA_H_

#include <botan/stream_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(chacha.h)

namespace Botan {

/**
* DJB's ChaCha (https://cr.yp.to/chacha.html)
*/
class BOTAN_PUBLIC_API(2,0) ChaCha final : public StreamCipher
   {
   public:
      /**
      * @param rounds number of rounds
      * @note Currently only 8, 12 or 20 rounds are supported, all others
      * will throw an exception
      */
      explicit ChaCha(size_t rounds = 20);

      std::string provider() const override;

      void cipher(const uint8_t in[], uint8_t out[], size_t length) override;

      void write_keystream(uint8_t out[], size_t len) override;

      void set_iv(const uint8_t iv[], size_t iv_len) override;

      /*
      * ChaCha accepts 0, 8, 12 or 24 byte IVs.
      * The default IV is a 8 zero bytes.
      * An IV of length 0 is treated the same as the default zero IV.
      * An IV of length 24 selects XChaCha mode
      */
      bool valid_iv_length(size_t iv_len) const override;

      size_t default_iv_length() const override;

      Key_Length_Specification key_spec() const override;

      void clear() override;

      StreamCipher* clone() const override;

      std::string name() const override;

      void seek(uint64_t offset) override;

   private:
      void key_schedule(const uint8_t key[], size_t key_len) override;

      void initialize_state();

      void chacha_x8(uint8_t output[64*8], uint32_t state[16], size_t rounds);

#if defined(BOTAN_HAS_CHACHA_SIMD32)
      void chacha_simd32_x4(uint8_t output[64*4], uint32_t state[16], size_t rounds);
#endif

#if defined(BOTAN_HAS_CHACHA_AVX2)
      void chacha_avx2_x8(uint8_t output[64*8], uint32_t state[16], size_t rounds);
#endif

      size_t m_rounds;
      secure_vector<uint32_t> m_key;
      secure_vector<uint32_t> m_state;
      secure_vector<uint8_t> m_buffer;
      size_t m_position = 0;
   };

}

#endif
