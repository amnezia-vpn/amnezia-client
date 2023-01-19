/* stats1a.h -- statistics for the the LZO1A algorithm

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


/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the LZO package and is subject
   to change.
 */


#ifndef __LZO_STATS1A_H
#define __LZO_STATS1A_H 1

#ifdef __cplusplus
extern "C" {
#endif



/***********************************************************************
// collect statistical information when compressing
// used for finetuning, view with a debugger
************************************************************************/

#if (LZO_COLLECT_STATS)
#  define LZO_STATS(expr)   expr
#else
#  define LZO_STATS(expr)   ((void) 0)
#endif


/***********************************************************************
//
************************************************************************/

typedef struct {

/* configuration */
    unsigned rbits;
    unsigned clevel;

/* internal configuration */
    unsigned dbits;
    unsigned lbits;

/* constants */
    unsigned min_match_short;
    unsigned max_match_short;
    unsigned min_match_long;
    unsigned max_match_long;
    unsigned min_offset;
    unsigned max_offset;
    unsigned r0min;
    unsigned r0fast;
    unsigned r0max;

/* counts */
    long short_matches;
    long long_matches;
    long r1_matches;
    long lit_runs;
    long lit_runs_after_long_match;
    long r0short_runs;
    long r0fast_runs;
    long r0long_runs;

/* */
    long lit_run[RSIZE];
    long lit_run_after_long_match[RSIZE];
    long short_match[MAX_MATCH_SHORT + 1];
    long long_match[MAX_MATCH_LONG + 1];
    long marker[256];

/* these could prove useful for further optimizations */
    long short_match_offset_osize[MAX_MATCH_SHORT + 1];
    long short_match_offset_256[MAX_MATCH_SHORT + 1];
    long short_match_offset_1024[MAX_MATCH_SHORT + 1];
    long matches_out_of_range;
    long matches_out_of_range_2;
    long matches_out_of_range_4;
    long match_out_of_range[MAX_MATCH_SHORT + 1];

/* */
    long in_len;
    long out_len;
}
lzo1a_stats_t;

extern lzo1a_stats_t *lzo1a_stats;



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* already included */


/* vim:set ts=4 sw=4 et: */
