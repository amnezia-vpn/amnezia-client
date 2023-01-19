/* lzo_ptr.c -- low-level pointer constructs

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


#include "lzo_conf.h"


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(lzo_uintptr_t)
__lzo_ptr_linear(const lzo_voidp ptr)
{
    lzo_uintptr_t p;

#if (LZO_ARCH_I086)
#error "LZO_ARCH_I086 is unsupported"
#elif (LZO_MM_PVP)
#error "LZO_MM_PVP is unsupported"
#else
    p = (lzo_uintptr_t) PTR_LINEAR(ptr);
#endif

    return p;
}


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(unsigned)
__lzo_align_gap(const lzo_voidp ptr, lzo_uint size)
{
#if (__LZO_UINTPTR_T_IS_POINTER)
#error "__LZO_UINTPTR_T_IS_POINTER is unsupported"
#else
    lzo_uintptr_t p, n;
    if (size < 2) return 0;
    p = __lzo_ptr_linear(ptr);
#if 0
    n = (((p + size - 1) / size) * size) - p;
#else
    if ((size & (size - 1)) != 0)
        return 0;
    n = size; n = ((p + n - 1) & ~(n - 1)) - p;
#endif
#endif
    assert((long)n >= 0);
    assert(n <= size);
    return (unsigned)n;
}


/* vim:set ts=4 sw=4 et: */
