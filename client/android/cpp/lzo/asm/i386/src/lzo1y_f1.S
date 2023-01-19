/* lzo1y_f1.S -- fast LZO1Y decompression in assembler (i386 + gcc)

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


/***********************************************************************
//
************************************************************************/

#define LZO_FAST

#include "lzo_asm.h"

    .text

    LZO_PUBLIC(lzo1y_decompress_asm_fast)

#define LZO1Y

#include "enter.ash"
#include "lzo1x_d.ash"
#include "leave.ash"

    LZO_PUBLIC_END(lzo1y_decompress_asm_fast)


/* vim:set ts=4 sw=4 et: */
