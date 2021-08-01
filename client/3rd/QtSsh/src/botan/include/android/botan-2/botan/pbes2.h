/*
* PKCS #5 v2.0 PBE
* (C) 1999-2007,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PBE_PKCS_v20_H_
#define BOTAN_PBE_PKCS_v20_H_

#include <botan/asn1_obj.h>
#include <chrono>

BOTAN_FUTURE_INTERNAL_HEADER(pbes2.h)

namespace Botan {

class RandomNumberGenerator;

/**
* Encrypt with PBES2 from PKCS #5 v2.0
* @param key_bits the input
* @param passphrase the passphrase to use for encryption
* @param msec how many milliseconds to run PBKDF2
* @param cipher specifies the block cipher to use to encrypt
* @param digest specifies the PRF to use with PBKDF2 (eg "HMAC(SHA-1)")
* @param rng a random number generator
*/
std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
BOTAN_PUBLIC_API(2,0) pbes2_encrypt(const secure_vector<uint8_t>& key_bits,
                        const std::string& passphrase,
                        std::chrono::milliseconds msec,
                        const std::string& cipher,
                        const std::string& digest,
                        RandomNumberGenerator& rng);

/**
* Encrypt with PBES2 from PKCS #5 v2.0
* @param key_bits the input
* @param passphrase the passphrase to use for encryption
* @param msec how many milliseconds to run PBKDF2
* @param out_iterations_if_nonnull if not null, set to the number
* of PBKDF iterations used
* @param cipher specifies the block cipher to use to encrypt
* @param digest specifies the PRF to use with PBKDF2 (eg "HMAC(SHA-1)")
* @param rng a random number generator
*/
std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
BOTAN_PUBLIC_API(2,1) pbes2_encrypt_msec(const secure_vector<uint8_t>& key_bits,
                             const std::string& passphrase,
                             std::chrono::milliseconds msec,
                             size_t* out_iterations_if_nonnull,
                             const std::string& cipher,
                             const std::string& digest,
                             RandomNumberGenerator& rng);

/**
* Encrypt with PBES2 from PKCS #5 v2.0
* @param key_bits the input
* @param passphrase the passphrase to use for encryption
* @param iterations how many iterations to run PBKDF2
* @param cipher specifies the block cipher to use to encrypt
* @param digest specifies the PRF to use with PBKDF2 (eg "HMAC(SHA-1)")
* @param rng a random number generator
*/
std::pair<AlgorithmIdentifier, std::vector<uint8_t>>
BOTAN_PUBLIC_API(2,1) pbes2_encrypt_iter(const secure_vector<uint8_t>& key_bits,
                             const std::string& passphrase,
                             size_t iterations,
                             const std::string& cipher,
                             const std::string& digest,
                             RandomNumberGenerator& rng);

/**
* Decrypt a PKCS #5 v2.0 encrypted stream
* @param key_bits the input
* @param passphrase the passphrase to use for decryption
* @param params the PBES2 parameters
*/
secure_vector<uint8_t>
BOTAN_PUBLIC_API(2,0) pbes2_decrypt(const secure_vector<uint8_t>& key_bits,
                        const std::string& passphrase,
                        const std::vector<uint8_t>& params);

}

#endif
