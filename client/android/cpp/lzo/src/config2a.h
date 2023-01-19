/* config2a.h -- configuration for the LZO2A algorithm

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


#ifndef __LZO_CONFIG2A_H
#define __LZO_CONFIG2A_H 1

#include "lzo_conf.h"
#include <lzo/lzo2a.h>


/***********************************************************************
// algorithm configuration
************************************************************************/

/* dictionary depth (0 - 6) - this only affects the compressor.
 * 0 is fastest, 6 is best compression ratio */
#if !defined(DDBITS)
#  define DDBITS    0
#endif

/* compression level (1 - 9) - this only affects the compressor.
 * 1 is fastest, 9 is best compression ratio */
#if !defined(CLEVEL)
#  define CLEVEL    1           /* fastest by default */
#endif


/* check configuration */
#if (DDBITS < 0 || DDBITS > 6)
#  error "invalid DDBITS"
#endif
#if (CLEVEL < 1 || CLEVEL > 9)
#  error "invalid CLEVEL"
#endif


/***********************************************************************
// internal configuration
************************************************************************/

#if 1
#define SWD_N        8191           /* size of ring buffer */
#else
#define SWD_N       16383           /* size of ring buffer */
#endif

#define M1_MIN_LEN  2
#define M1_MAX_LEN  5
#define M2_MIN_LEN  3
#define M3_MIN_LEN  3


/* add a special code so that the decompressor can detect the
 * end of the compressed data block (overhead is 3 bytes per block) */
#define LZO_EOF_CODE 1

#undef LZO_DETERMINISTIC


/***********************************************************************
// algorithm internal configuration
************************************************************************/

/* choose the hashing strategy */
#ifndef LZO_HASH
#define LZO_HASH        LZO_HASH_LZO_INCREMENTAL_A
#endif

/* config */
#define DD_BITS         DDBITS
#ifndef D_BITS
#define D_BITS          14
#endif



/***********************************************************************
// optimization and debugging
************************************************************************/

/* Collect statistics */
#if 0 && !defined(LZO_COLLECT_STATS)
#  define LZO_COLLECT_STATS 1
#endif


/***********************************************************************
//
************************************************************************/

/* get bits */
#define _NEEDBITS \
    { _NEEDBYTE; b |= ((lzo_uint32_t) _NEXTBYTE) << k; k += 8; assert(k <= 32); }
#define NEEDBITS(j)     { assert((j) < 8); if (k < (j)) _NEEDBITS }

/* set bits */
#define SETBITS(j,x)    { b |= ((lzo_uint32_t)(x)) << k; k += (j); assert(k <= 32); }

/* access bits */
#define MASKBITS(j)     (b & ((((lzo_uint32_t)1 << (j)) - 1)))

/* drop bits */
#define DUMPBITS(j)     { assert(k >= j); b >>= (j); k -= (j); }



#endif /* already included */


/* vim:set ts=4 sw=4 et: */
