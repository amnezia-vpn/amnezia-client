/*
* Keypair Checks
* (C) 1999-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_KEYPAIR_CHECKS_H_
#define BOTAN_KEYPAIR_CHECKS_H_

#include <botan/pk_keys.h>

BOTAN_FUTURE_INTERNAL_HEADER(keypair.h)

namespace Botan {

namespace KeyPair {

/**
* Tests whether the key is consistent for encryption; whether
* encrypting and then decrypting gives to the original plaintext.
* @param rng the rng to use
* @param private_key the key to test
* @param public_key the key to test
* @param padding the encryption padding method to use
* @return true if consistent otherwise false
*/
BOTAN_PUBLIC_API(2,0) bool
encryption_consistency_check(RandomNumberGenerator& rng,
                             const Private_Key& private_key,
                             const Public_Key& public_key,
                             const std::string& padding);

/**
* Tests whether the key is consistent for signatures; whether a
* signature can be created and then verified
* @param rng the rng to use
* @param private_key the key to test
* @param public_key the key to test
* @param padding the signature padding method to use
* @return true if consistent otherwise false
*/
BOTAN_PUBLIC_API(2,0) bool
signature_consistency_check(RandomNumberGenerator& rng,
                            const Private_Key& private_key,
                            const Public_Key& public_key,
                            const std::string& padding);

/**
* Tests whether the key is consistent for encryption; whether
* encrypting and then decrypting gives to the original plaintext.
* @param rng the rng to use
* @param key the key to test
* @param padding the encryption padding method to use
* @return true if consistent otherwise false
*/
inline bool
encryption_consistency_check(RandomNumberGenerator& rng,
                             const Private_Key& key,
                             const std::string& padding)
   {
   return encryption_consistency_check(rng, key, key, padding);
   }

/**
* Tests whether the key is consistent for signatures; whether a
* signature can be created and then verified
* @param rng the rng to use
* @param key the key to test
* @param padding the signature padding method to use
* @return true if consistent otherwise false
*/
inline bool
signature_consistency_check(RandomNumberGenerator& rng,
                            const Private_Key& key,
                            const std::string& padding)
   {
   return signature_consistency_check(rng, key, key, padding);
   }

}

}

#endif
