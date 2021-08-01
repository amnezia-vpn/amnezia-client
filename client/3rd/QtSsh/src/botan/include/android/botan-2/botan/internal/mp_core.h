/*
* MPI Algorithms
* (C) 1999-2010,2018 Jack Lloyd
*     2006 Luca Piccarreta
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MP_CORE_OPS_H_
#define BOTAN_MP_CORE_OPS_H_

#include <botan/types.h>
#include <botan/exceptn.h>
#include <botan/mem_ops.h>
#include <botan/internal/mp_asmi.h>
#include <botan/internal/ct_utils.h>
#include <algorithm>

namespace Botan {

const word MP_WORD_MAX = ~static_cast<word>(0);

/*
* If cond == 0, does nothing.
* If cond > 0, swaps x[0:size] with y[0:size]
* Runs in constant time
*/
inline void bigint_cnd_swap(word cnd, word x[], word y[], size_t size)
   {
   const auto mask = CT::Mask<word>::expand(cnd);

   for(size_t i = 0; i != size; ++i)
      {
      const word a = x[i];
      const word b = y[i];
      x[i] = mask.select(b, a);
      y[i] = mask.select(a, b);
      }
   }

inline word bigint_cnd_add(word cnd, word x[], word x_size,
                           const word y[], size_t y_size)
   {
   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const auto mask = CT::Mask<word>::expand(cnd);

   word carry = 0;

   const size_t blocks = y_size - (y_size % 8);
   word z[8] = { 0 };

   for(size_t i = 0; i != blocks; i += 8)
      {
      carry = word8_add3(z, x + i, y + i, carry);
      mask.select_n(x + i, z, x + i, 8);
      }

   for(size_t i = blocks; i != y_size; ++i)
      {
      z[0] = word_add(x[i], y[i], &carry);
      x[i] = mask.select(z[0], x[i]);
      }

   for(size_t i = y_size; i != x_size; ++i)
      {
      z[0] = word_add(x[i], 0, &carry);
      x[i] = mask.select(z[0], x[i]);
      }

   return mask.if_set_return(carry);
   }

/*
* If cond > 0 adds x[0:size] and y[0:size] and returns carry
* Runs in constant time
*/
inline word bigint_cnd_add(word cnd, word x[], const word y[], size_t size)
   {
   return bigint_cnd_add(cnd, x, size, y, size);
   }

/*
* If cond > 0 subtracts x[0:size] and y[0:size] and returns borrow
* Runs in constant time
*/
inline word bigint_cnd_sub(word cnd,
                           word x[], size_t x_size,
                           const word y[], size_t y_size)
   {
   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const auto mask = CT::Mask<word>::expand(cnd);

   word carry = 0;

   const size_t blocks = y_size - (y_size % 8);
   word z[8] = { 0 };

   for(size_t i = 0; i != blocks; i += 8)
      {
      carry = word8_sub3(z, x + i, y + i, carry);
      mask.select_n(x + i, z, x + i, 8);
      }

   for(size_t i = blocks; i != y_size; ++i)
      {
      z[0] = word_sub(x[i], y[i], &carry);
      x[i] = mask.select(z[0], x[i]);
      }

   for(size_t i = y_size; i != x_size; ++i)
      {
      z[0] = word_sub(x[i], 0, &carry);
      x[i] = mask.select(z[0], x[i]);
      }

   return mask.if_set_return(carry);
   }

/*
* If cond > 0 adds x[0:size] and y[0:size] and returns carry
* Runs in constant time
*/
inline word bigint_cnd_sub(word cnd, word x[], const word y[], size_t size)
   {
   return bigint_cnd_sub(cnd, x, size, y, size);
   }


/*
* Equivalent to
*   bigint_cnd_add( mask, x, y, size);
*   bigint_cnd_sub(~mask, x, y, size);
*
* Mask must be either 0 or all 1 bits
*/
inline void bigint_cnd_add_or_sub(CT::Mask<word> mask, word x[], const word y[], size_t size)
   {
   const size_t blocks = size - (size % 8);

   word carry = 0;
   word borrow = 0;

   word t0[8] = { 0 };
   word t1[8] = { 0 };

   for(size_t i = 0; i != blocks; i += 8)
      {
      carry = word8_add3(t0, x + i, y + i, carry);
      borrow = word8_sub3(t1, x + i, y + i, borrow);

      for(size_t j = 0; j != 8; ++j)
         x[i+j] = mask.select(t0[j], t1[j]);
      }

   for(size_t i = blocks; i != size; ++i)
      {
      const word a = word_add(x[i], y[i], &carry);
      const word s = word_sub(x[i], y[i], &borrow);

      x[i] = mask.select(a, s);
      }
   }

/*
* Equivalent to
*   bigint_cnd_add( mask, x, size, y, size);
*   bigint_cnd_sub(~mask, x, size, z, size);
*
* Mask must be either 0 or all 1 bits
*
* Returns the carry or borrow resp
*/
inline word bigint_cnd_addsub(CT::Mask<word> mask, word x[],
                              const word y[], const word z[],
                              size_t size)
   {
   const size_t blocks = size - (size % 8);

   word carry = 0;
   word borrow = 0;

   word t0[8] = { 0 };
   word t1[8] = { 0 };

   for(size_t i = 0; i != blocks; i += 8)
      {
      carry = word8_add3(t0, x + i, y + i, carry);
      borrow = word8_sub3(t1, x + i, z + i, borrow);

      for(size_t j = 0; j != 8; ++j)
         x[i+j] = mask.select(t0[j], t1[j]);
      }

   for(size_t i = blocks; i != size; ++i)
      {
      t0[0] = word_add(x[i], y[i], &carry);
      t1[0] = word_sub(x[i], z[i], &borrow);
      x[i] = mask.select(t0[0], t1[0]);
      }

   return mask.select(carry, borrow);
   }

/*
* 2s complement absolute value
* If cond > 0 sets x to ~x + 1
* Runs in constant time
*/
inline void bigint_cnd_abs(word cnd, word x[], size_t size)
   {
   const auto mask = CT::Mask<word>::expand(cnd);

   word carry = mask.if_set_return(1);
   for(size_t i = 0; i != size; ++i)
      {
      const word z = word_add(~x[i], 0, &carry);
      x[i] = mask.select(z, x[i]);
      }
   }

/**
* Two operand addition with carry out
*/
inline word bigint_add2_nc(word x[], size_t x_size, const word y[], size_t y_size)
   {
   word carry = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8)
      carry = word8_add2(x + i, y + i, carry);

   for(size_t i = blocks; i != y_size; ++i)
      x[i] = word_add(x[i], y[i], &carry);

   for(size_t i = y_size; i != x_size; ++i)
      x[i] = word_add(x[i], 0, &carry);

   return carry;
   }

/**
* Three operand addition with carry out
*/
inline word bigint_add3_nc(word z[],
                           const word x[], size_t x_size,
                           const word y[], size_t y_size)
   {
   if(x_size < y_size)
      { return bigint_add3_nc(z, y, y_size, x, x_size); }

   word carry = 0;

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8)
      carry = word8_add3(z + i, x + i, y + i, carry);

   for(size_t i = blocks; i != y_size; ++i)
      z[i] = word_add(x[i], y[i], &carry);

   for(size_t i = y_size; i != x_size; ++i)
      z[i] = word_add(x[i], 0, &carry);

   return carry;
   }

/**
* Two operand addition
* @param x the first operand (and output)
* @param x_size size of x
* @param y the second operand
* @param y_size size of y (must be >= x_size)
*/
inline void bigint_add2(word x[], size_t x_size,
                        const word y[], size_t y_size)
   {
   x[x_size] += bigint_add2_nc(x, x_size, y, y_size);
   }

/**
* Three operand addition
*/
inline void bigint_add3(word z[],
                        const word x[], size_t x_size,
                        const word y[], size_t y_size)
   {
   z[x_size > y_size ? x_size : y_size] +=
      bigint_add3_nc(z, x, x_size, y, y_size);
   }

/**
* Two operand subtraction
*/
inline word bigint_sub2(word x[], size_t x_size,
                        const word y[], size_t y_size)
   {
   word borrow = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8)
      borrow = word8_sub2(x + i, y + i, borrow);

   for(size_t i = blocks; i != y_size; ++i)
      x[i] = word_sub(x[i], y[i], &borrow);

   for(size_t i = y_size; i != x_size; ++i)
      x[i] = word_sub(x[i], 0, &borrow);

   return borrow;
   }

/**
* Two operand subtraction, x = y - x; assumes y >= x
*/
inline void bigint_sub2_rev(word x[], const word y[], size_t y_size)
   {
   word borrow = 0;

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8)
      borrow = word8_sub2_rev(x + i, y + i, borrow);

   for(size_t i = blocks; i != y_size; ++i)
      x[i] = word_sub(y[i], x[i], &borrow);

   BOTAN_ASSERT(borrow == 0, "y must be greater than x");
   }

/**
* Three operand subtraction
*/
inline word bigint_sub3(word z[],
                        const word x[], size_t x_size,
                        const word y[], size_t y_size)
   {
   word borrow = 0;

   BOTAN_ASSERT(x_size >= y_size, "Expected sizes");

   const size_t blocks = y_size - (y_size % 8);

   for(size_t i = 0; i != blocks; i += 8)
      borrow = word8_sub3(z + i, x + i, y + i, borrow);

   for(size_t i = blocks; i != y_size; ++i)
      z[i] = word_sub(x[i], y[i], &borrow);

   for(size_t i = y_size; i != x_size; ++i)
      z[i] = word_sub(x[i], 0, &borrow);

   return borrow;
   }

/**
* Return abs(x-y), ie if x >= y, then compute z = x - y
* Otherwise compute z = y - x
* No borrow is possible since the result is always >= 0
*
* Returns ~0 if x >= y or 0 if x < y
* @param z output array of at least N words
* @param x input array of N words
* @param y input array of N words
* @param N length of x and y
* @param ws array of at least 2*N words
*/
inline CT::Mask<word>
bigint_sub_abs(word z[],
               const word x[], const word y[], size_t N,
               word ws[])
   {
   // Subtract in both direction then conditional copy out the result

   word* ws0 = ws;
   word* ws1 = ws + N;

   word borrow0 = 0;
   word borrow1 = 0;

   const size_t blocks = N - (N % 8);

   for(size_t i = 0; i != blocks; i += 8)
      {
      borrow0 = word8_sub3(ws0 + i, x + i, y + i, borrow0);
      borrow1 = word8_sub3(ws1 + i, y + i, x + i, borrow1);
      }

   for(size_t i = blocks; i != N; ++i)
      {
      ws0[i] = word_sub(x[i], y[i], &borrow0);
      ws1[i] = word_sub(y[i], x[i], &borrow1);
      }

   return CT::conditional_copy_mem(borrow0, z, ws1, ws0, N);
   }

/*
* Shift Operations
*/
inline void bigint_shl1(word x[], size_t x_size, size_t x_words,
                        size_t word_shift, size_t bit_shift)
   {
   copy_mem(x + word_shift, x, x_words);
   clear_mem(x, word_shift);

   const auto carry_mask = CT::Mask<word>::expand(bit_shift);
   const size_t carry_shift = carry_mask.if_set_return(BOTAN_MP_WORD_BITS - bit_shift);

   word carry = 0;
   for(size_t i = word_shift; i != x_size; ++i)
      {
      const word w = x[i];
      x[i] = (w << bit_shift) | carry;
      carry = carry_mask.if_set_return(w >> carry_shift);
      }
   }

inline void bigint_shr1(word x[], size_t x_size,
                        size_t word_shift, size_t bit_shift)
   {
   const size_t top = x_size >= word_shift ? (x_size - word_shift) : 0;

   if(top > 0)
      copy_mem(x, x + word_shift, top);
   clear_mem(x + top, std::min(word_shift, x_size));

   const auto carry_mask = CT::Mask<word>::expand(bit_shift);
   const size_t carry_shift = carry_mask.if_set_return(BOTAN_MP_WORD_BITS - bit_shift);

   word carry = 0;

   for(size_t i = 0; i != top; ++i)
      {
      const word w = x[top - i - 1];
      x[top-i-1] = (w >> bit_shift) | carry;
      carry = carry_mask.if_set_return(w << carry_shift);
      }
   }

inline void bigint_shl2(word y[], const word x[], size_t x_size,
                        size_t word_shift, size_t bit_shift)
   {
   copy_mem(y + word_shift, x, x_size);

   const auto carry_mask = CT::Mask<word>::expand(bit_shift);
   const size_t carry_shift = carry_mask.if_set_return(BOTAN_MP_WORD_BITS - bit_shift);

   word carry = 0;
   for(size_t i = word_shift; i != x_size + word_shift + 1; ++i)
      {
      const word w = y[i];
      y[i] = (w << bit_shift) | carry;
      carry = carry_mask.if_set_return(w >> carry_shift);
      }
   }

inline void bigint_shr2(word y[], const word x[], size_t x_size,
                        size_t word_shift, size_t bit_shift)
   {
   const size_t new_size = x_size < word_shift ? 0 : (x_size - word_shift);

   if(new_size > 0)
      copy_mem(y, x + word_shift, new_size);

   const auto carry_mask = CT::Mask<word>::expand(bit_shift);
   const size_t carry_shift = carry_mask.if_set_return(BOTAN_MP_WORD_BITS - bit_shift);

   word carry = 0;
   for(size_t i = new_size; i > 0; --i)
      {
      word w = y[i-1];
      y[i-1] = (w >> bit_shift) | carry;
      carry = carry_mask.if_set_return(w << carry_shift);
      }
   }

/*
* Linear Multiply - returns the carry
*/
inline word BOTAN_WARN_UNUSED_RESULT bigint_linmul2(word x[], size_t x_size, word y)
   {
   const size_t blocks = x_size - (x_size % 8);

   word carry = 0;

   for(size_t i = 0; i != blocks; i += 8)
      carry = word8_linmul2(x + i, y, carry);

   for(size_t i = blocks; i != x_size; ++i)
      x[i] = word_madd2(x[i], y, &carry);

   return carry;
   }

inline void bigint_linmul3(word z[], const word x[], size_t x_size, word y)
   {
   const size_t blocks = x_size - (x_size % 8);

   word carry = 0;

   for(size_t i = 0; i != blocks; i += 8)
      carry = word8_linmul3(z + i, x + i, y, carry);

   for(size_t i = blocks; i != x_size; ++i)
      z[i] = word_madd2(x[i], y, &carry);

   z[x_size] = carry;
   }

/**
* Compare x and y
* Return -1 if x < y
* Return 0 if x == y
* Return 1 if x > y
*/
inline int32_t bigint_cmp(const word x[], size_t x_size,
                          const word y[], size_t y_size)
   {
   static_assert(sizeof(word) >= sizeof(uint32_t), "Size assumption");

   const word LT = static_cast<word>(-1);
   const word EQ = 0;
   const word GT = 1;

   const size_t common_elems = std::min(x_size, y_size);

   word result = EQ; // until found otherwise

   for(size_t i = 0; i != common_elems; i++)
      {
      const auto is_eq = CT::Mask<word>::is_equal(x[i], y[i]);
      const auto is_lt = CT::Mask<word>::is_lt(x[i], y[i]);

      result = is_eq.select(result, is_lt.select(LT, GT));
      }

   if(x_size < y_size)
      {
      word mask = 0;
      for(size_t i = x_size; i != y_size; i++)
         mask |= y[i];

      // If any bits were set in high part of y, then x < y
      result = CT::Mask<word>::is_zero(mask).select(result, LT);
      }
   else if(y_size < x_size)
      {
      word mask = 0;
      for(size_t i = y_size; i != x_size; i++)
         mask |= x[i];

      // If any bits were set in high part of x, then x > y
      result = CT::Mask<word>::is_zero(mask).select(result, GT);
      }

   CT::unpoison(result);
   BOTAN_DEBUG_ASSERT(result == LT || result == GT || result == EQ);
   return static_cast<int32_t>(result);
   }

/**
* Compare x and y
* Return ~0 if x[0:x_size] < y[0:y_size] or 0 otherwise
* If lt_or_equal is true, returns ~0 also for x == y
*/
inline CT::Mask<word>
bigint_ct_is_lt(const word x[], size_t x_size,
                const word y[], size_t y_size,
                bool lt_or_equal = false)
   {
   const size_t common_elems = std::min(x_size, y_size);

   auto is_lt = CT::Mask<word>::expand(lt_or_equal);

   for(size_t i = 0; i != common_elems; i++)
      {
      const auto eq = CT::Mask<word>::is_equal(x[i], y[i]);
      const auto lt = CT::Mask<word>::is_lt(x[i], y[i]);
      is_lt = eq.select_mask(is_lt, lt);
      }

   if(x_size < y_size)
      {
      word mask = 0;
      for(size_t i = x_size; i != y_size; i++)
         mask |= y[i];
      // If any bits were set in high part of y, then is_lt should be forced true
      is_lt |= CT::Mask<word>::expand(mask);
      }
   else if(y_size < x_size)
      {
      word mask = 0;
      for(size_t i = y_size; i != x_size; i++)
         mask |= x[i];

      // If any bits were set in high part of x, then is_lt should be false
      is_lt &= CT::Mask<word>::is_zero(mask);
      }

   return is_lt;
   }

inline CT::Mask<word>
bigint_ct_is_eq(const word x[], size_t x_size,
                const word y[], size_t y_size)
   {
   const size_t common_elems = std::min(x_size, y_size);

   word diff = 0;

   for(size_t i = 0; i != common_elems; i++)
      {
      diff |= (x[i] ^ y[i]);
      }

   // If any bits were set in high part of x/y, then they are not equal
   if(x_size < y_size)
      {
      for(size_t i = x_size; i != y_size; i++)
         diff |= y[i];
      }
   else if(y_size < x_size)
      {
      for(size_t i = y_size; i != x_size; i++)
         diff |= x[i];
      }

   return CT::Mask<word>::is_zero(diff);
   }

/**
* Set z to abs(x-y), ie if x >= y, then compute z = x - y
* Otherwise compute z = y - x
* No borrow is possible since the result is always >= 0
*
* Return the relative size of x vs y (-1, 0, 1)
*
* @param z output array of max(x_size,y_size) words
* @param x input param
* @param x_size length of x
* @param y input param
* @param y_size length of y
*/
inline int32_t
bigint_sub_abs(word z[],
               const word x[], size_t x_size,
               const word y[], size_t y_size)
   {
   const int32_t relative_size = bigint_cmp(x, x_size, y, y_size);

   // Swap if relative_size == -1
   const bool need_swap = relative_size < 0;
   CT::conditional_swap_ptr(need_swap, x, y);
   CT::conditional_swap(need_swap, x_size, y_size);

   /*
   * We know at this point that x >= y so if y_size is larger than
   * x_size, we are guaranteed they are just leading zeros which can
   * be ignored
   */
   y_size = std::min(x_size, y_size);

   bigint_sub3(z, x, x_size, y, y_size);

   return relative_size;
   }

/**
* Set t to t-s modulo mod
*
* @param t first integer
* @param s second integer
* @param mod the modulus
* @param mod_sw size of t, s, and mod
* @param ws workspace of size mod_sw
*/
inline void
bigint_mod_sub(word t[], const word s[], const word mod[], size_t mod_sw, word ws[])
   {
   // is t < s or not?
   const auto is_lt = bigint_ct_is_lt(t, mod_sw, s, mod_sw);

   // ws = p - s
   const word borrow = bigint_sub3(ws, mod, mod_sw, s, mod_sw);

   // Compute either (t - s) or (t + (p - s)) depending on mask
   const word carry = bigint_cnd_addsub(is_lt, t, ws, s, mod_sw);

   BOTAN_DEBUG_ASSERT(borrow == 0 && carry == 0);
   BOTAN_UNUSED(carry, borrow);
   }

template<size_t N>
inline void bigint_mod_sub_n(word t[], const word s[], const word mod[], word ws[])
   {
   // is t < s or not?
   const auto is_lt = bigint_ct_is_lt(t, N, s, N);

   // ws = p - s
   const word borrow = bigint_sub3(ws, mod, N, s, N);

   // Compute either (t - s) or (t + (p - s)) depending on mask
   const word carry = bigint_cnd_addsub(is_lt, t, ws, s, N);

   BOTAN_DEBUG_ASSERT(borrow == 0 && carry == 0);
   BOTAN_UNUSED(carry, borrow);
   }

/**
* Compute ((n1<<bits) + n0) / d
*/
inline word bigint_divop(word n1, word n0, word d)
   {
   if(d == 0)
      throw Invalid_Argument("bigint_divop divide by zero");

#if defined(BOTAN_HAS_MP_DWORD)
   return ((static_cast<dword>(n1) << BOTAN_MP_WORD_BITS) | n0) / d;
#else

   word high = n1 % d;
   word quotient = 0;

   for(size_t i = 0; i != BOTAN_MP_WORD_BITS; ++i)
      {
      const word high_top_bit = high >> (BOTAN_MP_WORD_BITS-1);

      high <<= 1;
      high |= (n0 >> (BOTAN_MP_WORD_BITS-1-i)) & 1;
      quotient <<= 1;

      if(high_top_bit || high >= d)
         {
         high -= d;
         quotient |= 1;
         }
      }

   return quotient;
#endif
   }

/**
* Compute ((n1<<bits) + n0) % d
*/
inline word bigint_modop(word n1, word n0, word d)
   {
   if(d == 0)
      throw Invalid_Argument("bigint_modop divide by zero");

#if defined(BOTAN_HAS_MP_DWORD)
   return ((static_cast<dword>(n1) << BOTAN_MP_WORD_BITS) | n0) % d;
#else
   word z = bigint_divop(n1, n0, d);
   word dummy = 0;
   z = word_madd2(z, d, &dummy);
   return (n0-z);
#endif
   }

/*
* Comba Multiplication / Squaring
*/
void bigint_comba_mul4(word z[8], const word x[4], const word y[4]);
void bigint_comba_mul6(word z[12], const word x[6], const word y[6]);
void bigint_comba_mul8(word z[16], const word x[8], const word y[8]);
void bigint_comba_mul9(word z[18], const word x[9], const word y[9]);
void bigint_comba_mul16(word z[32], const word x[16], const word y[16]);
void bigint_comba_mul24(word z[48], const word x[24], const word y[24]);

void bigint_comba_sqr4(word out[8], const word in[4]);
void bigint_comba_sqr6(word out[12], const word in[6]);
void bigint_comba_sqr8(word out[16], const word in[8]);
void bigint_comba_sqr9(word out[18], const word in[9]);
void bigint_comba_sqr16(word out[32], const word in[16]);
void bigint_comba_sqr24(word out[48], const word in[24]);

/**
* Montgomery Reduction
* @param z integer to reduce, of size exactly 2*(p_size+1).
           Output is in the first p_size+1 words, higher
           words are set to zero.
* @param p modulus
* @param p_size size of p
* @param p_dash Montgomery value
* @param workspace array of at least 2*(p_size+1) words
* @param ws_size size of workspace in words
*/
void bigint_monty_redc(word z[],
                       const word p[], size_t p_size,
                       word p_dash,
                       word workspace[],
                       size_t ws_size);

/*
* High Level Multiplication/Squaring Interfaces
*/

void bigint_mul(word z[], size_t z_size,
                const word x[], size_t x_size, size_t x_sw,
                const word y[], size_t y_size, size_t y_sw,
                word workspace[], size_t ws_size);

void bigint_sqr(word z[], size_t z_size,
                const word x[], size_t x_size, size_t x_sw,
                word workspace[], size_t ws_size);

}

#endif
