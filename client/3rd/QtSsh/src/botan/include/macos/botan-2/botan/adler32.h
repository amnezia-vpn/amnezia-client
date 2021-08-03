/*
* Adler32
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ADLER32_H_
#define BOTAN_ADLER32_H_

#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(adler32.h)

namespace Botan {

/**
* The Adler32 checksum, used in zlib
*/
class BOTAN_PUBLIC_API(2,0) Adler32 final : public HashFunction
   {
   public:
      std::string name() const override { return "Adler32"; }
      size_t output_length() const override { return 4; }
      HashFunction* clone() const override { return new Adler32; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override { m_S1 = 1; m_S2 = 0; }

      Adler32() { clear(); }
      ~Adler32() { clear(); }
   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      uint16_t m_S1, m_S2;
   };

}

#endif
