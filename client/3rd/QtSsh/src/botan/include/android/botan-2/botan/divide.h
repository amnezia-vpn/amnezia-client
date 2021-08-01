/*
* Division
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DIVISON_ALGORITHM_H_
#define BOTAN_DIVISON_ALGORITHM_H_

#include <botan/bigint.h>

BOTAN_FUTURE_INTERNAL_HEADER(divide.h)

namespace Botan {

/**
* BigInt Division
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_UNSTABLE_API vartime_divide(const BigInt& x,
                                       const BigInt& y,
                                       BigInt& q,
                                       BigInt& r);

/**
* BigInt division, const time variant
*
* This runs with control flow independent of the values of x/y.
* Warning: the loop bounds still leak the sizes of x and y.
*
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_PUBLIC_API(2,9) ct_divide(const BigInt& x,
                                     const BigInt& y,
                                     BigInt& q,
                                     BigInt& r);

inline void divide(const BigInt& x,
                   const BigInt& y,
                   BigInt& q,
                   BigInt& r)
   {
   ct_divide(x, y, q, r);
   }

/**
* BigInt division, const time variant
*
* This runs with control flow independent of the values of x/y.
* Warning: the loop bounds still leak the sizes of x and y.
*
* @param x an integer
* @param y a non-zero integer
* @return x/y with remainder discarded
*/
inline BigInt ct_divide(const BigInt& x, const BigInt& y)
   {
   BigInt q, r;
   ct_divide(x, y, q, r);
   return q;
   }

/**
* BigInt division, const time variant
*
* This runs with control flow independent of the values of x/y.
* Warning: the loop bounds still leak the sizes of x and y.
*
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_PUBLIC_API(2,9) ct_divide_u8(const BigInt& x,
                                        uint8_t y,
                                        BigInt& q,
                                        uint8_t& r);

/**
* BigInt modulo, const time variant
*
* Using this function is (slightly) cheaper than calling ct_divide and
* using only the remainder.
*
* @param x a non-negative integer
* @param modulo a positive integer
* @return result x % modulo
*/
BigInt BOTAN_PUBLIC_API(2,9) ct_modulo(const BigInt& x,
                                       const BigInt& modulo);

}

#endif
