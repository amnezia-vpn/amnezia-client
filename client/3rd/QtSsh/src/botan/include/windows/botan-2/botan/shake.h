/*
* SHAKE hash functions
* (C) 2010,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SHAKE_HASH_H_
#define BOTAN_SHAKE_HASH_H_

#include <botan/hash.h>
#include <botan/secmem.h>
#include <string>

BOTAN_FUTURE_INTERNAL_HEADER(shake.h)

namespace Botan {

/**
* SHAKE-128
*/
class BOTAN_PUBLIC_API(2,0) SHAKE_128 final : public HashFunction
   {
   public:

      /**
      * @param output_bits the desired output size in bits
      * must be a multiple of 8
      */
      explicit SHAKE_128(size_t output_bits);

      size_t hash_block_size() const override { return SHAKE_128_BITRATE / 8; }
      size_t output_length() const override { return m_output_bits / 8; }

      HashFunction* clone() const override;
      std::unique_ptr<HashFunction> copy_state() const override;
      std::string name() const override;
      void clear() override;

   private:
      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

      static const size_t SHAKE_128_BITRATE = 1600 - 256;

      size_t m_output_bits;
      secure_vector<uint64_t> m_S;
      size_t m_S_pos;
   };

/**
* SHAKE-256
*/
class BOTAN_PUBLIC_API(2,0) SHAKE_256 final : public HashFunction
   {
   public:

      /**
      * @param output_bits the desired output size in bits
      * must be a multiple of 8
      */
      explicit SHAKE_256(size_t output_bits);

      size_t hash_block_size() const override { return SHAKE_256_BITRATE / 8; }
      size_t output_length() const override { return m_output_bits / 8; }

      HashFunction* clone() const override;
      std::unique_ptr<HashFunction> copy_state() const override;
      std::string name() const override;
      void clear() override;

   private:
      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

      static const size_t SHAKE_256_BITRATE = 1600 - 512;

      size_t m_output_bits;
      secure_vector<uint64_t> m_S;
      size_t m_S_pos;
   };

}

#endif
