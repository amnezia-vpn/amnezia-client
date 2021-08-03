/*
* BLAKE2b
* (C) 2016 cynecx
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BLAKE2B_H_
#define BOTAN_BLAKE2B_H_

#include <botan/hash.h>
#include <string>
#include <memory>

BOTAN_FUTURE_INTERNAL_HEADER(blake2b.h)

namespace Botan {

/**
* BLAKE2B
*/
class BOTAN_PUBLIC_API(2,0) BLAKE2b final : public HashFunction
   {
   public:
      /**
      * @param output_bits the output size of BLAKE2b in bits
      */
      explicit BLAKE2b(size_t output_bits = 512);

      size_t hash_block_size() const override { return 128; }
      size_t output_length() const override { return m_output_bits / 8; }

      HashFunction* clone() const override;
      std::string name() const override;
      void clear() override;

      std::unique_ptr<HashFunction> copy_state() const override;

   private:
      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

      void state_init();
      void compress(const uint8_t* data, size_t blocks, uint64_t increment);

      const size_t m_output_bits;

      secure_vector<uint8_t> m_buffer;
      size_t m_bufpos;

      secure_vector<uint64_t> m_H;
      uint64_t m_T[2];
      uint64_t m_F[2];
   };

typedef BLAKE2b Blake2b;

}

#endif
