/* lzo_func.h -- functions

   This file is part of the LZO real-time data compression library.

   Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the library and is subject
   to change.
 */


#ifndef __LZO_FUNC_H
#define __LZO_FUNC_H 1


/***********************************************************************
// bitops
************************************************************************/

#if !defined(LZO_BITOPS_USE_ASM_BITSCAN) && !defined(LZO_BITOPS_USE_GNUC_BITSCAN) && !defined(LZO_BITOPS_USE_MSC_BITSCAN)
#if 1 && (LZO_ARCH_AMD64) && (LZO_CC_GNUC && (LZO_CC_GNUC < 0x040000ul)) && (LZO_ASM_SYNTAX_GNUC)
#define LZO_BITOPS_USE_ASM_BITSCAN 1
#elif (LZO_CC_CLANG || (LZO_CC_GNUC >= 0x030400ul) || (LZO_CC_INTELC_GNUC && (__INTEL_COMPILER >= 1000)) || (LZO_CC_LLVM && (!defined(__llvm_tools_version__) || (__llvm_tools_version__+0 >= 0x010500ul))))
#define LZO_BITOPS_USE_GNUC_BITSCAN 1
#elif (LZO_OS_WIN32 || LZO_OS_WIN64) && ((LZO_CC_INTELC_MSC && (__INTEL_COMPILER >= 1010)) || (LZO_CC_MSC && (_MSC_VER >= 1400)))
#define LZO_BITOPS_USE_MSC_BITSCAN 1
#if (LZO_CC_MSC) && (LZO_ARCH_AMD64 || LZO_ARCH_I386)
#include <intrin.h>
#endif
#if (LZO_CC_MSC) && (LZO_ARCH_AMD64 || LZO_ARCH_I386)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#endif
#if (LZO_CC_MSC) && (LZO_ARCH_AMD64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#endif
#endif
#endif

__lzo_static_forceinline unsigned lzo_bitops_ctlz32_func(lzo_uint32_t v)
{
#if (LZO_BITOPS_USE_MSC_BITSCAN) && (LZO_ARCH_AMD64 || LZO_ARCH_I386)
    unsigned long r; (void) _BitScanReverse(&r, v); return (unsigned) r ^ 31;
#define lzo_bitops_ctlz32(v)    lzo_bitops_ctlz32_func(v)
#elif (LZO_BITOPS_USE_ASM_BITSCAN) && (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    lzo_uint32_t r;
    __asm__("bsr %1,%0" : "=r" (r) : "rm" (v) __LZO_ASM_CLOBBER_LIST_CC);
    return (unsigned) r ^ 31;
#define lzo_bitops_ctlz32(v)    lzo_bitops_ctlz32_func(v)
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_INT == 4)
    unsigned r; r = (unsigned) __builtin_clz(v); return r;
#define lzo_bitops_ctlz32(v)    ((unsigned) __builtin_clz(v))
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_LONG == 8) && (LZO_WORDSIZE >= 8)
    unsigned r; r = (unsigned) __builtin_clzl(v); return r ^ 32;
#define lzo_bitops_ctlz32(v)    (((unsigned) __builtin_clzl(v)) ^ 32)
#else
    LZO_UNUSED(v); return 0;
#endif
}

#if defined(lzo_uint64_t)
__lzo_static_forceinline unsigned lzo_bitops_ctlz64_func(lzo_uint64_t v)
{
#if (LZO_BITOPS_USE_MSC_BITSCAN) && (LZO_ARCH_AMD64)
    unsigned long r; (void) _BitScanReverse64(&r, v); return (unsigned) r ^ 63;
#define lzo_bitops_ctlz64(v)    lzo_bitops_ctlz64_func(v)
#elif (LZO_BITOPS_USE_ASM_BITSCAN) && (LZO_ARCH_AMD64) && (LZO_ASM_SYNTAX_GNUC)
    lzo_uint64_t r;
    __asm__("bsr %1,%0" : "=r" (r) : "rm" (v) __LZO_ASM_CLOBBER_LIST_CC);
    return (unsigned) r ^ 63;
#define lzo_bitops_ctlz64(v)    lzo_bitops_ctlz64_func(v)
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_LONG == 8) && (LZO_WORDSIZE >= 8)
    unsigned r; r = (unsigned) __builtin_clzl(v); return r;
#define lzo_bitops_ctlz64(v)    ((unsigned) __builtin_clzl(v))
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_LONG_LONG == 8) && (LZO_WORDSIZE >= 8)
    unsigned r; r = (unsigned) __builtin_clzll(v); return r;
#define lzo_bitops_ctlz64(v)    ((unsigned) __builtin_clzll(v))
#else
    LZO_UNUSED(v); return 0;
#endif
}
#endif

__lzo_static_forceinline unsigned lzo_bitops_cttz32_func(lzo_uint32_t v)
{
#if (LZO_BITOPS_USE_MSC_BITSCAN) && (LZO_ARCH_AMD64 || LZO_ARCH_I386)
    unsigned long r; (void) _BitScanForward(&r, v); return (unsigned) r;
#define lzo_bitops_cttz32(v)    lzo_bitops_cttz32_func(v)
#elif (LZO_BITOPS_USE_ASM_BITSCAN) && (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    lzo_uint32_t r;
    __asm__("bsf %1,%0" : "=r" (r) : "rm" (v) __LZO_ASM_CLOBBER_LIST_CC);
    return (unsigned) r;
#define lzo_bitops_cttz32(v)    lzo_bitops_cttz32_func(v)
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_INT >= 4)
    unsigned r; r = (unsigned) __builtin_ctz(v); return r;
#define lzo_bitops_cttz32(v)    ((unsigned) __builtin_ctz(v))
#else
    LZO_UNUSED(v); return 0;
#endif
}

#if defined(lzo_uint64_t)
__lzo_static_forceinline unsigned lzo_bitops_cttz64_func(lzo_uint64_t v)
{
#if (LZO_BITOPS_USE_MSC_BITSCAN) && (LZO_ARCH_AMD64)
    unsigned long r; (void) _BitScanForward64(&r, v); return (unsigned) r;
#define lzo_bitops_cttz64(v)    lzo_bitops_cttz64_func(v)
#elif (LZO_BITOPS_USE_ASM_BITSCAN) && (LZO_ARCH_AMD64) && (LZO_ASM_SYNTAX_GNUC)
    lzo_uint64_t r;
    __asm__("bsf %1,%0" : "=r" (r) : "rm" (v) __LZO_ASM_CLOBBER_LIST_CC);
    return (unsigned) r;
#define lzo_bitops_cttz64(v)    lzo_bitops_cttz64_func(v)
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_LONG >= 8) && (LZO_WORDSIZE >= 8)
    unsigned r; r = (unsigned) __builtin_ctzl(v); return r;
#define lzo_bitops_cttz64(v)    ((unsigned) __builtin_ctzl(v))
#elif (LZO_BITOPS_USE_GNUC_BITSCAN) && (LZO_SIZEOF_LONG_LONG >= 8) && (LZO_WORDSIZE >= 8)
    unsigned r; r = (unsigned) __builtin_ctzll(v); return r;
#define lzo_bitops_cttz64(v)    ((unsigned) __builtin_ctzll(v))
#else
    LZO_UNUSED(v); return 0;
#endif
}
#endif

lzo_unused_funcs_impl(void, lzo_bitops_unused_funcs)(void)
{
    LZO_UNUSED_FUNC(lzo_bitops_unused_funcs);
    LZO_UNUSED_FUNC(lzo_bitops_ctlz32_func);
    LZO_UNUSED_FUNC(lzo_bitops_cttz32_func);
#if defined(lzo_uint64_t)
    LZO_UNUSED_FUNC(lzo_bitops_ctlz64_func);
    LZO_UNUSED_FUNC(lzo_bitops_cttz64_func);
#endif
}


/***********************************************************************
// memops
************************************************************************/

#if defined(__lzo_alignof) && !(LZO_CFG_NO_UNALIGNED)
/* CBUG: disabled because of gcc bug 64516 */
#if !defined(lzo_memops_tcheck__) && 0
#define lzo_memops_tcheck__(t,a,b) ((void)0, sizeof(t) == (a) && __lzo_alignof(t) == (b))
#endif
#endif
#ifndef lzo_memops_TU0p
#define lzo_memops_TU0p void __LZO_MMODEL *
#endif
#ifndef lzo_memops_TU1p
#define lzo_memops_TU1p unsigned char __LZO_MMODEL *
#endif
#ifndef lzo_memops_TU2p
#if (LZO_OPT_UNALIGNED16)
typedef lzo_uint16_t __lzo_may_alias lzo_memops_TU2;
#define lzo_memops_TU2p volatile lzo_memops_TU2 *
#elif defined(__lzo_byte_struct)
__lzo_byte_struct(lzo_memops_TU2_struct,2)
typedef struct lzo_memops_TU2_struct lzo_memops_TU2;
#else
struct lzo_memops_TU2_struct { unsigned char a[2]; } __lzo_may_alias;
typedef struct lzo_memops_TU2_struct lzo_memops_TU2;
#endif
#ifndef lzo_memops_TU2p
#define lzo_memops_TU2p lzo_memops_TU2 *
#endif
#endif
#ifndef lzo_memops_TU4p
#if (LZO_OPT_UNALIGNED32)
typedef lzo_uint32_t __lzo_may_alias lzo_memops_TU4;
#define lzo_memops_TU4p volatile lzo_memops_TU4 __LZO_MMODEL *
#elif defined(__lzo_byte_struct)
__lzo_byte_struct(lzo_memops_TU4_struct,4)
typedef struct lzo_memops_TU4_struct lzo_memops_TU4;
#else
struct lzo_memops_TU4_struct { unsigned char a[4]; } __lzo_may_alias;
typedef struct lzo_memops_TU4_struct lzo_memops_TU4;
#endif
#ifndef lzo_memops_TU4p
#define lzo_memops_TU4p lzo_memops_TU4 __LZO_MMODEL *
#endif
#endif
#ifndef lzo_memops_TU8p
#if (LZO_OPT_UNALIGNED64)
typedef lzo_uint64_t __lzo_may_alias lzo_memops_TU8;
#define lzo_memops_TU8p volatile lzo_memops_TU8 __LZO_MMODEL *
#elif defined(__lzo_byte_struct)
__lzo_byte_struct(lzo_memops_TU8_struct,8)
typedef struct lzo_memops_TU8_struct lzo_memops_TU8;
#else
struct lzo_memops_TU8_struct { unsigned char a[8]; } __lzo_may_alias;
typedef struct lzo_memops_TU8_struct lzo_memops_TU8;
#endif
#ifndef lzo_memops_TU8p
#define lzo_memops_TU8p lzo_memops_TU8 __LZO_MMODEL *
#endif
#endif
#ifndef lzo_memops_set_TU1p
#define lzo_memops_set_TU1p     volatile lzo_memops_TU1p
#endif
#ifndef lzo_memops_move_TU1p
#define lzo_memops_move_TU1p    lzo_memops_TU1p
#endif
#define LZO_MEMOPS_SET1(dd,cc) \
    LZO_BLOCK_BEGIN \
    lzo_memops_set_TU1p d__1 = (lzo_memops_set_TU1p) (lzo_memops_TU0p) (dd); \
    d__1[0] = LZO_BYTE(cc); \
    LZO_BLOCK_END
#define LZO_MEMOPS_SET2(dd,cc) \
    LZO_BLOCK_BEGIN \
    lzo_memops_set_TU1p d__2 = (lzo_memops_set_TU1p) (lzo_memops_TU0p) (dd); \
    d__2[0] = LZO_BYTE(cc); d__2[1] = LZO_BYTE(cc); \
    LZO_BLOCK_END
#define LZO_MEMOPS_SET3(dd,cc) \
    LZO_BLOCK_BEGIN \
    lzo_memops_set_TU1p d__3 = (lzo_memops_set_TU1p) (lzo_memops_TU0p) (dd); \
    d__3[0] = LZO_BYTE(cc); d__3[1] = LZO_BYTE(cc); d__3[2] = LZO_BYTE(cc); \
    LZO_BLOCK_END
#define LZO_MEMOPS_SET4(dd,cc) \
    LZO_BLOCK_BEGIN \
    lzo_memops_set_TU1p d__4 = (lzo_memops_set_TU1p) (lzo_memops_TU0p) (dd); \
    d__4[0] = LZO_BYTE(cc); d__4[1] = LZO_BYTE(cc); d__4[2] = LZO_BYTE(cc); d__4[3] = LZO_BYTE(cc); \
    LZO_BLOCK_END
#define LZO_MEMOPS_MOVE1(dd,ss) \
    LZO_BLOCK_BEGIN \
    lzo_memops_move_TU1p d__1 = (lzo_memops_move_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_move_TU1p s__1 = (const lzo_memops_move_TU1p) (const lzo_memops_TU0p) (ss); \
    d__1[0] = s__1[0]; \
    LZO_BLOCK_END
#define LZO_MEMOPS_MOVE2(dd,ss) \
    LZO_BLOCK_BEGIN \
    lzo_memops_move_TU1p d__2 = (lzo_memops_move_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_move_TU1p s__2 = (const lzo_memops_move_TU1p) (const lzo_memops_TU0p) (ss); \
    d__2[0] = s__2[0]; d__2[1] = s__2[1]; \
    LZO_BLOCK_END
#define LZO_MEMOPS_MOVE3(dd,ss) \
    LZO_BLOCK_BEGIN \
    lzo_memops_move_TU1p d__3 = (lzo_memops_move_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_move_TU1p s__3 = (const lzo_memops_move_TU1p) (const lzo_memops_TU0p) (ss); \
    d__3[0] = s__3[0]; d__3[1] = s__3[1]; d__3[2] = s__3[2]; \
    LZO_BLOCK_END
#define LZO_MEMOPS_MOVE4(dd,ss) \
    LZO_BLOCK_BEGIN \
    lzo_memops_move_TU1p d__4 = (lzo_memops_move_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_move_TU1p s__4 = (const lzo_memops_move_TU1p) (const lzo_memops_TU0p) (ss); \
    d__4[0] = s__4[0]; d__4[1] = s__4[1]; d__4[2] = s__4[2]; d__4[3] = s__4[3]; \
    LZO_BLOCK_END
#define LZO_MEMOPS_MOVE8(dd,ss) \
    LZO_BLOCK_BEGIN \
    lzo_memops_move_TU1p d__8 = (lzo_memops_move_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_move_TU1p s__8 = (const lzo_memops_move_TU1p) (const lzo_memops_TU0p) (ss); \
    d__8[0] = s__8[0]; d__8[1] = s__8[1]; d__8[2] = s__8[2]; d__8[3] = s__8[3]; \
    d__8[4] = s__8[4]; d__8[5] = s__8[5]; d__8[6] = s__8[6]; d__8[7] = s__8[7]; \
    LZO_BLOCK_END
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU1p)0)==1)
#define LZO_MEMOPS_COPY1(dd,ss) LZO_MEMOPS_MOVE1(dd,ss)
#if (LZO_OPT_UNALIGNED16)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU2p)0)==2)
#define LZO_MEMOPS_COPY2(dd,ss) \
    * (lzo_memops_TU2p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU2p) (const lzo_memops_TU0p) (ss)
#elif defined(lzo_memops_tcheck__)
#define LZO_MEMOPS_COPY2(dd,ss) \
    LZO_BLOCK_BEGIN if (lzo_memops_tcheck__(lzo_memops_TU2,2,1)) { \
        * (lzo_memops_TU2p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU2p) (const lzo_memops_TU0p) (ss); \
    } else { LZO_MEMOPS_MOVE2(dd,ss); } LZO_BLOCK_END
#else
#define LZO_MEMOPS_COPY2(dd,ss) LZO_MEMOPS_MOVE2(dd,ss)
#endif
#if (LZO_OPT_UNALIGNED32)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU4p)0)==4)
#define LZO_MEMOPS_COPY4(dd,ss) \
    * (lzo_memops_TU4p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU4p) (const lzo_memops_TU0p) (ss)
#elif defined(lzo_memops_tcheck__)
#define LZO_MEMOPS_COPY4(dd,ss) \
    LZO_BLOCK_BEGIN if (lzo_memops_tcheck__(lzo_memops_TU4,4,1)) { \
        * (lzo_memops_TU4p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU4p) (const lzo_memops_TU0p) (ss); \
    } else { LZO_MEMOPS_MOVE4(dd,ss); } LZO_BLOCK_END
#else
#define LZO_MEMOPS_COPY4(dd,ss) LZO_MEMOPS_MOVE4(dd,ss)
#endif
#if (LZO_WORDSIZE != 8)
#define LZO_MEMOPS_COPY8(dd,ss) \
    LZO_BLOCK_BEGIN LZO_MEMOPS_COPY4(dd,ss); LZO_MEMOPS_COPY4((lzo_memops_TU1p)(lzo_memops_TU0p)(dd)+4,(const lzo_memops_TU1p)(const lzo_memops_TU0p)(ss)+4); LZO_BLOCK_END
#else
#if (LZO_OPT_UNALIGNED64)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU8p)0)==8)
#define LZO_MEMOPS_COPY8(dd,ss) \
    * (lzo_memops_TU8p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU8p) (const lzo_memops_TU0p) (ss)
#elif (LZO_OPT_UNALIGNED32)
#define LZO_MEMOPS_COPY8(dd,ss) \
    LZO_BLOCK_BEGIN LZO_MEMOPS_COPY4(dd,ss); LZO_MEMOPS_COPY4((lzo_memops_TU1p)(lzo_memops_TU0p)(dd)+4,(const lzo_memops_TU1p)(const lzo_memops_TU0p)(ss)+4); LZO_BLOCK_END
#elif defined(lzo_memops_tcheck__)
#define LZO_MEMOPS_COPY8(dd,ss) \
    LZO_BLOCK_BEGIN if (lzo_memops_tcheck__(lzo_memops_TU8,8,1)) { \
        * (lzo_memops_TU8p) (lzo_memops_TU0p) (dd) = * (const lzo_memops_TU8p) (const lzo_memops_TU0p) (ss); \
    } else { LZO_MEMOPS_MOVE8(dd,ss); } LZO_BLOCK_END
#else
#define LZO_MEMOPS_COPY8(dd,ss) LZO_MEMOPS_MOVE8(dd,ss)
#endif
#endif
#define LZO_MEMOPS_COPYN(dd,ss,nn) \
    LZO_BLOCK_BEGIN \
    lzo_memops_TU1p d__n = (lzo_memops_TU1p) (lzo_memops_TU0p) (dd); \
    const lzo_memops_TU1p s__n = (const lzo_memops_TU1p) (const lzo_memops_TU0p) (ss); \
    lzo_uint n__n = (nn); \
    while ((void)0, n__n >= 8) { LZO_MEMOPS_COPY8(d__n, s__n); d__n += 8; s__n += 8; n__n -= 8; } \
    if ((void)0, n__n >= 4) { LZO_MEMOPS_COPY4(d__n, s__n); d__n += 4; s__n += 4; n__n -= 4; } \
    if ((void)0, n__n > 0) do { *d__n++ = *s__n++; } while (--n__n > 0); \
    LZO_BLOCK_END

__lzo_static_forceinline lzo_uint16_t lzo_memops_get_le16(const lzo_voidp ss)
{
    lzo_uint16_t v;
#if (LZO_ABI_LITTLE_ENDIAN)
    LZO_MEMOPS_COPY2(&v, ss);
#elif (LZO_OPT_UNALIGNED16 && LZO_ARCH_POWERPC && LZO_ABI_BIG_ENDIAN) && (LZO_ASM_SYNTAX_GNUC)
    const lzo_memops_TU2p s = (const lzo_memops_TU2p) ss;
    unsigned long vv;
    __asm__("lhbrx %0,0,%1" : "=r" (vv) : "r" (s), "m" (*s));
    v = (lzo_uint16_t) vv;
#else
    const lzo_memops_TU1p s = (const lzo_memops_TU1p) ss;
    v = (lzo_uint16_t) (((lzo_uint16_t)s[0]) | ((lzo_uint16_t)s[1] << 8));
#endif
    return v;
}
#if (LZO_OPT_UNALIGNED16) && (LZO_ABI_LITTLE_ENDIAN)
#define LZO_MEMOPS_GET_LE16(ss)    (* (const lzo_memops_TU2p) (const lzo_memops_TU0p) (ss))
#else
#define LZO_MEMOPS_GET_LE16(ss)    lzo_memops_get_le16(ss)
#endif

__lzo_static_forceinline lzo_uint32_t lzo_memops_get_le32(const lzo_voidp ss)
{
    lzo_uint32_t v;
#if (LZO_ABI_LITTLE_ENDIAN)
    LZO_MEMOPS_COPY4(&v, ss);
#elif (LZO_OPT_UNALIGNED32 && LZO_ARCH_POWERPC && LZO_ABI_BIG_ENDIAN) && (LZO_ASM_SYNTAX_GNUC)
    const lzo_memops_TU4p s = (const lzo_memops_TU4p) ss;
    unsigned long vv;
    __asm__("lwbrx %0,0,%1" : "=r" (vv) : "r" (s), "m" (*s));
    v = (lzo_uint32_t) vv;
#else
    const lzo_memops_TU1p s = (const lzo_memops_TU1p) ss;
    v = (lzo_uint32_t) (((lzo_uint32_t)s[0]) | ((lzo_uint32_t)s[1] << 8) | ((lzo_uint32_t)s[2] << 16) | ((lzo_uint32_t)s[3] << 24));
#endif
    return v;
}
#if (LZO_OPT_UNALIGNED32) && (LZO_ABI_LITTLE_ENDIAN)
#define LZO_MEMOPS_GET_LE32(ss)    (* (const lzo_memops_TU4p) (const lzo_memops_TU0p) (ss))
#else
#define LZO_MEMOPS_GET_LE32(ss)    lzo_memops_get_le32(ss)
#endif

#if (LZO_OPT_UNALIGNED64) && (LZO_ABI_LITTLE_ENDIAN)
#define LZO_MEMOPS_GET_LE64(ss)    (* (const lzo_memops_TU8p) (const lzo_memops_TU0p) (ss))
#endif

__lzo_static_forceinline lzo_uint16_t lzo_memops_get_ne16(const lzo_voidp ss)
{
    lzo_uint16_t v;
    LZO_MEMOPS_COPY2(&v, ss);
    return v;
}
#if (LZO_OPT_UNALIGNED16)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU2p)0)==2)
#define LZO_MEMOPS_GET_NE16(ss)    (* (const lzo_memops_TU2p) (const lzo_memops_TU0p) (ss))
#else
#define LZO_MEMOPS_GET_NE16(ss)    lzo_memops_get_ne16(ss)
#endif

__lzo_static_forceinline lzo_uint32_t lzo_memops_get_ne32(const lzo_voidp ss)
{
    lzo_uint32_t v;
    LZO_MEMOPS_COPY4(&v, ss);
    return v;
}
#if (LZO_OPT_UNALIGNED32)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU4p)0)==4)
#define LZO_MEMOPS_GET_NE32(ss)    (* (const lzo_memops_TU4p) (const lzo_memops_TU0p) (ss))
#else
#define LZO_MEMOPS_GET_NE32(ss)    lzo_memops_get_ne32(ss)
#endif

#if (LZO_OPT_UNALIGNED64)
LZO_COMPILE_TIME_ASSERT_HEADER(sizeof(*(lzo_memops_TU8p)0)==8)
#define LZO_MEMOPS_GET_NE64(ss)    (* (const lzo_memops_TU8p) (const lzo_memops_TU0p) (ss))
#endif

__lzo_static_forceinline void lzo_memops_put_le16(lzo_voidp dd, lzo_uint16_t vv)
{
#if (LZO_ABI_LITTLE_ENDIAN)
    LZO_MEMOPS_COPY2(dd, &vv);
#elif (LZO_OPT_UNALIGNED16 && LZO_ARCH_POWERPC && LZO_ABI_BIG_ENDIAN) && (LZO_ASM_SYNTAX_GNUC)
    lzo_memops_TU2p d = (lzo_memops_TU2p) dd;
    unsigned long v = vv;
    __asm__("sthbrx %2,0,%1" : "=m" (*d) : "r" (d), "r" (v));
#else
    lzo_memops_TU1p d = (lzo_memops_TU1p) dd;
    d[0] = LZO_BYTE((vv      ) & 0xff);
    d[1] = LZO_BYTE((vv >>  8) & 0xff);
#endif
}
#if (LZO_OPT_UNALIGNED16) && (LZO_ABI_LITTLE_ENDIAN)
#define LZO_MEMOPS_PUT_LE16(dd,vv) (* (lzo_memops_TU2p) (lzo_memops_TU0p) (dd) = (vv))
#else
#define LZO_MEMOPS_PUT_LE16(dd,vv) lzo_memops_put_le16(dd,vv)
#endif

__lzo_static_forceinline void lzo_memops_put_le32(lzo_voidp dd, lzo_uint32_t vv)
{
#if (LZO_ABI_LITTLE_ENDIAN)
    LZO_MEMOPS_COPY4(dd, &vv);
#elif (LZO_OPT_UNALIGNED32 && LZO_ARCH_POWERPC && LZO_ABI_BIG_ENDIAN) && (LZO_ASM_SYNTAX_GNUC)
    lzo_memops_TU4p d = (lzo_memops_TU4p) dd;
    unsigned long v = vv;
    __asm__("stwbrx %2,0,%1" : "=m" (*d) : "r" (d), "r" (v));
#else
    lzo_memops_TU1p d = (lzo_memops_TU1p) dd;
    d[0] = LZO_BYTE((vv      ) & 0xff);
    d[1] = LZO_BYTE((vv >>  8) & 0xff);
    d[2] = LZO_BYTE((vv >> 16) & 0xff);
    d[3] = LZO_BYTE((vv >> 24) & 0xff);
#endif
}
#if (LZO_OPT_UNALIGNED32) && (LZO_ABI_LITTLE_ENDIAN)
#define LZO_MEMOPS_PUT_LE32(dd,vv) (* (lzo_memops_TU4p) (lzo_memops_TU0p) (dd) = (vv))
#else
#define LZO_MEMOPS_PUT_LE32(dd,vv) lzo_memops_put_le32(dd,vv)
#endif

__lzo_static_forceinline void lzo_memops_put_ne16(lzo_voidp dd, lzo_uint16_t vv)
{
    LZO_MEMOPS_COPY2(dd, &vv);
}
#if (LZO_OPT_UNALIGNED16)
#define LZO_MEMOPS_PUT_NE16(dd,vv) (* (lzo_memops_TU2p) (lzo_memops_TU0p) (dd) = (vv))
#else
#define LZO_MEMOPS_PUT_NE16(dd,vv) lzo_memops_put_ne16(dd,vv)
#endif

__lzo_static_forceinline void lzo_memops_put_ne32(lzo_voidp dd, lzo_uint32_t vv)
{
    LZO_MEMOPS_COPY4(dd, &vv);
}
#if (LZO_OPT_UNALIGNED32)
#define LZO_MEMOPS_PUT_NE32(dd,vv) (* (lzo_memops_TU4p) (lzo_memops_TU0p) (dd) = (vv))
#else
#define LZO_MEMOPS_PUT_NE32(dd,vv) lzo_memops_put_ne32(dd,vv)
#endif

lzo_unused_funcs_impl(void, lzo_memops_unused_funcs)(void)
{
    LZO_UNUSED_FUNC(lzo_memops_unused_funcs);
    LZO_UNUSED_FUNC(lzo_memops_get_le16);
    LZO_UNUSED_FUNC(lzo_memops_get_le32);
    LZO_UNUSED_FUNC(lzo_memops_get_ne16);
    LZO_UNUSED_FUNC(lzo_memops_get_ne32);
    LZO_UNUSED_FUNC(lzo_memops_put_le16);
    LZO_UNUSED_FUNC(lzo_memops_put_le32);
    LZO_UNUSED_FUNC(lzo_memops_put_ne16);
    LZO_UNUSED_FUNC(lzo_memops_put_ne32);
}

#endif /* already included */

/* vim:set ts=4 sw=4 et: */
