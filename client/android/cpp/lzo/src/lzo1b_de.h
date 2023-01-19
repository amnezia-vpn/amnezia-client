/* lzo1b_de.h -- definitions for the the LZO1B/LZO1C algorithm

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


#ifndef __LZO_DEFS_H
#define __LZO_DEFS_H 1

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
//
************************************************************************/

/*
     Format of the marker byte

     76543210
     --------
     00000000   R0 - a long literal run ('R0' run)
     000rrrrr   R  - a short literal run with len r
     00100000   M4 - a very long match
     001mmmmm   M3 - a long match  (len = m+M3_MIN_LEN)
     mmmooooo   M2 - a short match (len = m+M2_MIN_LEN, o = offset low bits)

                M1 is not used !
*/


#ifndef R_BITS
#define R_BITS              (5)
#endif


#ifndef M1L_BITS
#define M1L_BITS            (0)
#endif
#ifndef M2L_BITS
#define M2L_BITS            (CHAR_BIT - M2O_BITS)
#endif
#ifndef M3L_BITS
#define M3L_BITS            (R_BITS)
#endif
#ifndef M4L_BITS
#define M4L_BITS            (CHAR_BIT)
#endif

#ifndef M1O_BITS
#define M1O_BITS            (6)
#endif
#ifndef M2O_BITS
#define M2O_BITS            (R_BITS)
#endif
#ifndef M3O_BITS
#define M3O_BITS            (CHAR_BIT)
#endif
#ifndef M4O_BITS
#define M4O_BITS            (M3O_BITS)              /* must be the same */
#endif

#ifndef M1X_BITS
#define M1X_BITS            (M1O_BITS)
#endif
#ifndef M2X_BITS
#define M2X_BITS            (M2O_BITS + CHAR_BIT)
#endif
#ifndef M3X_BITS
#define M3X_BITS            (M3O_BITS + CHAR_BIT)
#endif
#ifndef M4X_BITS
#define M4X_BITS            M3X_BITS
#endif


#define __MIN_OFFSET(bits)  1
#define __MAX_OFFSET(bits)  LZO_SIZE(bits)

#define M1_MIN_OFFSET       __MIN_OFFSET(M1X_BITS)
#define M2_MIN_OFFSET       __MIN_OFFSET(M2X_BITS)
#define M3_MIN_OFFSET       __MIN_OFFSET(M3X_BITS)
#define M4_MIN_OFFSET       M3_MIN_OFFSET

#if defined(LZO_EOF_CODE) && !defined(M3_EOF_OFFSET)
#define M3_EOF_OFFSET       1
#else
#define M3_EOF_OFFSET       0
#endif

#ifndef _M1_MAX_OFFSET
#define _M1_MAX_OFFSET      __MAX_OFFSET(M1X_BITS)
#endif
#ifndef _M2_MAX_OFFSET
#define _M2_MAX_OFFSET      __MAX_OFFSET(M2X_BITS)
#endif
#ifndef _M3_MAX_OFFSET
#define _M3_MAX_OFFSET      (__MAX_OFFSET(M3X_BITS) - M3_EOF_OFFSET)
#endif
#ifndef _M4_MAX_OFFSET
#define _M4_MAX_OFFSET      _M3_MAX_OFFSET
#endif
#ifndef _MAX_OFFSET
#define _MAX_OFFSET         _M4_MAX_OFFSET
#endif

#if (M3_EOF_OFFSET > 0) && (_M2_MAX_OFFSET == _M3_MAX_OFFSET + M3_EOF_OFFSET)
#  undef _M2_MAX_OFFSET
#  define _M2_MAX_OFFSET    _M3_MAX_OFFSET
#endif
#if (_M2_MAX_OFFSET > _M3_MAX_OFFSET)
#  error
#endif

#define M1_MAX_OFFSET       ((lzo_uint) _M1_MAX_OFFSET)
#define M2_MAX_OFFSET       ((lzo_uint) _M2_MAX_OFFSET)
#define M3_MAX_OFFSET       ((lzo_uint) _M3_MAX_OFFSET)
#define M4_MAX_OFFSET       ((lzo_uint) _M4_MAX_OFFSET)
#define MAX_OFFSET          ((lzo_uint) _MAX_OFFSET)


#ifndef M1_MIN_LEN
#define M1_MIN_LEN          (2)
#endif
#ifndef M2_MIN_LEN
#define M2_MIN_LEN          (3)
#endif
#ifndef M3_MIN_LEN
#if (M3X_BITS == M2X_BITS)
#define M3_MIN_LEN          (M2_MAX_LEN + 1)
#else
#define M3_MIN_LEN          (4)
#endif
#endif
#ifndef M4_MIN_LEN
#define M4_MIN_LEN          (M3_MAX_LEN + 1)
#endif

#ifndef M1_MAX_LEN
#define M1_MAX_LEN          (M1_MIN_LEN + LZO_SIZE(M1L_BITS) - 1)
#endif
#ifndef M2_MAX_LEN
#define M2_MAX_LEN          (M2_MIN_LEN + LZO_SIZE(M2L_BITS) - 3)
#endif
#ifndef M3_MAX_LEN
#define M3_MAX_LEN          (M3_MIN_LEN + LZO_SIZE(M3L_BITS) - 2)
#endif
#ifndef M4_MAX_LEN
#define M4_MAX_LEN          (ULONG_MAX)
#endif


#define M1O_MASK            LZO_MASK(M1O_BITS)
#define M1L_MASK            LZO_MASK(M1L_BITS)
#define M2O_MASK            LZO_MASK(M2O_BITS)
#define M2L_MASK            LZO_MASK(M2L_BITS)
#define M3O_MASK            LZO_MASK(M3O_BITS)
#define M3L_MASK            LZO_MASK(M3L_BITS)
#define M4O_MASK            LZO_MASK(M4O_BITS)
#define M4L_MASK            LZO_MASK(M4L_BITS)


#define M1_MARKER           (1 << M1O_BITS)
#define M2_MARKER           (2 << M2O_BITS)
#define M3_MARKER           (1 << M3L_BITS)
#define M4_MARKER           M3_MARKER


/***********************************************************************
// R0 literal run (a long run)
************************************************************************/

#ifndef R0MIN
#define R0MIN   (LZO_SIZE(R_BITS))  /* Minimum len of R0 run of literals */
#endif
#define R0MAX   (R0MIN + 256 - 1)   /* Maximum len of R0 run of literals */

#if (R0MAX - (R0MAX & ~7u) >= 7)
#define R0FAST  (R0MAX & ~7u)       /* R0MAX aligned to 8 byte boundary */
#else
#define R0FAST  (R0MAX & ~15u)      /* R0MAX aligned to 8 byte boundary */
#endif

#if (R0MAX - R0FAST < 7) || ((R0FAST & 7) != 0)
#  error "something went wrong"
#endif
#if (R0FAST * 2 < 512)
#  error "R0FAST is not big enough"
#endif

/* 7 special codes from R0FAST+1 .. R0MAX
 * these codes mean long R0 runs with lengths
 * 512, 1024, 2048, 4096, 8192, 16384, 32768
 */



/***********************************************************************
// matching
************************************************************************/

#define PS  *m_pos++ != *ip++


/* We already matched M2_MIN_LEN bytes.
 * Try to match another M2_MAX_LEN - M2_MIN_LEN bytes. */

#if (M2_MAX_LEN - M2_MIN_LEN == 4)
#  define MATCH_M2X     (PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 5)
#  define MATCH_M2X     (PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 6)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 7)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 13)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 14)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 16)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS || PS || PS || PS)
#elif (M2_MAX_LEN - M2_MIN_LEN == 29)
#  define MATCH_M2X     (PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS || PS || PS || PS || \
                         PS || PS || PS || PS || PS)
#else
#  error "MATCH_M2X not yet implemented"
#endif


/* We already matched M2_MIN_LEN bytes.
 * Try to match another M2_MAX_LEN + 1 - M2_MIN_LEN bytes
 * to see if we get more than a M2 match */

#define MATCH_M2        (MATCH_M2X || PS)


/***********************************************************************
// copying
************************************************************************/

#define _CP             *op++ = *m_pos++

#if (M2_MIN_LEN == 2)
#  define COPY_M2X      _CP
#elif (M2_MIN_LEN == 3)
#  define COPY_M2X      _CP; _CP
#elif (M2_MIN_LEN == 4)
#  define COPY_M2X      _CP; _CP; _CP
#else
#  error "COPY_M2X not yet implemented"
#endif

#if (M3_MIN_LEN == 3)
#  define COPY_M3X      _CP; _CP
#elif (M3_MIN_LEN == 4)
#  define COPY_M3X      _CP; _CP; _CP
#elif (M3_MIN_LEN == 9)
#  define COPY_M3X      _CP; _CP; _CP; _CP; _CP; _CP; _CP; _CP
#else
#  error "COPY_M3X not yet implemented"
#endif

#define COPY_M2         COPY_M2X; *op++ = *m_pos++
#define COPY_M3         COPY_M3X; *op++ = *m_pos++


/***********************************************************************
//
************************************************************************/

#if defined(LZO_NEED_DICT_H)

#define DL_MIN_LEN          M2_MIN_LEN
#define D_INDEX1(d,p)       d = DM(DMUL(0x21,DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)       d = (d & (D_MASK & 0x7ff)) ^ (D_HIGH | 0x1f)
#include "lzo_dict.h"

#ifndef MIN_LOOKAHEAD
#define MIN_LOOKAHEAD       (M2_MAX_LEN + 1)
#endif
#ifndef MAX_LOOKBEHIND
#define MAX_LOOKBEHIND      (MAX_OFFSET)
#endif

#endif /* defined(LZO_NEED_DICT_H) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 sw=4 et: */
