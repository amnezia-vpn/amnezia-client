/*
* Serpent SBox Expressions
* (C) 1999-2007,2013 Jack Lloyd
*
* The sbox expressions used here were discovered by Dag Arne Osvik and
* are described in his paper "Speeding Up Serpent".
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SERPENT_SBOX_H_
#define BOTAN_SERPENT_SBOX_H_

#include <botan/build.h>

template<typename T>
BOTAN_FORCE_INLINE void SBoxE0(T& a, T& b, T& c, T& d)
   {
   d ^= a;
   T t0 = b;
   b &= d;
   t0 ^= c;
   b ^= a;
   a |= d;
   a ^= t0;
   t0 ^= d;
   d ^= c;
   c |= b;
   c ^= t0;
   t0 = ~t0;
   t0 |= b;
   b ^= d;
   b ^= t0;
   d |= a;
   b ^= d;
   t0 ^= d;
   d = a;
   a = b;
   b = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE1(T& a, T& b, T& c, T& d)
   {
   a = ~a;
   c = ~c;
   T t0 = a;
   a &= b;
   c ^= a;
   a |= d;
   d ^= c;
   b ^= a;
   a ^= t0;
   t0 |= b;
   b ^= d;
   c |= a;
   c &= t0;
   a ^= b;
   b &= c;
   b ^= a;
   a &= c;
   t0 ^= a;
   a = c;
   c = d;
   d = b;
   b = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE2(T& a, T& b, T& c, T& d)
   {
   T t0 = a;
   a &= c;
   a ^= d;
   c ^= b;
   c ^= a;
   d |= t0;
   d ^= b;
   t0 ^= c;
   b = d;
   d |= t0;
   d ^= a;
   a &= b;
   t0 ^= a;
   b ^= d;
   b ^= t0;
   a = c;
   c = b;
   b = d;
   d = ~t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE3(T& a, T& b, T& c, T& d)
   {
   T t0 = a;
   a |= d;
   d ^= b;
   b &= t0;
   t0 ^= c;
   c ^= d;
   d &= a;
   t0 |= b;
   d ^= t0;
   a ^= b;
   t0 &= a;
   b ^= d;
   t0 ^= c;
   b |= a;
   b ^= c;
   a ^= d;
   c = b;
   b |= d;
   a ^= b;
   b = c;
   c = d;
   d = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE4(T& a, T& b, T& c, T& d)
   {
   b ^= d;
   d = ~d;
   c ^= d;
   d ^= a;
   T t0 = b;
   b &= d;
   b ^= c;
   t0 ^= d;
   a ^= t0;
   c &= t0;
   c ^= a;
   a &= b;
   d ^= a;
   t0 |= b;
   t0 ^= a;
   a |= d;
   a ^= c;
   c &= d;
   a = ~a;
   t0 ^= c;
   c = a;
   a = b;
   b = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE5(T& a, T& b, T& c, T& d)
   {
   a ^= b;
   b ^= d;
   d = ~d;
   T t0 = b;
   b &= a;
   c ^= d;
   b ^= c;
   c |= t0;
   t0 ^= d;
   d &= b;
   d ^= a;
   t0 ^= b;
   t0 ^= c;
   c ^= a;
   a &= d;
   c = ~c;
   a ^= t0;
   t0 |= d;
   t0 ^= c;
   c = a;
   a = b;
   b = d;
   d = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE6(T& a, T& b, T& c, T& d)
   {
   c = ~c;
   T t0 = d;
   d &= a;
   a ^= t0;
   d ^= c;
   c |= t0;
   b ^= d;
   c ^= a;
   a |= b;
   c ^= b;
   t0 ^= a;
   a |= d;
   a ^= c;
   t0 ^= d;
   t0 ^= a;
   d = ~d;
   c &= t0;
   d ^= c;
   c = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxE7(T& a, T& b, T& c, T& d)
   {
   T t0 = b;
   b |= c;
   b ^= d;
   t0 ^= c;
   c ^= b;
   d |= t0;
   d &= a;
   t0 ^= c;
   d ^= b;
   b |= t0;
   b ^= a;
   a |= t0;
   a ^= c;
   b ^= t0;
   c ^= b;
   b &= a;
   b ^= t0;
   c = ~c;
   c |= a;
   t0 ^= c;
   c = b;
   b = d;
   d = a;
   a = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD0(T& a, T& b, T& c, T& d)
   {
   c = ~c;
   T t0 = b;
   b |= a;
   t0 = ~t0;
   b ^= c;
   c |= t0;
   b ^= d;
   a ^= t0;
   c ^= a;
   a &= d;
   t0 ^= a;
   a |= b;
   a ^= c;
   d ^= t0;
   c ^= b;
   d ^= a;
   d ^= b;
   c &= d;
   t0 ^= c;
   c = b;
   b = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD1(T& a, T& b, T& c, T& d)
   {
   T t0 = b;
   b ^= d;
   d &= b;
   t0 ^= c;
   d ^= a;
   a |= b;
   c ^= d;
   a ^= t0;
   a |= c;
   b ^= d;
   a ^= b;
   b |= d;
   b ^= a;
   t0 = ~t0;
   t0 ^= b;
   b |= a;
   b ^= a;
   b |= t0;
   d ^= b;
   b = a;
   a = t0;
   t0 = c;
   c = d;
   d = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD2(T& a, T& b, T& c, T& d)
   {
   c ^= d;
   d ^= a;
   T t0 = d;
   d &= c;
   d ^= b;
   b |= c;
   b ^= t0;
   t0 &= d;
   c ^= d;
   t0 &= a;
   t0 ^= c;
   c &= b;
   c |= a;
   d = ~d;
   c ^= d;
   a ^= d;
   a &= b;
   d ^= t0;
   d ^= a;
   a = b;
   b = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD3(T& a, T& b, T& c, T& d)
   {
   T t0 = c;
   c ^= b;
   a ^= c;
   t0 &= c;
   t0 ^= a;
   a &= b;
   b ^= d;
   d |= t0;
   c ^= d;
   a ^= d;
   b ^= t0;
   d &= c;
   d ^= b;
   b ^= a;
   b |= c;
   a ^= d;
   b ^= t0;
   a ^= b;
   t0 = a;
   a = c;
   c = d;
   d = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD4(T& a, T& b, T& c, T& d)
   {
   T t0 = c;
   c &= d;
   c ^= b;
   b |= d;
   b &= a;
   t0 ^= c;
   t0 ^= b;
   b &= c;
   a = ~a;
   d ^= t0;
   b ^= d;
   d &= a;
   d ^= c;
   a ^= b;
   c &= a;
   d ^= a;
   c ^= t0;
   c |= d;
   d ^= a;
   c ^= b;
   b = d;
   d = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD5(T& a, T& b, T& c, T& d)
   {
   b = ~b;
   T t0 = d;
   c ^= b;
   d |= a;
   d ^= c;
   c |= b;
   c &= a;
   t0 ^= d;
   c ^= t0;
   t0 |= a;
   t0 ^= b;
   b &= c;
   b ^= d;
   t0 ^= c;
   d &= t0;
   t0 ^= b;
   d ^= t0;
   t0 = ~t0;
   d ^= a;
   a = b;
   b = t0;
   t0 = d;
   d = c;
   c = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD6(T& a, T& b, T& c, T& d)
   {
   a ^= c;
   T t0 = c;
   c &= a;
   t0 ^= d;
   c = ~c;
   d ^= b;
   c ^= d;
   t0 |= a;
   a ^= c;
   d ^= t0;
   t0 ^= b;
   b &= d;
   b ^= a;
   a ^= d;
   a |= c;
   d ^= b;
   t0 ^= a;
   a = b;
   b = c;
   c = t0;
   }

template<typename T>
BOTAN_FORCE_INLINE void SBoxD7(T& a, T& b, T& c, T& d)
   {
   T t0 = c;
   c ^= a;
   a &= d;
   t0 |= d;
   c = ~c;
   d ^= b;
   b |= a;
   a ^= c;
   c &= t0;
   d &= t0;
   b ^= c;
   c ^= a;
   a |= c;
   t0 ^= b;
   a ^= d;
   d ^= t0;
   t0 |= a;
   d ^= c;
   t0 ^= c;
   c = b;
   b = a;
   a = d;
   d = t0;
   }

#endif
