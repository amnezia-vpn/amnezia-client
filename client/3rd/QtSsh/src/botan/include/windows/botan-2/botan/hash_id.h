/*
* Hash Function Identification
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_HASHID_H_
#define BOTAN_HASHID_H_

#include <botan/secmem.h>
#include <string>

namespace Botan {

/**
* Return the PKCS #1 hash identifier
* @see RFC 3447 section 9.2
* @param hash_name the name of the hash function
* @return uint8_t sequence identifying the hash
* @throw Invalid_Argument if the hash has no known PKCS #1 hash id
*/
BOTAN_PUBLIC_API(2,0) std::vector<uint8_t> pkcs_hash_id(const std::string& hash_name);

/**
* Return the IEEE 1363 hash identifier
* @param hash_name the name of the hash function
* @return uint8_t code identifying the hash, or 0 if not known
*/
BOTAN_PUBLIC_API(2,0) uint8_t ieee1363_hash_id(const std::string& hash_name);

}

#endif
