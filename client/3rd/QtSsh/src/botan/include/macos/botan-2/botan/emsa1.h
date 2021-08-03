/*
* EMSA1
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_EMSA1_H_
#define BOTAN_EMSA1_H_

#include <botan/emsa.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(emsa1.h)

namespace Botan {

/**
* EMSA1 from IEEE 1363
* Essentially, sign the hash directly
*/
class BOTAN_PUBLIC_API(2,0) EMSA1 final : public EMSA
   {
   public:
      /**
      * @param hash the hash function to use
      */
      explicit EMSA1(HashFunction* hash) : m_hash(hash) {}

      EMSA* clone() override;

      std::string name() const override;

      AlgorithmIdentifier config_for_x509(const Private_Key& key,
                                          const std::string& cert_hash_name) const override;
   private:
      size_t hash_output_length() const { return m_hash->output_length(); }

      void update(const uint8_t[], size_t) override;
      secure_vector<uint8_t> raw_data() override;

      secure_vector<uint8_t> encoding_of(const secure_vector<uint8_t>& msg,
                                         size_t output_bits,
                                         RandomNumberGenerator& rng) override;

      bool verify(const secure_vector<uint8_t>& coded,
                  const secure_vector<uint8_t>& raw,
                  size_t key_bits) override;

      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
