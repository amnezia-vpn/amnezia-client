/* lzo1f_1.c -- implementation of the LZO1F-1 compression algorithm

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
#include <lzo/lzo1f.h>


/***********************************************************************
//
************************************************************************/

#define M2_MAX_OFFSET   0x0800
#define M3_MAX_OFFSET   0x3fff
#define M3_MARKER       224


#ifndef LZO_HASH
#define LZO_HASH        LZO_HASH_LZO_INCREMENTAL_A
#endif
#define D_BITS          14
#define D_INDEX1(d,p)   d = DM(DMUL(0x21,DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)   d = (d & (D_MASK & 0x7ff)) ^ (D_HIGH | 0x1f)
#include "lzo_dict.h"


/***********************************************************************
// compress a block of data.
************************************************************************/

static __lzo_noinline
int do_compress          ( const lzo_bytep in , lzo_uint  in_len,
                                 lzo_bytep out, lzo_uintp out_len,
                                 lzo_voidp wrkmem )
{
    const lzo_bytep ip;
    lzo_bytep op;
    const lzo_bytep const in_end = in + in_len;
    const lzo_bytep const ip_end = in + in_len - 9;
    const lzo_bytep ii;
    lzo_dict_p const dict = (lzo_dict_p) wrkmem;

    op = out;
    ip = in;
    ii = ip;

    ip++;
    for (;;)
    {
        const lzo_bytep m_pos;
        LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, m_off, 0);
        lzo_uint m_len;
        lzo_uint dindex;
        lzo_uint lit;

        DINDEX1(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M3_MAX_OFFSET))
            goto literal;
#if 1
        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
        DINDEX2(dindex,ip);
#endif
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M3_MAX_OFFSET))
            goto literal;
        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
        goto literal;


try_match:
#if 0 && (LZO_OPT_UNALIGNED16)
        if (UA_GET_NE16(m_pos) != UA_GET_NE16(ip))
#else
        if (m_pos[0] != ip[0] || m_pos[1] != ip[1])
#endif
        {
        }
        else
        {
            if (m_pos[2] == ip[2])
            {
                m_pos += 3;
#if 0
                if (m_off <= M2_MAX_OFFSET)
                    goto match;
                if (lit <= 3)
                    goto match;
                if (lit == 3)           /* better compression, but slower */
                {
                    assert(op - 2 > out); op[-2] |= LZO_BYTE(3);
                    *op++ = *ii++; *op++ = *ii++; *op++ = *ii++;
                    goto code_match;
                }
                if (*m_pos == ip[3])
#endif
                    goto match;
            }
        }


    /* a literal */
literal:
        UPDATE_I(dict,0,dindex,ip,in);
        if (++ip >= ip_end)
            break;
        continue;


    /* a match */
match:
        UPDATE_I(dict,0,dindex,ip,in);
        /* store current literal run */
        lit = pd(ip,ii);
        if (lit > 0)
        {
            lzo_uint t = lit;

            if (t < 4 && op > out)
                op[-2] = LZO_BYTE(op[-2] | t);
            else if (t <= 31)
                *op++ = LZO_BYTE(t);
            else
            {
                lzo_uint tt = t - 31;

                *op++ = 0;
                while (tt > 255)
                {
                    tt -= 255;
                    UA_SET1(op, 0);
                    op++;
                }
                assert(tt > 0);
                *op++ = LZO_BYTE(tt);
            }
            do *op++ = *ii++; while (--t > 0);
        }
        assert(ii == ip);


        /* code the match */
        ip += 3;
        if (*m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++ ||
            *m_pos++ != *ip++ || *m_pos++ != *ip++ || *m_pos++ != *ip++)
        {
            --ip;
            m_len = pd(ip, ii);
            assert(m_len >= 3); assert(m_len <= 8);

            if (m_off <= M2_MAX_OFFSET)
            {
                m_off -= 1;
                *op++ = LZO_BYTE(((m_len - 2) << 5) | ((m_off & 7) << 2));
                *op++ = LZO_BYTE(m_off >> 3);
            }
            else if (m_len == 3 && m_off <= 2*M2_MAX_OFFSET && lit > 0)
            {
                m_off -= 1;
                /* m_off -= M2_MAX_OFFSET; */
                *op++ = LZO_BYTE(((m_off & 7) << 2));
                *op++ = LZO_BYTE(m_off >> 3);
            }
            else
            {
                *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
                *op++ = LZO_BYTE((m_off & 63) << 2);
                *op++ = LZO_BYTE(m_off >> 6);
            }
        }
        else
        {
            {
                const lzo_bytep end;
                end = in_end;
                while (ip < end && *m_pos == *ip)
                    { m_pos++; ip++; }
                m_len = pd(ip, ii);
            }
            assert(m_len >= 3);

            if (m_len <= 33)
                *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
            else
            {
                m_len -= 33;
                *op++ = M3_MARKER | 0;
                while (m_len > 255)
                {
                    m_len -= 255;
                    UA_SET1(op, 0);
                    op++;
                }
                assert(m_len > 0);
                *op++ = LZO_BYTE(m_len);
            }
            *op++ = LZO_BYTE((m_off & 63) << 2);
            *op++ = LZO_BYTE(m_off >> 6);
        }

        ii = ip;
        if (ip >= ip_end)
            break;
    }


    /* store final literal run */
    if (pd(in_end,ii) > 0)
    {
        lzo_uint t = pd(in_end,ii);

        if (t < 4 && op > out)
            op[-2] = LZO_BYTE(op[-2] | t);
        else if (t <= 31)
            *op++ = LZO_BYTE(t);
        else
        {
            lzo_uint tt = t - 31;

            *op++ = 0;
            while (tt > 255)
            {
                tt -= 255;
                UA_SET1(op, 0);
                op++;
            }
            assert(tt > 0);
            *op++ = LZO_BYTE(tt);
        }
        UA_COPYN(op, ii, t);
        op += t;
    }

    *out_len = pd(op, out);
    return LZO_E_OK;
}


/***********************************************************************
// public entry point
************************************************************************/

LZO_PUBLIC(int)
lzo1f_1_compress ( const lzo_bytep in , lzo_uint  in_len,
                         lzo_bytep out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
    lzo_bytep op = out;
    int r = LZO_E_OK;

    if (in_len == 0)
        *out_len = 0;
    else if (in_len <= 10)
    {
        *op++ = LZO_BYTE(in_len);
        do *op++ = *in++; while (--in_len > 0);
        *out_len = pd(op, out);
    }
    else
        r = do_compress(in,in_len,out,out_len,wrkmem);

    if (r == LZO_E_OK)
    {
        op = out + *out_len;
        op[0] = M3_MARKER | 1;
        op[1] = 0;
        op[2] = 0;
        *out_len += 3;
    }

    return r;
}


/* vim:set ts=4 sw=4 et: */
