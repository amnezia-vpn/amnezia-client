/* config1b.h -- configuration for the LZO1B algorithm

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


#ifndef __LZO_CONFIG1B_H
#define __LZO_CONFIG1B_H 1

#include "lzo_conf.h"
#include <lzo/lzo1b.h>


/***********************************************************************
// algorithm configuration
************************************************************************/

/* run bits (4 - 5) - the compressor and the decompressor
 * must use the same value. */
#if !defined(RBITS)
#  define RBITS     5
#endif

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
#if (RBITS < 4 || RBITS > 5)
#  error "invalid RBITS"
#endif
#if (DDBITS < 0 || DDBITS > 6)
#  error "invalid DDBITS"
#endif
#if (CLEVEL < 1 || CLEVEL > 9)
#  error "invalid CLEVEL"
#endif


/***********************************************************************
// internal configuration
************************************************************************/

/* add a special code so that the decompressor can detect the
 * end of the compressed data block (overhead is 3 bytes per block) */
#define LZO_EOF_CODE 1


/***********************************************************************
// algorithm internal configuration
************************************************************************/

/* choose the hashing strategy */
#ifndef LZO_HASH
#define LZO_HASH        LZO_HASH_LZO_INCREMENTAL_A
#endif

/* config */
#define R_BITS          RBITS
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

#include "lzo1b_de.h"
#include "stats1b.h"

#include "lzo1b_cc.h"


#endif /* already included */


/* vim:set ts=4 sw=4 et: */
