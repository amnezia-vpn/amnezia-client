/*
* RIPEMD-160
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RIPEMD_160_H_
#define BOTAN_RIPEMD_160_H_

#include <botan/mdx_hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(rmd160.h)

namespace Botan {

/**
* RIPEMD-160
*/
class BOTAN_PUBLIC_API(2,0) RIPEMD_160 final : public MDx_HashFunction
   {
   public:
      std::string name() const override { return "RIPEMD-160"; }
      size_t output_length() const override { return 20; }
      HashFunction* clone() const override { return new RIPEMD_160; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override;

      RIPEMD_160() : MDx_HashFunction(64, false, true), m_M(16), m_digest(5)
         { clear(); }
   private:
      void compress_n(const uint8_t[], size_t blocks) override;
      void copy_out(uint8_t[]) override;

      secure_vector<uint32_t> m_M, m_digest;
   };

}

#endif
