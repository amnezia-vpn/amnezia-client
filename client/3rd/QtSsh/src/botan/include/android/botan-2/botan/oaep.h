/*
* OAEP
* (C) 1999-2007,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OAEP_H_
#define BOTAN_OAEP_H_

#include <botan/eme.h>
#include <botan/hash.h>

BOTAN_FUTURE_INTERNAL_HEADER(oaep.h)

namespace Botan {

/**
* OAEP (called EME1 in IEEE 1363 and in earlier versions of the library)
* as specified in PKCS#1 v2.0 (RFC 2437)
*/
class BOTAN_PUBLIC_API(2,0) OAEP final : public EME
   {
   public:
      size_t maximum_input_size(size_t) const override;

      /**
      * @param hash function to use for hashing (takes ownership)
      * @param P an optional label. Normally empty.
      */
      OAEP(HashFunction* hash, const std::string& P = "");

      /**
      * @param hash function to use for hashing (takes ownership)
      * @param mgf1_hash function to use for MGF1 (takes ownership)
      * @param P an optional label. Normally empty.
      */
      OAEP(HashFunction* hash,
           HashFunction* mgf1_hash,
           const std::string& P = "");
   private:
      secure_vector<uint8_t> pad(const uint8_t in[],
                              size_t in_length,
                              size_t key_length,
                              RandomNumberGenerator& rng) const override;

      secure_vector<uint8_t> unpad(uint8_t& valid_mask,
                                const uint8_t in[],
                                size_t in_len) const override;

      secure_vector<uint8_t> m_Phash;
      std::unique_ptr<HashFunction> m_mgf1_hash;
   };

secure_vector<uint8_t>
BOTAN_TEST_API oaep_find_delim(uint8_t& valid_mask,
                               const uint8_t input[], size_t input_len,
                               const secure_vector<uint8_t>& Phash);

}

#endif
