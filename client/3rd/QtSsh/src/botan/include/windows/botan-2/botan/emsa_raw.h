/*
* EMSA-Raw
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_EMSA_RAW_H_
#define BOTAN_EMSA_RAW_H_

#include <botan/emsa.h>

BOTAN_FUTURE_INTERNAL_HEADER(emsa_raw.h)

namespace Botan {

/**
* EMSA-Raw - sign inputs directly
* Don't use this unless you know what you are doing.
*/
class BOTAN_PUBLIC_API(2,0) EMSA_Raw final : public EMSA
   {
   public:
      EMSA* clone() override { return new EMSA_Raw(); }

      explicit EMSA_Raw(size_t expected_hash_size = 0) :
         m_expected_size(expected_hash_size) {}

      std::string name() const override;
   private:
      void update(const uint8_t[], size_t) override;
      secure_vector<uint8_t> raw_data() override;

      secure_vector<uint8_t> encoding_of(const secure_vector<uint8_t>&, size_t,
                                         RandomNumberGenerator&) override;

      bool verify(const secure_vector<uint8_t>&,
                  const secure_vector<uint8_t>&,
                  size_t) override;

      const size_t m_expected_size;
      secure_vector<uint8_t> m_message;
   };

}

#endif
