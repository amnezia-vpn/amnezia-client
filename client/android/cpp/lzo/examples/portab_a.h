/* portab_a.h -- advanced portability layer

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


/*************************************************************************
//
**************************************************************************/

#if defined(LZO_HAVE_CONFIG_H)
#  define LZO_CFG_NO_CONFIG_HEADER 1
#endif

#define LZO_WANT_ACC_INCD_H 1
#define LZO_WANT_ACC_INCE_H 1
#if defined(LZO_WANT_ACCLIB_GETOPT) || defined(WANT_LZO_PCLOCK) || defined(WANT_LZO_WILDARGV)
#  define LZO_WANT_ACC_INCI_H 1
#  define LZO_WANT_ACC_LIB_H 1
#endif
#if defined(WANT_LZO_PCLOCK)
#  define LZO_WANT_ACCLIB_PCLOCK 1
#endif
#if defined(WANT_LZO_WILDARGV)
#  define LZO_WANT_ACCLIB_WILDARGV 1
#endif
#include "src/lzo_supp.h"

#if defined(WANT_LZO_MALLOC)
#  define lzo_malloc(a)         (malloc(a))
#  define lzo_free(a)           (free(a))
#endif
#if defined(WANT_LZO_FREAD)
#  define lzo_fread(f,b,s)      (fread(b,1,s,f))
#  define lzo_fwrite(f,b,s)     (fwrite(b,1,s,f))
#endif


/* vim:set ts=4 sw=4 et: */
