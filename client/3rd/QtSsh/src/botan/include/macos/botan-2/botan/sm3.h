/*
* SM3
* (C) 2017 Ribose Inc.
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SM3_H_
#define BOTAN_SM3_H_

#include <botan/mdx_hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(sm3.h)

namespace Botan {

enum {
  SM3_BLOCK_BYTES = 64,
  SM3_DIGEST_BYTES = 32
};

/**
* SM3
*/
class BOTAN_PUBLIC_API(2,2) SM3 final : public MDx_HashFunction
   {
   public:
      std::string name() const override { return "SM3"; }
      size_t output_length() const override { return SM3_DIGEST_BYTES; }
      HashFunction* clone() const override { return new SM3; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override;

      SM3() : MDx_HashFunction(SM3_BLOCK_BYTES, true, true), m_digest(SM3_DIGEST_BYTES)
         { clear(); }
   private:
      void compress_n(const uint8_t[], size_t blocks) override;
      void copy_out(uint8_t[]) override;

      /**
      * The digest value
      */
      secure_vector<uint32_t> m_digest;
   };

}

#endif
