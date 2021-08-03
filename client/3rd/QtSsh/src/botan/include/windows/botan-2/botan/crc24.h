/*
* CRC24
* (C) 1999-2007 Jack Lloyd
* (C) 2017 [Ribose Inc](https://www.ribose.com). Performed by Krzysztof Kwiatkowski.
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CRC24_H_
#define BOTAN_CRC24_H_

#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(crc24.h)

namespace Botan {

/**
* 24-bit cyclic redundancy check
*/
class BOTAN_PUBLIC_API(2,0) CRC24 final : public HashFunction
   {
   public:
      std::string name() const override { return "CRC24"; }
      size_t output_length() const override { return 3; }
      HashFunction* clone() const override { return new CRC24; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override { m_crc = 0XCE04B7L; }

      CRC24() { clear(); }
      ~CRC24() { clear(); }
   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      uint32_t m_crc;
   };

}

#endif
