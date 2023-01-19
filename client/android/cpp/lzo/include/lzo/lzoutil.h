/* lzoutil.h -- utility functions for use by applications

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


#ifndef __LZOUTIL_H_INCLUDED
#define __LZOUTIL_H_INCLUDED 1

#ifndef __LZOCONF_H_INCLUDED
#include <lzo/lzoconf.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/***********************************************************************
// LZO-v1 deprecated macros (which were used in the old example programs)
//
// THIS FILE IS DEPRECATED. DO NOT USE.
************************************************************************/

#define lzo_alloc(a,b)      (malloc((a)*(b)))
#define lzo_malloc(a)       (malloc(a))
#define lzo_free(a)         (free(a))

#define lzo_fread(f,b,s)    (fread(b,1,s,f))
#define lzo_fwrite(f,b,s)   (fwrite(b,1,s,f))


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 sw=4 et: */
