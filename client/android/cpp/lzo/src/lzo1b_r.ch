/* lzo1b_r.ch -- literal run handling for the the LZO1B/LZO1C algorithm

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
// store a literal run (internal)
************************************************************************/

LZO_LOCAL_IMPL(lzo_bytep )
STORE_RUN ( lzo_bytep const oo, const lzo_bytep const ii, lzo_uint r_len)
{
    lzo_bytep op;
    const lzo_bytep ip;
    lzo_uint t;

    LZO_STATS(lzo_stats->literals += r_len);

    op = oo;
    ip = ii;
    assert(r_len > 0);

    /* code a long R0 run */
    if (r_len >= 512)
    {
        unsigned r_bits = 6;        /* 256 << 6 == 16384 */
        lzo_uint tt = 32768u;

        while (r_len >= (t = tt))
        {
            r_len -= t;
            *op++ = 0; *op++ = (R0FAST - R0MIN) + 7;
            MEMCPY8_DS(op, ip, t);
            LZO_STATS(lzo_stats->r0long_runs++);
        }
        tt >>= 1;
        do {
            if (r_len >= (t = tt))
            {
                r_len -= t;
                *op++ = 0; *op++ = LZO_BYTE((R0FAST - R0MIN) + r_bits);
                MEMCPY8_DS(op, ip, t);
                LZO_STATS(lzo_stats->r0long_runs++);
            }
            tt >>= 1;
        } while (--r_bits > 0);
    }
    assert(r_len < 512);

    while (r_len >= (t = R0FAST))
    {
        r_len -= t;
        *op++ = 0; *op++ = (R0FAST - R0MIN);
        MEMCPY8_DS(op, ip, t);
        LZO_STATS(lzo_stats->r0fast_runs++);
    }

    t = r_len;
    if (t >= R0MIN)
    {
        /* code a short R0 run */
        *op++ = 0; *op++ = LZO_BYTE(t - R0MIN);
        MEMCPY_DS(op, ip, t);
        LZO_STATS(lzo_stats->r0short_runs++);
    }
    else if (t > 0)
    {
        /* code a short literal run */
        LZO_STATS(lzo_stats->lit_runs++);
        LZO_STATS(lzo_stats->lit_run[t]++);
        *op++ = LZO_BYTE(t);
        MEMCPY_DS(op, ip, t);
    }

    return op;
}


/* vim:set ts=4 sw=4 et: */
