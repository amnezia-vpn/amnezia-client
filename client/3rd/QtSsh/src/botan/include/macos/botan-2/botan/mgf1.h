/*
* MGF1
* (C) 1999-2007,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MGF1_H_
#define BOTAN_MGF1_H_

#include <botan/types.h>

namespace Botan {

class HashFunction;

/**
* MGF1 from PKCS #1 v2.0
* @param hash hash function to use
* @param in input buffer
* @param in_len size of the input buffer in bytes
* @param out output buffer
* @param out_len size of the output buffer in bytes
*/
void BOTAN_PUBLIC_API(2,0) mgf1_mask(HashFunction& hash,
                         const uint8_t in[], size_t in_len,
                         uint8_t out[], size_t out_len);

}

#endif
