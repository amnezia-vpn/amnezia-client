/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_POLY_DBL_H_
#define BOTAN_POLY_DBL_H_

#include <botan/types.h>

namespace Botan {

/**
* Polynomial doubling in GF(2^n)
*/
void BOTAN_TEST_API poly_double_n(uint8_t out[], const uint8_t in[], size_t n);

/**
* Returns true iff poly_double_n is implemented for this size.
*/
inline bool poly_double_supported_size(size_t n)
   {
   return (n == 8 || n == 16 || n == 24 || n == 32 || n == 64 || n == 128);
   }

inline void poly_double_n(uint8_t buf[], size_t n)
   {
   return poly_double_n(buf, buf, n);
   }

/*
* Little endian convention - used for XTS
*/
void BOTAN_TEST_API poly_double_n_le(uint8_t out[], const uint8_t in[], size_t n);

}

#endif
