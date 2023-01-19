/* lzo1.c -- implementation of the LZO1 algorithm

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
#include <lzo/lzo1.h>


/***********************************************************************
// The next two defines can be changed to customize LZO1.
// The default version is LZO1-5/1.
************************************************************************/

/* run bits (3 - 5) - the compressor and the decompressor
 * must use the same value. */
#if !defined(RBITS)
#  define RBITS     5
#endif

/* compression level (1 - 9) - this only affects the compressor.
 * 1 is fastest, 9 is best compression ratio */
#if !defined(CLEVEL)
#  define CLEVEL    1           /* fastest by default */
#endif


/* check configuration */
#if (RBITS < 3 || RBITS > 5)
#  error "invalid RBITS"
#endif
#if (CLEVEL < 1 || CLEVEL > 9)
#  error "invalid CLEVEL"
#endif


/***********************************************************************
// You should not have to change anything below this line.
************************************************************************/

/*
     Format of the marker byte


     76543210
     --------
     00000000   a long run (a 'R0' run) - there are short and long R0 runs
     000rrrrr   a short run with len r
     mmmooooo   a short match (len = 2+m, o = offset low bits)
     111ooooo   a long match (o = offset low bits)
*/


#define RSIZE   (1 << RBITS)
#define RMASK   (RSIZE - 1)

#define OBITS   RBITS               /* offset and run-length use same bits */
#define OSIZE   (1 << OBITS)
#define OMASK   (OSIZE - 1)

#define MBITS   (8 - OBITS)
#define MSIZE   (1 << MBITS)
#define MMASK   (MSIZE - 1)


/* sanity checks */
#if (OBITS < 3 || OBITS > 5)
#  error "invalid OBITS"
#endif
#if (MBITS < 3 || MBITS > 5)
#  error "invalid MBITS"
#endif


/***********************************************************************
// some macros to improve readability
************************************************************************/

/* Minimum len of a match */
#define MIN_MATCH           3
#define THRESHOLD           (MIN_MATCH - 1)

/* Minimum len of match coded in 2 bytes */
#define MIN_MATCH_SHORT     MIN_MATCH

/* Maximum len of match coded in 2 bytes */
#define MAX_MATCH_SHORT     (THRESHOLD + (MSIZE - 2))
/* MSIZE - 2: 0 is used to indicate runs,
 *            MSIZE-1 is used to indicate a long match */

/* Minimum len of match coded in 3 bytes */
#define MIN_MATCH_LONG      (MAX_MATCH_SHORT + 1)

/* Maximum len of match coded in 3 bytes */
#define MAX_MATCH_LONG      (MIN_MATCH_LONG + 255)

/* Maximum offset of a match */
#define MAX_OFFSET          (1 << (8 + OBITS))


/*

RBITS | MBITS  MIN  THR.  MSIZE  MAXS  MINL  MAXL   MAXO  R0MAX R0FAST
======+===============================================================
  3   |   5      3    2     32    32    33    288   2048    263   256
  4   |   4      3    2     16    16    17    272   4096    271   264
  5   |   3      3    2      8     8     9    264   8192    287   280

 */


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

#define DBITS       (8 + RBITS)
#include "lzo_dict.h"
#define DVAL_LEN    DVAL_LOOKAHEAD


/***********************************************************************
// get algorithm info, return memory required for compression
************************************************************************/

LZO_EXTERN(lzo_uint) lzo1_info ( int *rbits, int *clevel );

LZO_PUBLIC(lzo_uint)
lzo1_info ( int *rbits, int *clevel )
{
    if (rbits)
        *rbits = RBITS;
    if (clevel)
        *clevel = CLEVEL;
    return D_SIZE * lzo_sizeof(lzo_bytep);
}


/***********************************************************************
// decode a R0 literal run (a long run)
************************************************************************/

#define R0MIN   (RSIZE)             /* Minimum len of R0 run of literals */
#define R0MAX   (R0MIN + 255)       /* Maximum len of R0 run of literals */
#define R0FAST  (R0MAX & ~7u)       /* R0MAX aligned to 8 byte boundary */

#if (R0MAX - R0FAST != 7) || ((R0FAST & 7) != 0)
#  error "something went wrong"
#endif

/* 7 special codes from R0FAST+1 .. R0MAX
 * these codes mean long R0 runs with lengths
 * 512, 1024, 2048, 4096, 8192, 16384, 32768 */


/***********************************************************************
// LZO1 decompress a block of data.
//
// Could be easily translated into assembly code.
************************************************************************/

LZO_PUBLIC(int)
lzo1_decompress  ( const lzo_bytep in , lzo_uint  in_len,
                         lzo_bytep out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
    lzo_bytep op;
    const lzo_bytep ip;
    const lzo_bytep const ip_end = in + in_len;
    lzo_uint t;

    LZO_UNUSED(wrkmem);

    op = out;
    ip = in;
    while (ip < ip_end)
    {
        t = *ip++;  /* get marker */

        if (t < R0MIN)          /* a literal run */
        {
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
            }
            MEMCPY_DS(op,ip,t);
        }
        else                    /* a match */
        {
            lzo_uint tt;
            /* get match offset */
            const lzo_bytep m_pos = op - 1;
            m_pos -= (lzo_uint)(t & OMASK) | (((lzo_uint) *ip++) << OBITS);

            /* get match len */
            if (t >= ((MSIZE - 1) << OBITS))                /* all m-bits set */
                tt = (MIN_MATCH_LONG - THRESHOLD) + *ip++;  /* a long match */
            else
                tt = t >> OBITS;                            /* a short match */

            assert(m_pos >= out);
            assert(m_pos <  op);
            /* a half unrolled loop */
            *op++ = *m_pos++;
            *op++ = *m_pos++;
            MEMCPY_DS(op,m_pos,tt);
        }
    }

    *out_len = pd(op, out);

    /* the next line is the only check in the decompressor ! */
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));
}


/***********************************************************************
// code a literal run
************************************************************************/

static
#if LZO_ARCH_AVR
__lzo_noinline
#endif
lzo_bytep
store_run(lzo_bytep op, const lzo_bytep ii, lzo_uint r_len)
{
    assert(r_len > 0);

    /* code a long R0 run */
    if (r_len >= 512)
    {
        unsigned r_bits = 7;        /* 256 << 7 == 32768 */
        do {
            while (r_len >= (256u << r_bits))
            {
                r_len -= (256u << r_bits);
                *op++ = 0; *op++ = LZO_BYTE((R0FAST - R0MIN) + r_bits);
                MEMCPY8_DS(op, ii, (256u << r_bits));
            }
        } while (--r_bits > 0);
    }
    while (r_len >= R0FAST)
    {
        r_len -= R0FAST;
        *op++ = 0; *op++ = R0FAST - R0MIN;
        MEMCPY8_DS(op, ii, R0FAST);
    }

    if (r_len >= R0MIN)
    {
        /* code a short R0 run */
        *op++ = 0; *op++ = LZO_BYTE(r_len - R0MIN);
        MEMCPY_DS(op, ii, r_len);
    }
    else if (r_len > 0)
    {
        /* code a 'normal' run */
        *op++ = LZO_BYTE(r_len);
        MEMCPY_DS(op, ii, r_len);
    }

    assert(r_len == 0);
    return op;
}



/***********************************************************************
// LZO1 compress a block of data.
//
// Could be translated into assembly code without too much effort.
//
// I apologize for the spaghetti code, but it really helps the optimizer.
************************************************************************/

static int
do_compress    ( const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem )
{
    const lzo_bytep ip;
#if defined(__LZO_HASH_INCREMENTAL)
    lzo_xint dv;
#endif
    lzo_bytep op;
    const lzo_bytep m_pos;
    const lzo_bytep const ip_end = in+in_len - DVAL_LEN - MIN_MATCH_LONG;
    const lzo_bytep const in_end = in+in_len - DVAL_LEN;
    const lzo_bytep ii;
    lzo_dict_p const dict = (lzo_dict_p) wrkmem;

#if !defined(NDEBUG)
    const lzo_bytep m_pos_sav;
#endif

    op = out;
    ip = in;
    ii = ip;                /* point to start of literal run */
    if (in_len <= MIN_MATCH_LONG + DVAL_LEN + 1)
        goto the_end;

    /* init dictionary */
#if (LZO_DETERMINISTIC)
    BZERO8_PTR(wrkmem,sizeof(lzo_dict_t),D_SIZE);
#endif

    DVAL_FIRST(dv,ip);
    UPDATE_D(dict,0,dv,ip,in);
    ip++;
    DVAL_NEXT(dv,ip);

    do {
        LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, m_off, 0);
        lzo_uint dindex;

        DINDEX1(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS(m_pos,m_off,in,ip,MAX_OFFSET))
            goto literal;
        if (m_pos[0] == ip[0] && m_pos[1] == ip[1] && m_pos[2] == ip[2])
            goto match;
        DINDEX2(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS(m_pos,m_off,in,ip,MAX_OFFSET))
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
        m_pos_sav = m_pos;
#endif
        m_pos += 3;
        {
    /* we have found a match (of at least length 3) */
#if !defined(NDEBUG) && !(LZO_DICT_USE_PTR)
            assert((m_pos_sav = ip - m_off) == (m_pos - 3));
#endif
            /* 1) store the current literal run */
            if (pd(ip,ii) > 0)
            {
                lzo_uint t = pd(ip,ii);
#if 1
                /* OPTIMIZED: inline the copying of a short run */
                if (t < R0MIN)
                {
                    *op++ = LZO_BYTE(t);
                    MEMCPY_DS(op, ii, t);
                }
                else
#endif
                    op = store_run(op,ii,t);
            }

            /* 2a) compute match len */
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
                lzo_uint m_len;

            /* 2b) code a short match */
                    assert(pd(ip,m_pos) == m_off);
                --ip;   /* ran one too far, point back to non-match */
                m_len = pd(ip, ii);
                    assert(m_len >= MIN_MATCH_SHORT);
                    assert(m_len <= MAX_MATCH_SHORT);
                    assert(m_off > 0);
                    assert(m_off <= MAX_OFFSET);
                    assert(ii-m_off == m_pos_sav);
                    assert(lzo_memcmp(m_pos_sav,ii,m_len) == 0);
                --m_off;
                /* code short match len + low offset bits */
                *op++ = LZO_BYTE(((m_len - THRESHOLD) << OBITS) |
                                 (m_off & OMASK));
                /* code high offset bits */
                *op++ = LZO_BYTE(m_off >> OBITS);


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

            /* 2b) code the long match */
                m_len = pd(ip, ii);
                    assert(m_len >= MIN_MATCH_LONG);
                    assert(m_len <= MAX_MATCH_LONG);
                    assert(m_off > 0);
                    assert(m_off <= MAX_OFFSET);
                    assert(ii-m_off == m_pos_sav);
                    assert(lzo_memcmp(m_pos_sav,ii,m_len) == 0);
                    assert(pd(ip,m_pos) == m_off);
                --m_off;
                /* code long match flag + low offset bits */
                *op++ = LZO_BYTE(((MSIZE - 1) << OBITS) | (m_off & OMASK));
                /* code high offset bits */
                *op++ = LZO_BYTE(m_off >> OBITS);
                /* code match len */
                *op++ = LZO_BYTE(m_len - MIN_MATCH_LONG);


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

            /* ii now points to the start of next literal run */
            assert(ii == ip);
        }
    } while (ip < ip_end);



the_end:
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
// compress public entry point.
************************************************************************/

LZO_PUBLIC(int)
lzo1_compress ( const lzo_bytep in , lzo_uint  in_len,
                      lzo_bytep out, lzo_uintp out_len,
                      lzo_voidp wrkmem )
{
    int r = LZO_E_OK;

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

    return r;
}


/* vim:set ts=4 sw=4 et: */
