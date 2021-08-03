/*
* CRC32
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CRC32_H_
#define BOTAN_CRC32_H_

#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(crc32.h)

namespace Botan {

/**
* 32-bit cyclic redundancy check
*/
class BOTAN_PUBLIC_API(2,0) CRC32 final : public HashFunction
   {
   public:
      std::string name() const override { return "CRC32"; }
      size_t output_length() const override { return 4; }
      HashFunction* clone() const override { return new CRC32; }
      std::unique_ptr<HashFunction> copy_state() const override;

      void clear() override { m_crc = 0xFFFFFFFF; }

      CRC32() { clear(); }
      ~CRC32() { clear(); }
   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      uint32_t m_crc;
   };

}

#endif
