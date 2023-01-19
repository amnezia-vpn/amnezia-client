/* lzo1c_cc.c -- LZO1C compression internal entry point

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
#include "config1c.h"


/***********************************************************************
// compression internal entry point.
************************************************************************/

LZO_LOCAL_IMPL(int)
_lzo1c_do_compress ( const lzo_bytep in,  lzo_uint  in_len,
                           lzo_bytep out, lzo_uintp out_len,
                           lzo_voidp wrkmem,
                           lzo_compress_t func )
{
    int r;
#if defined(LZO_TEST_COMPRESS_OVERRUN)
    lzo_uint avail_out = *out_len;
#endif


#if (LZO_COLLECT_STATS)
    _lzo1c_stats_init(lzo_stats);
    lzo_stats->in_len = in_len;
#endif


    /* don't try to compress a block that's too short */
    if (in_len == 0)
    {
        *out_len = 0;
        r = LZO_E_OK;
    }
    else if (in_len <= MIN_LOOKAHEAD + 1)
    {
#if defined(LZO_RETURN_IF_NOT_COMPRESSIBLE)
        *out_len = 0;
        r = LZO_E_NOT_COMPRESSIBLE;
#else
        *out_len = pd(STORE_RUN(out,in,in_len), out);
        r = (*out_len > in_len) ? LZO_E_OK : LZO_E_ERROR;
#endif
    }
    else
        r = func(in,in_len,out,out_len,wrkmem);


#if defined(LZO_EOF_CODE)
#if defined(LZO_TEST_COMPRESS_OVERRUN)
    if (r == LZO_E_OK && avail_out - *out_len < 3)
        r = LZO_E_COMPRESS_OVERRUN;
#endif
    if (r == LZO_E_OK)
    {
        lzo_bytep op = out + *out_len;
        op[0] = M3_MARKER | 1;
        op[1] = 0;
        op[2] = 0;
        *out_len += 3;
    }
#endif


#if (LZO_COLLECT_STATS)
    lzo_stats->out_len = *out_len;
    lzo_stats->match_bytes =
       1 * lzo_stats->m1_matches + 2 * lzo_stats->m2_matches +
       3 * lzo_stats->m3_matches + 4 * lzo_stats->m4_matches;
    _lzo1c_stats_calc(lzo_stats);
#endif

    return r;
}


/***********************************************************************
// note: this is not thread safe, but as it is used for finetuning only
//       we don't care
************************************************************************/

#undef lzo_stats
/* lzo_stats_t is still defined */


#if (LZO_COLLECT_STATS)

static lzo_stats_t lzo_statistics;
lzo_stats_t * const lzo1c_stats = &lzo_statistics;


void _lzo1c_stats_init(lzo_stats_t *lzo_stats)
{
    lzo_memset(lzo_stats,0,sizeof(*lzo_stats));
}


void _lzo1c_stats_calc(lzo_stats_t *lzo_stats)
{
    lzo_stats->matches =
       lzo_stats->m1_matches + lzo_stats->m2_matches +
       lzo_stats->m3_matches + lzo_stats->m4_matches;

    lzo_stats->literal_overhead = lzo_stats->lit_runs +
       2 * (lzo_stats->r0short_runs + lzo_stats->r0fast_runs +
            lzo_stats->r0long_runs);
    lzo_stats->literal_bytes = lzo_stats->literals +
       lzo_stats->literal_overhead;

#if 0
    assert(lzo_stats->match_bytes + lzo_stats->literal_bytes ==
       lzo_stats->out_len);
#endif

    lzo_stats->m2_matches -= lzo_stats->r1_matches;
    lzo_stats->m2_match[M2_MIN_LEN] -= lzo_stats->r1_matches;

    if (lzo_stats->literals > 0)
        lzo_stats->literal_overhead_percent =
           100.0 * lzo_stats->literal_overhead / lzo_stats->literals;
}


#endif


/* vim:set ts=4 sw=4 et: */
