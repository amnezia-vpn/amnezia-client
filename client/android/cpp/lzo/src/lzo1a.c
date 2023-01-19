/* lzo1a.c -- implementation of the LZO1A algorithm

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
#include <lzo/lzo1a.h>


/***********************************************************************
// The next two defines can be changed to customize LZO1A.
// The default version is LZO1A-5/1.
************************************************************************/

/* run bits (3 - 5) - the compressor and the decompressor
 * must use the same value. */
#if !defined(RBITS)
#  define RBITS     5
#endif

/* compression level (1 - 9) - this only affects the compressor.
 * 1 is fastest, 9 is best compression ratio
 */
#if !defined(CLEVEL)
#  define CLEVEL    1           /* fastest by default */
#endif


/* Collect statistics */
#if 0 && !defined(LZO_COLLECT_STATS)
#  define LZO_COLLECT_STATS 1
#endif


/***********************************************************************
// You should not have to change anything below this line.
************************************************************************/

/* check configuration */
#if (RBITS < 3 || RBITS > 5)
#  error "invalid RBITS"
#endif
#if (CLEVEL < 1 || CLEVEL > 9)
#  error "invalid CLEVEL"
#endif


/***********************************************************************
// internal configuration
// all of these affect compression only
************************************************************************/

/* choose the hashing strategy */
#ifndef LZO_HASH
#define LZO_HASH    LZO_HASH_LZO_INCREMENTAL_A
#endif
#define D_INDEX1(d,p)       d = DM(DMUL(0x21,DX2(p,5,5)) >> 5)
#define D_INDEX2(d,p)       d = d ^ D_MASK

#include "lzo1a_de.h"
#include "stats1a.h"


/* check other constants */
#if (LBITS < 5 || LBITS > 8)
#  error "invalid LBITS"
#endif


#if (LZO_COLLECT_STATS)
   static lzo1a_stats_t lzo_statistics;
   lzo1a_stats_t *lzo1a_stats = &lzo_statistics;
#  define lzo_stats lzo1a_stats
#endif


/***********************************************************************
// get algorithm info, return memory required for compression
************************************************************************/

LZO_EXTERN(lzo_uint) lzo1a_info ( int *rbits, int *clevel );

LZO_PUBLIC(lzo_uint)
lzo1a_info ( int *rbits, int *clevel )
{
    if (rbits)
        *rbits = RBITS;
    if (clevel)
        *clevel = CLEVEL;
    return D_SIZE * lzo_sizeof(lzo_bytep);
}


/***********************************************************************
// LZO1A decompress a block of data.
//
// Could be easily translated into assembly code.
************************************************************************/

LZO_PUBLIC(int)
lzo1a_decompress ( const lzo_bytep in , lzo_uint  in_len,
                         lzo_bytep out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
    lzo_bytep op;
    const lzo_bytep ip;
    lzo_uint t;
    const lzo_bytep m_pos;
    const lzo_bytep const ip_end = in + in_len;

    LZO_UNUSED(wrkmem);

    op = out;
    ip = in;
    while (ip < ip_end)
    {
        t = *ip++;      /* get marker */
        LZO_STATS(lzo_stats->marker[t]++);

        if (t == 0)             /* a R0 literal run */
        {
            t = *ip++;
            if (t >= R0FAST - R0MIN)            /* a long R0 run */
            {
                t -= R0FAST - R0MIN;
                if (t == 0)
                    t = R0FAST;
                else
                {
#if 0
                    t = 256u << ((unsigned) t);
#else
                    /* help the optimizer */
                    lzo_uint tt = 256;
                    do tt <<= 1; while (--t > 0);
                    t = tt;
#endif
                }
                MEMCPY8_DS(op,ip,t);
                continue;
            }
            t += R0MIN;
            goto literal;
        }
        else if (t < R0MIN)     /* a short literal run */
        {
literal:
            MEMCPY_DS(op,ip,t);

        /* after a literal a match must follow */
            while (ip < ip_end)
            {
                t = *ip++;          /* get R1 marker */
                if (t >= R0MIN)
                    goto match;

            /* R1 match - a context sensitive 3 byte match + 1 byte literal */
                assert((t & OMASK) == t);
                m_pos = op - MIN_OFFSET;
                m_pos -= t | (((lzo_uint) *ip++) << OBITS);
                assert(m_pos >= out); assert(m_pos < op);
                *op++ = m_pos[0];
                *op++ = m_pos[1];
                *op++ = m_pos[2];
                *op++ = *ip++;
            }
        }
        else                    /* a match */
        {
match:
            /* get match offset */
            m_pos = op - MIN_OFFSET;
            m_pos -= (t & OMASK) | (((lzo_uint) *ip++) << OBITS);
            assert(m_pos >= out); assert(m_pos < op);

            /* get match len */
            if (t < ((MSIZE - 1) << OBITS))         /* a short match */
            {
                t >>= OBITS;
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                MEMCPY_DS(op,m_pos,t);
            }
            else                                     /* a long match */
            {
#if (LBITS < 8)
                t = (MIN_MATCH_LONG - THRESHOLD) + ((lzo_uint)(*ip++) & LMASK);
#else
                t = (MIN_MATCH_LONG - THRESHOLD) + (lzo_uint)(*ip++);
#endif
                *op++ = *m_pos++;
                *op++ = *m_pos++;
                MEMCPY_DS(op,m_pos,t);
#if (LBITS < 8)
                /* a very short literal following a long match */
                t = ip[-1] >> LBITS;
                if (t) do
                    *op++ = *ip++;
                while (--t);
#endif
            }
        }
    }

    *out_len = pd(op, out);

    /* the next line is the only check in the decompressor */
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));
}



/***********************************************************************
// LZO1A compress a block of data.
//
// I apologize for the spaghetti code, but it really helps the optimizer.
************************************************************************/

#include "lzo1a_cr.ch"

static int
do_compress    ( const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem )
{
    const lzo_bytep ip;
#if defined(__LZO_HASH_INCREMENTAL)
    lzo_xint dv;
#endif
    const lzo_bytep m_pos;
    lzo_bytep op;
    const lzo_bytep const ip_end = in+in_len - DVAL_LEN - MIN_MATCH_LONG;
    const lzo_bytep const in_end = in+in_len - DVAL_LEN;
    const lzo_bytep ii;
    lzo_dict_p const dict = (lzo_dict_p) wrkmem;
    const lzo_bytep r1 = ip_end;    /* pointer for R1 match (none yet) */
#if (LBITS < 8)
    const lzo_bytep im = ip_end;    /* pointer to last match start */
#endif

#if !defined(NDEBUG)
    const lzo_bytep m_pos_sav;
#endif

    op = out;
    ip = in;
    ii = ip;            /* point to start of current literal run */

    /* init dictionary */
#if (LZO_DETERMINISTIC)
    BZERO8_PTR(wrkmem,sizeof(lzo_dict_t),D_SIZE);
#endif

    DVAL_FIRST(dv,ip); UPDATE_D(dict,0,dv,ip,in); ip++;
    DVAL_NEXT(dv,ip);

    do {
        LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, m_off, 0);
        lzo_uint dindex;

        DINDEX1(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,MAX_OFFSET))
            goto literal;
        if (m_pos[0] == ip[0] && m_pos[1] == ip[1] && m_pos[2] == ip[2])
            goto match;
        DINDEX2(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,MAX_OFFSET))
            goto literal;
        if (m_pos[0] == ip[0] && m_pos[1] == ip[1] && m_pos[2] == ip[2])
            goto match;
        goto literal;

literal:
        UPDATE_I(dict,0,dindex,ip,in);
        if (++ip >= ip_end)
            break;
        continue;

match:
        UPDATE_I(dict,0,dindex,ip,in);
#if !defined(NDEBUG) && (LZO_DICT_USE_PTR)
        assert(m_pos == NULL || m_pos >= in);
        m_pos_sav = m_pos;
#endif
        m_pos += 3;
        {
    /* we have found a match (of at least length 3) */

#if !defined(NDEBUG) && !(LZO_DICT_USE_PTR)
            assert((m_pos_sav = ip - m_off) == (m_pos - 3));
#endif

            assert(m_pos >= in);
            assert(ip < ip_end);

            /* 1) store the current literal run */
            if (pd(ip,ii) > 0)
            {
                lzo_uint t = pd(ip,ii);

                if (ip - r1 == MIN_MATCH + 1)
                {
                /* Code a context sensitive R1 match.
                 * This is tricky and somewhat difficult to explain:
                 * multiplex a literal run of length 1 into the previous
                 * short match of length MIN_MATCH.
                 * The key idea is:
                 *  - after a short run a match MUST follow
                 *  - therefore the value m = 000 in the mmmooooo marker is free
                 *  - use 000ooooo to indicate a MIN_MATCH match (this
                 *    is already coded) plus a 1 byte literal
                 */
                    assert(t == 1);
                    /* modify marker byte */
                    assert((op[-2] >> OBITS) == (MIN_MATCH - THRESHOLD));
                    op[-2] &= OMASK;
                    assert((op[-2] >> OBITS) == 0);
                    /* copy 1 literal */
                    *op++ = *ii;
                    LZO_STATS(lzo_stats->r1_matches++);
                    r1 = ip;                /* set new R1 pointer */
                }
                else if (t < R0MIN)
                {
                    /* inline the copying of a short run */
#if (LBITS < 8)
                    if (t < (1 << (8-LBITS)) && ii - im >= MIN_MATCH_LONG)
                    {
                    /* Code a very short literal run into the
                     * previous long match length byte.
                     */
                        LZO_STATS(lzo_stats->lit_runs_after_long_match++);
                        LZO_STATS(lzo_stats->lit_run_after_long_match[t]++);
                        assert(ii - im <= MAX_MATCH_LONG);
                        assert((op[-1] >> LBITS) == 0);
                        op[-1] = LZO_BYTE(op[-1] | (t << LBITS));
                        MEMCPY_DS(op, ii, t);
                    }
                    else
#endif
                    {
                        LZO_STATS(lzo_stats->lit_runs++);
                        LZO_STATS(lzo_stats->lit_run[t]++);
                        *op++ = LZO_BYTE(t);
                        MEMCPY_DS(op, ii, t);
                        r1 = ip;                /* set new R1 pointer */
                    }
                }
                else if (t < R0FAST)
                {
                    /* inline the copying of a short R0 run */
                    LZO_STATS(lzo_stats->r0short_runs++);
                    *op++ = 0; *op++ = LZO_BYTE(t - R0MIN);
                    MEMCPY_DS(op, ii, t);
                    r1 = ip;                /* set new R1 pointer */
                }
                else
                    op = store_run(op,ii,t);
            }
#if (LBITS < 8)
            im = ip;
#endif


            /* 2) compute match len */
            ii = ip;        /* point to start of current match */

            /* we already matched MIN_MATCH bytes,
             * m_pos also already advanced MIN_MATCH bytes */
            ip += MIN_MATCH;
            assert(m_pos < ip);

            /* try to match another MIN_MATCH_LONG - MIN_MATCH bytes
             * to see if we get a long match */

#define PS  *m_pos++ != *ip++

#if (MIN_MATCH_LONG - MIN_MATCH == 2)                   /* MBITS == 2 */
            if (PS || PS)
#elif (MIN_MATCH_LONG - MIN_MATCH == 6)                 /* MBITS == 3 */
            if (PS || PS || PS || PS || PS || PS)
#elif (MIN_MATCH_LONG - MIN_MATCH == 14)                /* MBITS == 4 */
            if (PS || PS || PS || PS || PS || PS || PS ||
                PS || PS || PS || PS || PS || PS || PS)
#elif (MIN_MATCH_LONG - MIN_MATCH == 30)                /* MBITS == 5 */
            if (PS || PS || PS || PS || PS || PS || PS || PS ||
                PS || PS || PS || PS || PS || PS || PS || PS ||
                PS || PS || PS || PS || PS || PS || PS || PS ||
                PS || PS || PS || PS || PS || PS)
#else
#  error "MBITS not yet implemented"
#endif
            {
            /* we've found a short match */
                lzo_uint m_len;

            /* 2a) compute match parameters */
                    assert(ip-m_pos == (int)m_off);
                --ip;   /* ran one too far, point back to non-match */
                m_len = pd(ip, ii);
                    assert(m_len >= MIN_MATCH_SHORT);
                    assert(m_len <= MAX_MATCH_SHORT);
                    assert(m_off >= MIN_OFFSET);
                    assert(m_off <= MAX_OFFSET);
                    assert(ii-m_off == m_pos_sav);
                    assert(lzo_memcmp(m_pos_sav,ii,m_len) == 0);
                m_off -= MIN_OFFSET;

            /* 2b) code a short match */
                /* code short match len + low offset bits */
                *op++ = LZO_BYTE(((m_len - THRESHOLD) << OBITS) |
                                 (m_off & OMASK));
                /* code high offset bits */
                *op++ = LZO_BYTE(m_off >> OBITS);


#if (LZO_COLLECT_STATS)
                lzo_stats->short_matches++;
                lzo_stats->short_match[m_len]++;
                if (m_off < OSIZE)
                    lzo_stats->short_match_offset_osize[m_len]++;
                if (m_off < 256)
                    lzo_stats->short_match_offset_256[m_len]++;
                if (m_off < 1024)
                    lzo_stats->short_match_offset_1024[m_len]++;
#endif


            /* 2c) Insert phrases (beginning with ii+1) into the dictionary. */

#define SI      /* nothing */
#define DI      ++ii; DVAL_NEXT(dv,ii); UPDATE_D(dict,0,dv,ii,in);
#define XI      assert(ii < ip); ii = ip; DVAL_FIRST(dv,(ip));

#if (CLEVEL == 9) || (CLEVEL >= 7 && MBITS <= 4) || (CLEVEL >= 5 && MBITS <= 3)
            /* Insert the whole match (ii+1)..(ip-1) into dictionary.  */
                ++ii;
                do {
                    DVAL_NEXT(dv,ii);
                    UPDATE_D(dict,0,dv,ii,in);
                } while (++ii < ip);
                DVAL_NEXT(dv,ii);
                assert(ii == ip);
                DVAL_ASSERT(dv,ip);
#elif (CLEVEL >= 3)
                SI   DI DI   XI
#elif (CLEVEL >= 2)
                SI   DI      XI
#else
                             XI
#endif

            }
            else
            {
            /* we've found a long match - see how far we can still go */
                const lzo_bytep end;
                lzo_uint m_len;

                assert(ip <= in_end);
                assert(ii == ip - MIN_MATCH_LONG);

                if (pd(in_end,ip) <= (MAX_MATCH_LONG - MIN_MATCH_LONG))
                    end = in_end;
                else
                {
                    end = ip + (MAX_MATCH_LONG - MIN_MATCH_LONG);
                    assert(end < in_end);
                }

                while (ip < end && *m_pos == *ip)
                    { m_pos++; ip++; }
                assert(ip <= in_end);

            /* 2a) compute match parameters */
                m_len = pd(ip, ii);
                    assert(m_len >= MIN_MATCH_LONG);
                    assert(m_len <= MAX_MATCH_LONG);
                    assert(m_off >= MIN_OFFSET);
                    assert(m_off <= MAX_OFFSET);
                    assert(ii-m_off == m_pos_sav);
                    assert(lzo_memcmp(m_pos_sav,ii,m_len) == 0);
                    assert(pd(ip,m_pos) == m_off);
                m_off -= MIN_OFFSET;

            /* 2b) code the long match */
                /* code long match flag + low offset bits */
                *op++ = LZO_BYTE(((MSIZE - 1) << OBITS) | (m_off & OMASK));
                /* code high offset bits */
                *op++ = LZO_BYTE(m_off >> OBITS);
                /* code match len */
                *op++ = LZO_BYTE(m_len - MIN_MATCH_LONG);


#if (LZO_COLLECT_STATS)
                lzo_stats->long_matches++;
                lzo_stats->long_match[m_len]++;
#endif


            /* 2c) Insert phrases (beginning with ii+1) into the dictionary. */
#if (CLEVEL == 9)
            /* Insert the whole match (ii+1)..(ip-1) into dictionary.  */
            /* This is not recommended because it is slow. */
                ++ii;
                do {
                    DVAL_NEXT(dv,ii);
                    UPDATE_D(dict,0,dv,ii,in);
                } while (++ii < ip);
                DVAL_NEXT(dv,ii);
                assert(ii == ip);
                DVAL_ASSERT(dv,ip);
#elif (CLEVEL >= 8)
                SI   DI DI DI DI DI DI DI DI   XI
#elif (CLEVEL >= 7)
                SI   DI DI DI DI DI DI DI      XI
#elif (CLEVEL >= 6)
                SI   DI DI DI DI DI DI         XI
#elif (CLEVEL >= 5)
                SI   DI DI DI DI               XI
#elif (CLEVEL >= 4)
                SI   DI DI DI                  XI
#elif (CLEVEL >= 3)
                SI   DI DI                     XI
#elif (CLEVEL >= 2)
                SI   DI                        XI
#else
                                               XI
#endif
            }

            /* ii now points to the start of the next literal run */
            assert(ii == ip);
        }

    } while (ip < ip_end);

    assert(ip <= in_end);


#if defined(LZO_RETURN_IF_NOT_COMPRESSIBLE)
    /* return -1 if op == out to indicate that we
     * couldn't compress and didn't copy anything.
     */
    if (op == out)
    {
        *out_len = 0;
        return LZO_E_NOT_COMPRESSIBLE;
    }
#endif

    /* store the final literal run */
    if (pd(in_end+DVAL_LEN,ii) > 0)
        op = store_run(op,ii,pd(in_end+DVAL_LEN,ii));

    *out_len = pd(op, out);
    return 0;               /* compression went ok */
}


/***********************************************************************
// LZO1A compress public entry point.
************************************************************************/

LZO_PUBLIC(int)
lzo1a_compress ( const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem )
{
    int r = LZO_E_OK;


#if (LZO_COLLECT_STATS)
    lzo_memset(lzo_stats,0,sizeof(*lzo_stats));
    lzo_stats->rbits  = RBITS;
    lzo_stats->clevel = CLEVEL;
    lzo_stats->dbits  = DBITS;
    lzo_stats->lbits  = LBITS;
    lzo_stats->min_match_short = MIN_MATCH_SHORT;
    lzo_stats->max_match_short = MAX_MATCH_SHORT;
    lzo_stats->min_match_long  = MIN_MATCH_LONG;
    lzo_stats->max_match_long  = MAX_MATCH_LONG;
    lzo_stats->min_offset      = MIN_OFFSET;
    lzo_stats->max_offset      = MAX_OFFSET;
    lzo_stats->r0min  = R0MIN;
    lzo_stats->r0fast = R0FAST;
    lzo_stats->r0max  = R0MAX;
    lzo_stats->in_len = in_len;
#endif


    /* don't try to compress a block that's too short */
    if (in_len == 0)
        *out_len = 0;
    else if (in_len <= MIN_MATCH_LONG + DVAL_LEN + 1)
    {
#if defined(LZO_RETURN_IF_NOT_COMPRESSIBLE)
        r = LZO_E_NOT_COMPRESSIBLE;
#else
        *out_len = pd(store_run(out,in,in_len), out);
#endif
    }
    else
        r = do_compress(in,in_len,out,out_len,wrkmem);


#if (LZO_COLLECT_STATS)
    lzo_stats->short_matches -= lzo_stats->r1_matches;
    lzo_stats->short_match[MIN_MATCH] -= lzo_stats->r1_matches;
    lzo_stats->out_len = *out_len;
#endif

    return r;
}


/* vim:set ts=4 sw=4 et: */
