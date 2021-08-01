/*
* MD4
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MD4_H_
#define BOTAN_MD4_H_

#include <botan/mdx_hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(md4.h)

namespace Botan {

/**
* MD4
*/
class BOTAN_PUBLIC_API(2,0) MD4 final : public MDx_HashFunction
   {
   public:
      std::string name() const override { return "MD4"; }
      size_t output_length() const override { return 16; }
      HashFunction* clone() const override { return new MD4; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override;

      MD4() : MDx_HashFunction(64, false, true), m_digest(4)
         { clear(); }

   private:
      void compress_n(const uint8_t input[], size_t blocks) override;
      void copy_out(uint8_t[]) override;

      /**
      * The digest value
      */
      secure_vector<uint32_t> m_digest;
   };

}

#endif
