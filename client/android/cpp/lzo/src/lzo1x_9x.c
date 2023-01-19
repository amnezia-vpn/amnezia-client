/* lzo1x_9x.c -- implementation of the LZO1X-999 compression algorithm

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


#if !defined(LZO1X) && !defined(LZO1Y) && !defined(LZO1Z)
#  define LZO1X 1
#endif

#if defined(LZO1X)
#  include "config1x.h"
#elif defined(LZO1Y)
#  include "config1y.h"
#elif defined(LZO1Z)
#  include "config1z.h"
#else
#  error
#endif


/***********************************************************************
//
************************************************************************/

#define SWD_N           M4_MAX_OFFSET   /* size of ring buffer */
#define SWD_THRESHOLD       1           /* lower limit for match length */
#define SWD_F            2048           /* upper limit for match length */

#define SWD_BEST_OFF    (LZO_MAX3( M2_MAX_LEN, M3_MAX_LEN, M4_MAX_LEN ) + 1)

#if defined(LZO1X)
#  define LZO_COMPRESS_T                lzo1x_999_t
#  define lzo_swd_t                     lzo1x_999_swd_t
#elif defined(LZO1Y)
#  define LZO_COMPRESS_T                lzo1y_999_t
#  define lzo_swd_t                     lzo1y_999_swd_t
#  define lzo1x_999_compress_internal   lzo1y_999_compress_internal
#  define lzo1x_999_compress_dict       lzo1y_999_compress_dict
#  define lzo1x_999_compress_level      lzo1y_999_compress_level
#  define lzo1x_999_compress            lzo1y_999_compress
#elif defined(LZO1Z)
#  define LZO_COMPRESS_T                lzo1z_999_t
#  define lzo_swd_t                     lzo1z_999_swd_t
#  define lzo1x_999_compress_internal   lzo1z_999_compress_internal
#  define lzo1x_999_compress_dict       lzo1z_999_compress_dict
#  define lzo1x_999_compress_level      lzo1z_999_compress_level
#  define lzo1x_999_compress            lzo1z_999_compress
#else
#  error
#endif

#if 0
#  define HEAD3(b,p) \
    ((((((lzo_xint)b[p]<<3)^b[p+1])<<3)^b[p+2]) & (SWD_HSIZE-1))
#endif
#if 0 && (LZO_OPT_UNALIGNED32) && (LZO_ABI_LITTLE_ENDIAN)
#  define HEAD3(b,p) \
    (((* (lzo_uint32_tp) &b[p]) ^ ((* (lzo_uint32_tp) &b[p])>>10)) & (SWD_HSIZE-1))
#endif

#include "lzo_mchw.ch"


/* this is a public functions, but there is no prototype in a header file */
LZO_EXTERN(int)
lzo1x_999_compress_internal ( const lzo_bytep in , lzo_uint  in_len,
                                    lzo_bytep out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                              const lzo_bytep dict, lzo_uint dict_len,
                                    lzo_callback_p cb,
                                    int try_lazy_parm,
                                    lzo_uint good_length,
                                    lzo_uint max_lazy,
                                    lzo_uint nice_length,
                                    lzo_uint max_chain,
                                    lzo_uint32_t flags );


/***********************************************************************
//
************************************************************************/

static lzo_bytep
code_match ( LZO_COMPRESS_T *c, lzo_bytep op, lzo_uint m_len, lzo_uint m_off )
{
    lzo_uint x_len = m_len;
    lzo_uint x_off = m_off;

    c->match_bytes += m_len;

#if 0
/*
    static lzo_uint last_m_len = 0, last_m_off = 0;
    static lzo_uint prev_m_off[4];
    static unsigned prev_m_off_ptr = 0;
    unsigned i;

    //if (m_len >= 3 && m_len <= M2_MAX_LEN && m_off <= M2_MAX_OFFSET)
    if (m_len >= 3 && m_len <= M2_MAX_LEN)
    {
    //if (m_len == last_m_len && m_off == last_m_off)
        //printf("last_m_len + last_m_off\n");
    //else
    if (m_off == last_m_off)
        printf("last_m_off\n");
    else
    {
        for (i = 0; i < 4; i++)
            if (m_off == prev_m_off[i])
                printf("prev_m_off %u: %5ld\n",i,(long)m_off);
    }
    }
    last_m_len = m_len;
    last_m_off = prev_m_off[prev_m_off_ptr] = m_off;
    prev_m_off_ptr = (prev_m_off_ptr + 1) & 3;
*/
#endif

    assert(op > c->out);
    if (m_len == 2)
    {
        assert(m_off <= M1_MAX_OFFSET);
        assert(c->r1_lit > 0); assert(c->r1_lit < 4);
        m_off -= 1;
#if defined(LZO1Z)
        *op++ = LZO_BYTE(M1_MARKER | (m_off >> 6));
        *op++ = LZO_BYTE(m_off << 2);
#else
        *op++ = LZO_BYTE(M1_MARKER | ((m_off & 3) << 2));
        *op++ = LZO_BYTE(m_off >> 2);
#endif
        c->m1a_m++;
    }
#if defined(LZO1Z)
    else if (m_len <= M2_MAX_LEN && (m_off <= M2_MAX_OFFSET || m_off == c->last_m_off))
#else
    else if (m_len <= M2_MAX_LEN && m_off <= M2_MAX_OFFSET)
#endif
    {
        assert(m_len >= 3);
#if defined(LZO1X)
        m_off -= 1;
        *op++ = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
        *op++ = LZO_BYTE(m_off >> 3);
        assert(op[-2] >= M2_MARKER);
#elif defined(LZO1Y)
        m_off -= 1;
        *op++ = LZO_BYTE(((m_len + 1) << 4) | ((m_off & 3) << 2));
        *op++ = LZO_BYTE(m_off >> 2);
        assert(op[-2] >= M2_MARKER);
#elif defined(LZO1Z)
        if (m_off == c->last_m_off)
            *op++ = LZO_BYTE(((m_len - 1) << 5) | (0x700 >> 6));
        else
        {
            m_off -= 1;
            *op++ = LZO_BYTE(((m_len - 1) << 5) | (m_off >> 6));
            *op++ = LZO_BYTE(m_off << 2);
        }
#endif
        c->m2_m++;
    }
    else if (m_len == M2_MIN_LEN && m_off <= MX_MAX_OFFSET && c->r1_lit >= 4)
    {
        assert(m_len == 3);
        assert(m_off > M2_MAX_OFFSET);
        m_off -= 1 + M2_MAX_OFFSET;
#if defined(LZO1Z)
        *op++ = LZO_BYTE(M1_MARKER | (m_off >> 6));
        *op++ = LZO_BYTE(m_off << 2);
#else
        *op++ = LZO_BYTE(M1_MARKER | ((m_off & 3) << 2));
        *op++ = LZO_BYTE(m_off >> 2);
#endif
        c->m1b_m++;
    }
    else if (m_off <= M3_MAX_OFFSET)
    {
        assert(m_len >= 3);
        m_off -= 1;
        if (m_len <= M3_MAX_LEN)
            *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
        else
        {
            m_len -= M3_MAX_LEN;
            *op++ = M3_MARKER | 0;
            while (m_len > 255)
            {
                m_len -= 255;
                *op++ = 0;
            }
            assert(m_len > 0);
            *op++ = LZO_BYTE(m_len);
        }
#if defined(LZO1Z)
        *op++ = LZO_BYTE(m_off >> 6);
        *op++ = LZO_BYTE(m_off << 2);
#else
        *op++ = LZO_BYTE(m_off << 2);
        *op++ = LZO_BYTE(m_off >> 6);
#endif
        c->m3_m++;
    }
    else
    {
        lzo_uint k;

        assert(m_len >= 3);
        assert(m_off > 0x4000); assert(m_off <= 0xbfff);
        m_off -= 0x4000;
        k = (m_off & 0x4000) >> 11;
        if (m_len <= M4_MAX_LEN)
            *op++ = LZO_BYTE(M4_MARKER | k | (m_len - 2));
        else
        {
            m_len -= M4_MAX_LEN;
            *op++ = LZO_BYTE(M4_MARKER | k | 0);
            while (m_len > 255)
            {
                m_len -= 255;
                *op++ = 0;
            }
            assert(m_len > 0);
            *op++ = LZO_BYTE(m_len);
        }
#if defined(LZO1Z)
        *op++ = LZO_BYTE(m_off >> 6);
        *op++ = LZO_BYTE(m_off << 2);
#else
        *op++ = LZO_BYTE(m_off << 2);
        *op++ = LZO_BYTE(m_off >> 6);
#endif
        c->m4_m++;
    }

    c->last_m_len = x_len;
    c->last_m_off = x_off;
    return op;
}


static lzo_bytep
STORE_RUN ( LZO_COMPRESS_T *c, lzo_bytep op, const lzo_bytep ii, lzo_uint t )
{
    c->lit_bytes += t;

    if (op == c->out && t <= 238)
    {
        *op++ = LZO_BYTE(17 + t);
    }
    else if (t <= 3)
    {
#if defined(LZO1Z)
        op[-1] = LZO_BYTE(op[-1] | t);
#else
        op[-2] = LZO_BYTE(op[-2] | t);
#endif
        c->lit1_r++;
    }
    else if (t <= 18)
    {
        *op++ = LZO_BYTE(t - 3);
        c->lit2_r++;
    }
    else
    {
        lzo_uint tt = t - 18;

        *op++ = 0;
        while (tt > 255)
        {
            tt -= 255;
            *op++ = 0;
        }
        assert(tt > 0);
        *op++ = LZO_BYTE(tt);
        c->lit3_r++;
    }
    do *op++ = *ii++; while (--t > 0);

    return op;
}


static lzo_bytep
code_run ( LZO_COMPRESS_T *c, lzo_bytep op, const lzo_bytep ii,
           lzo_uint lit, lzo_uint m_len )
{
    if (lit > 0)
    {
        assert(m_len >= 2);
        op = STORE_RUN(c,op,ii,lit);
        c->r1_m_len = m_len;
        c->r1_lit = lit;
    }
    else
    {
        assert(m_len >= 3);
        c->r1_m_len = 0;
        c->r1_lit = 0;
    }

    return op;
}


/***********************************************************************
//
************************************************************************/

static lzo_uint
len_of_coded_match ( lzo_uint m_len, lzo_uint m_off, lzo_uint lit )
{
    lzo_uint n = 4;

    if (m_len < 2)
        return 0;
    if (m_len == 2)
        return (m_off <= M1_MAX_OFFSET && lit > 0 && lit < 4) ? 2 : 0;
    if (m_len <= M2_MAX_LEN && m_off <= M2_MAX_OFFSET)
        return 2;
    if (m_len == M2_MIN_LEN && m_off <= MX_MAX_OFFSET && lit >= 4)
        return 2;
    if (m_off <= M3_MAX_OFFSET)
    {
        if (m_len <= M3_MAX_LEN)
            return 3;
        m_len -= M3_MAX_LEN;
        while (m_len > 255)
        {
            m_len -= 255;
            n++;
        }
        return n;
    }
    if (m_off <= M4_MAX_OFFSET)
    {
        if (m_len <= M4_MAX_LEN)
            return 3;
        m_len -= M4_MAX_LEN;
        while (m_len > 255)
        {
            m_len -= 255;
            n++;
        }
        return n;
    }
    return 0;
}


static lzo_uint
min_gain(lzo_uint ahead, lzo_uint lit1, lzo_uint lit2, lzo_uint l1, lzo_uint l2, lzo_uint l3)
{
    lzo_uint lazy_match_min_gain;

    assert (ahead >= 1);
    lazy_match_min_gain = ahead;

#if 0
    if (l3)
        lit2 -= ahead;
#endif

    if (lit1 <= 3)
        lazy_match_min_gain += (lit2 <= 3) ? 0 : 2;
    else if (lit1 <= 18)
        lazy_match_min_gain += (lit2 <= 18) ? 0 : 1;

    lazy_match_min_gain += (l2 - l1) * 2;
    if (l3)
        lazy_match_min_gain -= (ahead - l3) * 2;

    if ((lzo_int) lazy_match_min_gain < 0)
        lazy_match_min_gain = 0;

#if 0
    if (l1 == 2)
        if (lazy_match_min_gain == 0)
            lazy_match_min_gain = 1;
#endif

    return lazy_match_min_gain;
}


/***********************************************************************
//
************************************************************************/

#if !defined(NDEBUG)
static
void assert_match( const lzo_swd_p swd, lzo_uint m_len, lzo_uint m_off )
{
    const LZO_COMPRESS_T *c = swd->c;
    lzo_uint d_off;

    assert(m_len >= 2);
    if (m_off <= (lzo_uint) (c->bp - c->in))
    {
        assert(c->bp - m_off + m_len < c->ip);
        assert(lzo_memcmp(c->bp, c->bp - m_off, m_len) == 0);
    }
    else
    {
        assert(swd->dict != NULL);
        d_off = m_off - (lzo_uint) (c->bp - c->in);
        assert(d_off <= swd->dict_len);
        if (m_len > d_off)
        {
            assert(lzo_memcmp(c->bp, swd->dict_end - d_off, d_off) == 0);
            assert(c->in + m_len - d_off < c->ip);
            assert(lzo_memcmp(c->bp + d_off, c->in, m_len - d_off) == 0);
        }
        else
        {
            assert(lzo_memcmp(c->bp, swd->dict_end - d_off, m_len) == 0);
        }
    }
}
#else
#  define assert_match(a,b,c)   ((void)0)
#endif


#if defined(SWD_BEST_OFF)

static void
better_match ( const lzo_swd_p swd, lzo_uint *m_len, lzo_uint *m_off )
{
#if defined(LZO1Z)
    const LZO_COMPRESS_T *c = swd->c;
#endif

    if (*m_len <= M2_MIN_LEN)
        return;
#if defined(LZO1Z)
    if (*m_off == c->last_m_off && *m_len <= M2_MAX_LEN)
        return;
#if 1
    if (*m_len >= M2_MIN_LEN + 1 && *m_len <= M2_MAX_LEN + 1 &&
        c->last_m_off && swd->best_off[*m_len-1] == c->last_m_off)
    {
        *m_len = *m_len - 1;
        *m_off = swd->best_off[*m_len];
        return;
    }
#endif
#endif

    if (*m_off <= M2_MAX_OFFSET)
        return;

#if 1
    /* M3/M4 -> M2 */
    if (*m_off > M2_MAX_OFFSET &&
        *m_len >= M2_MIN_LEN + 1 && *m_len <= M2_MAX_LEN + 1 &&
        swd->best_off[*m_len-1] && swd->best_off[*m_len-1] <= M2_MAX_OFFSET)
    {
        *m_len = *m_len - 1;
        *m_off = swd->best_off[*m_len];
        return;
    }
#endif

#if 1
    /* M4 -> M2 */
    if (*m_off > M3_MAX_OFFSET &&
        *m_len >= M4_MAX_LEN + 1 && *m_len <= M2_MAX_LEN + 2 &&
        swd->best_off[*m_len-2] && swd->best_off[*m_len-2] <= M2_MAX_OFFSET)
    {
        *m_len = *m_len - 2;
        *m_off = swd->best_off[*m_len];
        return;
    }
#endif

#if 1
    /* M4 -> M3 */
    if (*m_off > M3_MAX_OFFSET &&
        *m_len >= M4_MAX_LEN + 1 && *m_len <= M3_MAX_LEN + 1 &&
        swd->best_off[*m_len-1] && swd->best_off[*m_len-1] <= M3_MAX_OFFSET)
    {
        *m_len = *m_len - 1;
        *m_off = swd->best_off[*m_len];
    }
#endif
}

#endif


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
lzo1x_999_compress_internal ( const lzo_bytep in , lzo_uint  in_len,
                                    lzo_bytep out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                              const lzo_bytep dict, lzo_uint dict_len,
                                    lzo_callback_p cb,
                                    int try_lazy_parm,
                                    lzo_uint good_length,
                                    lzo_uint max_lazy,
                                    lzo_uint nice_length,
                                    lzo_uint max_chain,
                                    lzo_uint32_t flags )
{
    lzo_bytep op;
    const lzo_bytep ii;
    lzo_uint lit;
    lzo_uint m_len, m_off;
    LZO_COMPRESS_T cc;
    LZO_COMPRESS_T * const c = &cc;
    lzo_swd_p const swd = (lzo_swd_p) wrkmem;
    lzo_uint try_lazy;
    int r;

    /* sanity check */
#if defined(LZO1X)
    LZO_COMPILE_TIME_ASSERT(LZO1X_999_MEM_COMPRESS >= SIZEOF_LZO_SWD_T)
#elif defined(LZO1Y)
    LZO_COMPILE_TIME_ASSERT(LZO1Y_999_MEM_COMPRESS >= SIZEOF_LZO_SWD_T)
#elif defined(LZO1Z)
    LZO_COMPILE_TIME_ASSERT(LZO1Z_999_MEM_COMPRESS >= SIZEOF_LZO_SWD_T)
#else
#  error
#endif

/* setup parameter defaults */
    /* number of lazy match tries */
    try_lazy = (lzo_uint) try_lazy_parm;
    if (try_lazy_parm < 0)
        try_lazy = 1;
    /* reduce lazy match search if we already have a match with this length */
    if (good_length == 0)
        good_length = 32;
    /* do not try a lazy match if we already have a match with this length */
    if (max_lazy == 0)
        max_lazy = 32;
    /* stop searching for longer matches than this one */
    if (nice_length == 0)
        nice_length = 0;
    /* don't search more positions than this */
    if (max_chain == 0)
        max_chain = SWD_MAX_CHAIN;

    c->init = 0;
    c->ip = c->in = in;
    c->in_end = in + in_len;
    c->out = out;
    c->cb = cb;
    c->m1a_m = c->m1b_m = c->m2_m = c->m3_m = c->m4_m = 0;
    c->lit1_r = c->lit2_r = c->lit3_r = 0;

    op = out;
    ii = c->ip;             /* point to start of literal run */
    lit = 0;
    c->r1_lit = c->r1_m_len = 0;

    r = init_match(c,swd,dict,dict_len,flags);
    if (r != 0)
        return r;
    if (max_chain > 0)
        swd->max_chain = max_chain;
    if (nice_length > 0)
        swd->nice_length = nice_length;

    r = find_match(c,swd,0,0);
    if (r != 0)
        return r;
    while (c->look > 0)
    {
        lzo_uint ahead;
        lzo_uint max_ahead;
        lzo_uint l1, l2, l3;

        c->codesize = pd(op, out);

        m_len = c->m_len;
        m_off = c->m_off;

        assert(c->bp == c->ip - c->look);
        assert(c->bp >= in);
        if (lit == 0)
            ii = c->bp;
        assert(ii + lit == c->bp);
        assert(swd->b_char == *(c->bp));

        if ( m_len < 2 ||
            (m_len == 2 && (m_off > M1_MAX_OFFSET || lit == 0 || lit >= 4)) ||
#if 1
            /* Do not accept this match for compressed-data compatibility
             * with LZO v1.01 and before
             * [ might be a problem for decompress() and optimize() ]
             */
            (m_len == 2 && op == out) ||
#endif
            (op == out && lit == 0))
        {
            /* a literal */
            m_len = 0;
        }
        else if (m_len == M2_MIN_LEN)
        {
            /* compression ratio improves if we code a literal in some cases */
            if (m_off > MX_MAX_OFFSET && lit >= 4)
                m_len = 0;
        }

        if (m_len == 0)
        {
    /* a literal */
            lit++;
            swd->max_chain = max_chain;
            r = find_match(c,swd,1,0);
            assert(r == 0); LZO_UNUSED(r);
            continue;
        }

    /* a match */
#if defined(SWD_BEST_OFF)
        if (swd->use_best_off)
            better_match(swd,&m_len,&m_off);
#endif
        assert_match(swd,m_len,m_off);


        /* shall we try a lazy match ? */
        ahead = 0;
        if (try_lazy == 0 || m_len >= max_lazy)
        {
            /* no */
            l1 = 0;
            max_ahead = 0;
        }
        else
        {
            /* yes, try a lazy match */
            l1 = len_of_coded_match(m_len,m_off,lit);
            assert(l1 > 0);
#if 1
            max_ahead = LZO_MIN(try_lazy, l1 - 1);
#else
            max_ahead = LZO_MIN3(try_lazy, l1, m_len - 1);
#endif
        }


        while (ahead < max_ahead && c->look > m_len)
        {
            lzo_uint lazy_match_min_gain;

            if (m_len >= good_length)
                swd->max_chain = max_chain >> 2;
            else
                swd->max_chain = max_chain;
            r = find_match(c,swd,1,0);
            ahead++;

            assert(r == 0); LZO_UNUSED(r);
            assert(c->look > 0);
            assert(ii + lit + ahead == c->bp);

#if defined(LZO1Z)
            if (m_off == c->last_m_off && c->m_off != c->last_m_off)
                if (m_len >= M2_MIN_LEN && m_len <= M2_MAX_LEN)
                    c->m_len = 0;
#endif
            if (c->m_len < m_len)
                continue;
#if 1
            if (c->m_len == m_len && c->m_off >= m_off)
                continue;
#endif
#if defined(SWD_BEST_OFF)
            if (swd->use_best_off)
                better_match(swd,&c->m_len,&c->m_off);
#endif
            l2 = len_of_coded_match(c->m_len,c->m_off,lit+ahead);
            if (l2 == 0)
                continue;
#if 0
            if (c->m_len == m_len && l2 >= l1)
                continue;
#endif


#if 1
            /* compressed-data compatibility [see above] */
            l3 = (op == out) ? 0 : len_of_coded_match(ahead,m_off,lit);
#else
            l3 = len_of_coded_match(ahead,m_off,lit);
#endif

            lazy_match_min_gain = min_gain(ahead,lit,lit+ahead,l1,l2,l3);
            if (c->m_len >= m_len + lazy_match_min_gain)
            {
                c->lazy++;
                assert_match(swd,c->m_len,c->m_off);

                if (l3)
                {
                    /* code previous run */
                    op = code_run(c,op,ii,lit,ahead);
                    lit = 0;
                    /* code shortened match */
                    op = code_match(c,op,ahead,m_off);
                }
                else
                {
                    lit += ahead;
                    assert(ii + lit == c->bp);
                }
                goto lazy_match_done;
            }
        }


        assert(ii + lit + ahead == c->bp);

        /* 1 - code run */
        op = code_run(c,op,ii,lit,m_len);
        lit = 0;

        /* 2 - code match */
        op = code_match(c,op,m_len,m_off);
        swd->max_chain = max_chain;
        r = find_match(c,swd,m_len,1+ahead);
        assert(r == 0); LZO_UNUSED(r);

lazy_match_done: ;
    }


    /* store final run */
    if (lit > 0)
        op = STORE_RUN(c,op,ii,lit);

#if defined(LZO_EOF_CODE)
    *op++ = M4_MARKER | 1;
    *op++ = 0;
    *op++ = 0;
#endif

    c->codesize = pd(op, out);
    assert(c->textsize == in_len);

    *out_len = pd(op, out);

    if (c->cb && c->cb->nprogress)
        (*c->cb->nprogress)(c->cb, c->textsize, c->codesize, 0);

#if 0
    printf("%ld %ld -> %ld  %ld: %ld %ld %ld %ld %ld  %ld: %ld %ld %ld  %ld\n",
        (long) c->textsize, (long) in_len, (long) c->codesize,
        c->match_bytes, c->m1a_m, c->m1b_m, c->m2_m, c->m3_m, c->m4_m,
        c->lit_bytes, c->lit1_r, c->lit2_r, c->lit3_r, c->lazy);
#endif
    assert(c->lit_bytes + c->match_bytes == in_len);

    return LZO_E_OK;
}


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
lzo1x_999_compress_level    ( const lzo_bytep in , lzo_uint  in_len,
                                    lzo_bytep out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                              const lzo_bytep dict, lzo_uint dict_len,
                                    lzo_callback_p cb,
                                    int compression_level )
{
    static const struct
    {
        int try_lazy_parm;
        lzo_uint good_length;
        lzo_uint max_lazy;
        lzo_uint nice_length;
        lzo_uint max_chain;
        lzo_uint32_t flags;
    } c[9] = {
        /* faster compression */
        {   0,     0,     0,     8,    4,   0 },
        {   0,     0,     0,    16,    8,   0 },
        {   0,     0,     0,    32,   16,   0 },
        {   1,     4,     4,    16,   16,   0 },
        {   1,     8,    16,    32,   32,   0 },
        {   1,     8,    16,   128,  128,   0 },
        {   2,     8,    32,   128,  256,   0 },
        {   2,    32,   128, SWD_F, 2048,   1 },
        {   2, SWD_F, SWD_F, SWD_F, 4096,   1 }
        /* max. compression */
    };

    if (compression_level < 1 || compression_level > 9)
        return LZO_E_ERROR;

    compression_level -= 1;
    return lzo1x_999_compress_internal(in, in_len, out, out_len, wrkmem,
                                       dict, dict_len, cb,
                                       c[compression_level].try_lazy_parm,
                                       c[compression_level].good_length,
                                       c[compression_level].max_lazy,
#if 0
                                       c[compression_level].nice_length,
#else
                                       0,
#endif
                                       c[compression_level].max_chain,
                                       c[compression_level].flags);
}


/***********************************************************************
//
************************************************************************/

LZO_PUBLIC(int)
lzo1x_999_compress_dict     ( const lzo_bytep in , lzo_uint  in_len,
                                    lzo_bytep out, lzo_uintp out_len,
                                    lzo_voidp wrkmem,
                              const lzo_bytep dict, lzo_uint dict_len )
{
    return lzo1x_999_compress_level(in, in_len, out, out_len, wrkmem,
                                    dict, dict_len, 0, 8);
}

LZO_PUBLIC(int)
lzo1x_999_compress  ( const lzo_bytep in , lzo_uint  in_len,
                            lzo_bytep out, lzo_uintp out_len,
                            lzo_voidp wrkmem )
{
    return lzo1x_999_compress_level(in, in_len, out, out_len, wrkmem,
                                    NULL, 0, (lzo_callback_p) 0, 8);
}


/* vim:set ts=4 sw=4 et: */
