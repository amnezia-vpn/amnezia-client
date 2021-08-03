/*
* Tiger
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TIGER_H_
#define BOTAN_TIGER_H_

#include <botan/mdx_hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(tiger.h)

namespace Botan {

/**
* Tiger
*/
class BOTAN_PUBLIC_API(2,0) Tiger final : public MDx_HashFunction
   {
   public:
      std::string name() const override;
      size_t output_length() const override { return m_hash_len; }

      HashFunction* clone() const override
         {
         return new Tiger(output_length(), m_passes);
         }

      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override;

      /**
      * @param out_size specifies the output length; can be 16, 20, or 24
      * @param passes to make in the algorithm
      */
      Tiger(size_t out_size = 24, size_t passes = 3);
   private:
      void compress_n(const uint8_t[], size_t block) override;
      void copy_out(uint8_t[]) override;

      static void pass(uint64_t& A, uint64_t& B, uint64_t& C,
                       const secure_vector<uint64_t>& M,
                       uint8_t mul);

      static const uint64_t SBOX1[256];
      static const uint64_t SBOX2[256];
      static const uint64_t SBOX3[256];
      static const uint64_t SBOX4[256];

      secure_vector<uint64_t> m_X, m_digest;
      const size_t m_hash_len, m_passes;
   };

}

#endif
