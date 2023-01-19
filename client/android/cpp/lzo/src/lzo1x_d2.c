/* lzo1x_d2.c -- LZO1X decompression with overrun testing

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


#include "config1x.h"

#define LZO_TEST_OVERRUN 1
#define DO_DECOMPRESS       lzo1x_decompress_safe

#include "lzo1x_d.ch"

#if defined(LZO_ARCH_I386) && defined(LZO_USE_ASM)
LZO_EXTERN(int) lzo1x_decompress_asm_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_PUBLIC(int) lzo1x_decompress_asm_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem)
{
    return lzo1x_decompress_safe(src, src_len, dst, dst_len, wrkmem);
}
LZO_EXTERN(int) lzo1x_decompress_asm_fast_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem);
LZO_PUBLIC(int) lzo1x_decompress_asm_fast_safe
                                (const lzo_bytep src, lzo_uint  src_len,
                                       lzo_bytep dst, lzo_uintp dst_len,
                                       lzo_voidp wrkmem)
{
    return lzo1x_decompress_safe(src, src_len, dst, dst_len, wrkmem);
}
#endif

/* vim:set ts=4 sw=4 et: */
