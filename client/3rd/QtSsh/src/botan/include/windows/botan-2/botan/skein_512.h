/*
* The Skein-512 hash function
* (C) 2009,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SKEIN_512_H_
#define BOTAN_SKEIN_512_H_

#include <botan/hash.h>
#include <botan/threefish_512.h>
#include <string>
#include <memory>

BOTAN_FUTURE_INTERNAL_HEADER(skin_512.h)

namespace Botan {

/**
* Skein-512, a SHA-3 candidate
*/
class BOTAN_PUBLIC_API(2,0) Skein_512 final : public HashFunction
   {
   public:
      /**
      * @param output_bits the output size of Skein in bits
      * @param personalization is a string that will parameterize the
      * hash output
      */
      Skein_512(size_t output_bits = 512,
                const std::string& personalization = "");

      size_t hash_block_size() const override { return 64; }
      size_t output_length() const override { return m_output_bits / 8; }

      HashFunction* clone() const override;
      std::unique_ptr<HashFunction> copy_state() const override;
      std::string name() const override;
      void clear() override;
   private:
      enum type_code {
         SKEIN_KEY = 0,
         SKEIN_CONFIG = 4,
         SKEIN_PERSONALIZATION = 8,
         SKEIN_PUBLIC_KEY = 12,
         SKEIN_KEY_IDENTIFIER = 16,
         SKEIN_NONCE = 20,
         SKEIN_MSG = 48,
         SKEIN_OUTPUT = 63
      };

      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

      void ubi_512(const uint8_t msg[], size_t msg_len);

      void initial_block();
      void reset_tweak(type_code type, bool is_final);

      std::string m_personalization;
      size_t m_output_bits;

      std::unique_ptr<Threefish_512> m_threefish;
      secure_vector<uint64_t> m_T;
      secure_vector<uint8_t> m_buffer;
      size_t m_buf_pos;
   };

}

#endif
