/*
* Salsa20 / XSalsa20
* (C) 1999-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SALSA20_H_
#define BOTAN_SALSA20_H_

#include <botan/stream_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(salsa20.h)

namespace Botan {

/**
* DJB's Salsa20 (and XSalsa20)
*/
class BOTAN_PUBLIC_API(2,0) Salsa20 final : public StreamCipher
   {
   public:
      void cipher(const uint8_t in[], uint8_t out[], size_t length) override;

      void set_iv(const uint8_t iv[], size_t iv_len) override;

      bool valid_iv_length(size_t iv_len) const override;

      size_t default_iv_length() const override;

      Key_Length_Specification key_spec() const override;

      void clear() override;
      std::string name() const override;
      StreamCipher* clone() const override;

      static void salsa_core(uint8_t output[64], const uint32_t input[16], size_t rounds);
      static void hsalsa20(uint32_t output[8], const uint32_t input[16]);

      void seek(uint64_t offset) override;
   private:
      void key_schedule(const uint8_t key[], size_t key_len) override;

      void initialize_state();

      secure_vector<uint32_t> m_key;
      secure_vector<uint32_t> m_state;
      secure_vector<uint8_t> m_buffer;
      size_t m_position = 0;
   };

}

#endif
