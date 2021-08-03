/*
* SHAKE-128 as a stream cipher
* (C) 2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SHAKE128_CIPHER_H_
#define BOTAN_SHAKE128_CIPHER_H_

#include <botan/stream_cipher.h>
#include <botan/secmem.h>

BOTAN_FUTURE_INTERNAL_HEADER(shake_cipher.h)

namespace Botan {

/**
* SHAKE-128 XOF presented as a stream cipher
*/
class BOTAN_PUBLIC_API(2,0) SHAKE_128_Cipher final : public StreamCipher
   {
   public:
      SHAKE_128_Cipher();

      /**
      * Produce more XOF output
      */
      void cipher(const uint8_t in[], uint8_t out[], size_t length) override;

      /**
      * Seeking is not supported, this function will throw
      */
      void seek(uint64_t offset) override;

      /**
      * IV not supported, this function will throw unless iv_len == 0
      */
      void set_iv(const uint8_t iv[], size_t iv_len) override;

      Key_Length_Specification key_spec() const override;

      void clear() override;
      std::string name() const override;
      StreamCipher* clone() const override;

   private:
      void key_schedule(const uint8_t key[], size_t key_len) override;

      secure_vector<uint64_t> m_state; // internal state
      secure_vector<uint8_t> m_buffer; // ciphertext buffer
      size_t m_buf_pos; // position in m_buffer
   };

}

#endif
