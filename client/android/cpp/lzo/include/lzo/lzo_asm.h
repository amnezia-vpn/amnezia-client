/* lzo_asm.h -- assembler prototypes for the LZO data compression library

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


#ifndef __LZO_ASM_H_INCLUDED
#define __LZO_ASM_H_INCLUDED 1

#ifndef __LZOCONF_H_INCLUDED
#include <lzo/lzoconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// i386 assembly decompressors
//
// NOTE:
// ====
//
// - For reasons of speed all fast assembler decompressors (having '_fast'
//   in their name) can access (write to) up to 3 bytes past the end of
//   the decompressed ("dst") block. Data past the end of the compressed
//   ("src") block is never accessed (read from).
//   [ technical note: because data is transferred in 32-bit units ]
//
// - Please also see asm/i386/00README.TXT and doc/LZO.FAQ for more
//   important details about the assembler versions.
//
************************************************************************/

LZO_EXTERN(int) lzo1c_decompress_asm
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1c_decompress_asm_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);

LZO_EXTERN(int) lzo1f_decompress_asm_fast
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1f_decompress_asm_fast_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);

LZO_EXTERN(int) lzo1x_decompress_asm
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1x_decompress_asm_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1x_decompress_asm_fast
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1x_decompress_asm_fast_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);

LZO_EXTERN(int) lzo1y_decompress_asm
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1y_decompress_asm_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1y_decompress_asm_fast
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_EXTERN(int) lzo1y_decompress_asm_fast_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);


/***********************************************************************
// checksum and misc functions
************************************************************************/

#if 0

LZO_EXTERN(lzo_uint32_t)
lzo_crc32_asm(lzo_uint32_t c, const lzo_bytep buf, lzo_uint len,
              const lzo_uint32_tp tab);

LZO_EXTERN(lzo_uint32_t)
lzo_crc32_asm_small(lzo_uint32_t c, const lzo_bytep buf, lzo_uint len);

LZO_EXTERN(int)
lzo_cpuid_asm(lzo_uint32_tp /* lzo_uint32_t info[16] */ );

LZO_EXTERN(lzo_uint32_t)
lzo_rdtsc_asm(lzo_uint32_tp /* lzo_uint32_t ticks[2] */ );

#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 sw=4 et: */
