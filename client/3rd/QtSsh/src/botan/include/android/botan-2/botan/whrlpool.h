/*
* Whirlpool
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_WHIRLPOOL_H_
#define BOTAN_WHIRLPOOL_H_

#include <botan/mdx_hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(whrlpool.h)

namespace Botan {

/**
* Whirlpool
*/
class BOTAN_PUBLIC_API(2,0) Whirlpool final : public MDx_HashFunction
   {
   public:
      std::string name() const override { return "Whirlpool"; }
      size_t output_length() const override { return 64; }
      HashFunction* clone() const override { return new Whirlpool; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override;

      Whirlpool() : MDx_HashFunction(64, true, true, 32), m_M(8), m_digest(8)
         { clear(); }
   private:
      void compress_n(const uint8_t[], size_t blocks) override;
      void copy_out(uint8_t[]) override;

      static const uint64_t C0[256];
      static const uint64_t C1[256];
      static const uint64_t C2[256];
      static const uint64_t C3[256];
      static const uint64_t C4[256];
      static const uint64_t C5[256];
      static const uint64_t C6[256];
      static const uint64_t C7[256];

      secure_vector<uint64_t> m_M, m_digest;
   };

}

#endif
