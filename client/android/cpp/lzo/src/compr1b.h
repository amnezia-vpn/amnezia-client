/* compr1b.h --

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


#define LZO_NEED_DICT_H 1
#include "config1b.h"


#if !defined(COMPRESS_ID)
#define COMPRESS_ID     LZO_PP_ECONCAT2(DD_BITS,CLEVEL)
#endif


#include "lzo1b_c.ch"


/***********************************************************************
//
************************************************************************/

#define LZO_COMPRESS \
    LZO_PP_ECONCAT3(lzo1b_,COMPRESS_ID,_compress)

#define LZO_COMPRESS_FUNC \
    LZO_PP_ECONCAT3(_lzo1b_,COMPRESS_ID,_compress_func)



/***********************************************************************
//
************************************************************************/

const lzo_compress_t LZO_COMPRESS_FUNC = do_compress;

LZO_PUBLIC(int)
LZO_COMPRESS ( const lzo_bytep in,  lzo_uint  in_len,
                     lzo_bytep out, lzo_uintp out_len,
                     lzo_voidp wrkmem )
{
    return _lzo1b_do_compress(in,in_len,out,out_len,wrkmem,do_compress);
}


/* vim:set ts=4 sw=4 et: */
