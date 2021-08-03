/*
 * (C) Copyright Projet SECRET, INRIA, Rocquencourt
 * (C) Bhaskar Biswas and  Nicolas Sendrier
 *
 * (C) 2014 cryptosource GmbH
 * (C) 2014 Falko Strenzke fstrenzke@cryptosource.de
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 *
 */

#ifndef BOTAN_CODE_BASED_UTIL_H_
#define BOTAN_CODE_BASED_UTIL_H_

#include <botan/gf2m_small_m.h>

namespace Botan {

/**
* Expand an input to a bit mask depending on it being being zero or non-zero
* @param tst the input
* @return the mask 0xFFFF if tst is non-zero and 0 otherwise
*/
template<typename T>
uint16_t expand_mask_16bit(T tst)
   {
   const uint16_t result = (tst != 0);
   return ~(result - 1);
   }

inline gf2m gray_to_lex(gf2m gray)
   {
   gf2m result = gray ^ (gray >> 8);
   result ^= (result >> 4);
   result ^= (result >> 2);
   result ^= (result >> 1);
   return result;
   }

inline gf2m lex_to_gray(gf2m lex)
   {
   return (lex >> 1) ^ lex;
   }

inline size_t bit_size_to_byte_size(size_t bit_size)
   {
   return (bit_size - 1) / 8 + 1;
   }

inline size_t bit_size_to_32bit_size(size_t bit_size)
   {
   return (bit_size - 1) / 32 + 1;
   }

}

#endif
