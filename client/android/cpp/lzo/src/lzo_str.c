/* lzo_str.c -- string functions for the the LZO library

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

#undef lzo_memcmp
#undef lzo_memcpy
#undef lzo_memmove
#undef lzo_memset


/***********************************************************************
// slow but portable <string.h> stuff, only used in assertions
************************************************************************/

#define lzo_hsize_t             lzo_uint
#define lzo_hvoid_p             lzo_voidp
#define lzo_hbyte_p             lzo_bytep
#define LZOLIB_PUBLIC(r,f)      LZO_PUBLIC(r) f
#ifndef __LZOLIB_FUNCNAME
#define __LZOLIB_FUNCNAME(f)    f
#endif
#define lzo_hmemcmp             __LZOLIB_FUNCNAME(lzo_memcmp)
#define lzo_hmemcpy             __LZOLIB_FUNCNAME(lzo_memcpy)
#define lzo_hmemmove            __LZOLIB_FUNCNAME(lzo_memmove)
#define lzo_hmemset             __LZOLIB_FUNCNAME(lzo_memset)
#define LZO_WANT_ACCLIB_HMEMCPY 1
#include "lzo_supp.h"
#undef LZOLIB_PUBLIC


/* vim:set ts=4 sw=4 et: */
