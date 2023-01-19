/* lzo_util.c -- utilities for the LZO library

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

/* If you use the LZO library in a product, I would appreciate that you
 * keep this copyright string in the executable of your product.
.*/

static const char lzo_copyright_[] =
#if !defined(__LZO_IN_MINLZO)
    /* save space as some people want a really small decompressor */
    LZO_VERSION_STRING;
#else
    "\r\n\n"
    "LZO data compression library.\n"
    "$Copyright: LZO Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\n"
    "<markus@oberhumer.com>\n"
    "http://www.oberhumer.com $\n\n"
    "$Id: LZO version: v" LZO_VERSION_STRING ", " LZO_VERSION_DATE " $\n"
    "$Info: " LZO_INFO_STRING " $\n";
#endif
static const char lzo_version_string_[] = LZO_VERSION_STRING;
static const char lzo_version_date_[] = LZO_VERSION_DATE;


LZO_PUBLIC(const lzo_bytep)
lzo_copyright(void)
{
    return (const lzo_bytep) lzo_copyright_;
}

LZO_PUBLIC(unsigned)
lzo_version(void)
{
    return LZO_VERSION;
}

LZO_PUBLIC(const char *)
lzo_version_string(void)
{
    return lzo_version_string_;
}

LZO_PUBLIC(const char *)
lzo_version_date(void)
{
    return lzo_version_date_;
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_string(void)
{
    return lzo_version_string_;
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_date(void)
{
    return lzo_version_date_;
}


/***********************************************************************
// adler32 checksum
// adapted from free code by Mark Adler <madler at alumni.caltech.edu>
// see http://www.zlib.org/
************************************************************************/

#define LZO_BASE 65521u /* largest prime smaller than 65536 */
#define LZO_NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define LZO_DO1(buf,i)  s1 += buf[i]; s2 += s1
#define LZO_DO2(buf,i)  LZO_DO1(buf,i); LZO_DO1(buf,i+1)
#define LZO_DO4(buf,i)  LZO_DO2(buf,i); LZO_DO2(buf,i+2)
#define LZO_DO8(buf,i)  LZO_DO4(buf,i); LZO_DO4(buf,i+4)
#define LZO_DO16(buf,i) LZO_DO8(buf,i); LZO_DO8(buf,i+8)

LZO_PUBLIC(lzo_uint32_t)
lzo_adler32(lzo_uint32_t adler, const lzo_bytep buf, lzo_uint len)
{
    lzo_uint32_t s1 = adler & 0xffff;
    lzo_uint32_t s2 = (adler >> 16) & 0xffff;
    unsigned k;

    if (buf == NULL)
        return 1;

    while (len > 0)
    {
        k = len < LZO_NMAX ? (unsigned) len : LZO_NMAX;
        len -= k;
        if (k >= 16) do
        {
            LZO_DO16(buf,0);
            buf += 16;
            k -= 16;
        } while (k >= 16);
        if (k != 0) do
        {
            s1 += *buf++;
            s2 += s1;
        } while (--k > 0);
        s1 %= LZO_BASE;
        s2 %= LZO_BASE;
    }
    return (s2 << 16) | s1;
}

#undef LZO_DO1
#undef LZO_DO2
#undef LZO_DO4
#undef LZO_DO8
#undef LZO_DO16


/* vim:set ts=4 sw=4 et: */
