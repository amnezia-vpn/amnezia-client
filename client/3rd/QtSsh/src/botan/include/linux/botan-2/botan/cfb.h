/*
* CFB mode
* (C) 1999-2007,2013 Jack Lloyd
* (C) 2016 Daniel Neus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MODE_CFB_H_
#define BOTAN_MODE_CFB_H_

#include <botan/cipher_mode.h>
#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cfb.h)

namespace Botan {

/**
* CFB Mode
*/
class BOTAN_PUBLIC_API(2,0) CFB_Mode : public Cipher_Mode
   {
   public:
      std::string name() const override final;

      size_t update_granularity() const override final;

      size_t minimum_final_size() const override final;

      Key_Length_Specification key_spec() const override final;

      size_t output_length(size_t input_length) const override final;

      size_t default_nonce_length() const override final;

      bool valid_nonce_length(size_t n) const override final;

      void clear() override final;

      void reset() override final;
   protected:
      CFB_Mode(BlockCipher* cipher, size_t feedback_bits);

      void shift_register();

      size_t feedback() const { return m_feedback_bytes; }
      const BlockCipher& cipher() const { return *m_cipher; }
      size_t block_size() const { return m_block_size; }

      secure_vector<uint8_t> m_state;
      secure_vector<uint8_t> m_keystream;
      size_t m_keystream_pos = 0;

   private:
      void start_msg(const uint8_t nonce[], size_t nonce_len) override;
      void key_schedule(const uint8_t key[], size_t length) override;

      std::unique_ptr<BlockCipher> m_cipher;
      const size_t m_block_size;
      const size_t m_feedback_bytes;
   };

/**
* CFB Encryption
*/
class BOTAN_PUBLIC_API(2,0) CFB_Encryption final : public CFB_Mode
   {
   public:
      /**
      * If feedback_bits is zero, cipher->block_size() bytes will be used.
      * @param cipher block cipher to use
      * @param feedback_bits number of bits fed back into the shift register,
      * must be a multiple of 8
      */
      CFB_Encryption(BlockCipher* cipher, size_t feedback_bits) :
         CFB_Mode(cipher, feedback_bits) {}

      size_t process(uint8_t buf[], size_t size) override;

      void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) override;
   };

/**
* CFB Decryption
*/
class BOTAN_PUBLIC_API(2,0) CFB_Decryption final : public CFB_Mode
   {
   public:
      /**
      * If feedback_bits is zero, cipher->block_size() bytes will be used.
      * @param cipher block cipher to use
      * @param feedback_bits number of bits fed back into the shift register,
      * must be a multiple of 8
      */
      CFB_Decryption(BlockCipher* cipher, size_t feedback_bits) :
         CFB_Mode(cipher, feedback_bits) {}

      size_t process(uint8_t buf[], size_t size) override;

      void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) override;
   };

}

#endif
