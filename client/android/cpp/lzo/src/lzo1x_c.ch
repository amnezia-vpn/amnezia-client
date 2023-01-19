/* lzo1x_c.ch -- implementation of the LZO1[XY]-1 compression algorithm

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



#if 1 && defined(DO_COMPRESS) && !defined(do_compress)
   /* choose a unique name to better help PGO optimizations */
#  define do_compress       LZO_PP_ECONCAT2(DO_COMPRESS,_core)
#endif


/***********************************************************************
// compress a block of data.
************************************************************************/

static __lzo_noinline lzo_uint
do_compress ( const lzo_bytep in , lzo_uint  in_len,
                    lzo_bytep out, lzo_uintp out_len,
                    lzo_uint  ti,  lzo_voidp wrkmem)
{
    const lzo_bytep ip;
    lzo_bytep op;
    const lzo_bytep const in_end = in + in_len;
    const lzo_bytep const ip_end = in + in_len - 20;
    const lzo_bytep ii;
    lzo_dict_p const dict = (lzo_dict_p) wrkmem;

    op = out;
    ip = in;
    ii = ip;

    ip += ti < 4 ? 4 - ti : 0;
    for (;;)
    {
        const lzo_bytep m_pos;
#if !(LZO_DETERMINISTIC)
        LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, m_off, 0);
        lzo_uint m_len;
        lzo_uint dindex;
next:
        if __lzo_unlikely(ip >= ip_end)
            break;
        DINDEX1(dindex,ip);
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M4_MAX_OFFSET))
            goto literal;
#if 1
        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
        DINDEX2(dindex,ip);
#endif
        GINDEX(m_pos,m_off,dict,dindex,in);
        if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M4_MAX_OFFSET))
            goto literal;
        if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
            goto try_match;
        goto literal;

try_match:
#if (LZO_OPT_UNALIGNED32)
        if (UA_GET_NE32(m_pos) != UA_GET_NE32(ip))
#else
        if (m_pos[0] != ip[0] || m_pos[1] != ip[1] || m_pos[2] != ip[2] || m_pos[3] != ip[3])
#endif
        {
            /* a literal */
literal:
            UPDATE_I(dict,0,dindex,ip,in);
            ip += 1 + ((ip - ii) >> 5);
            continue;
        }
/*match:*/
        UPDATE_I(dict,0,dindex,ip,in);
#else
        lzo_uint m_off;
        lzo_uint m_len;
        {
        lzo_uint32_t dv;
        lzo_uint dindex;
literal:
        ip += 1 + ((ip - ii) >> 5);
next:
        if __lzo_unlikely(ip >= ip_end)
            break;
        dv = UA_GET_LE32(ip);
        dindex = DINDEX(dv,ip);
        GINDEX(m_off,m_pos,in+dict,dindex,in);
        UPDATE_I(dict,0,dindex,ip,in);
        if __lzo_unlikely(dv != UA_GET_LE32(m_pos))
            goto literal;
        }
#endif

    /* a match */

        ii -= ti; ti = 0;
        {
        lzo_uint t = pd(ip,ii);
        if (t != 0)
        {
            if (t <= 3)
            {
                op[-2] = LZO_BYTE(op[-2] | t);
#if (LZO_OPT_UNALIGNED32)
                UA_COPY4(op, ii);
                op += t;
#else
                { do *op++ = *ii++; while (--t > 0); }
#endif
            }
#if (LZO_OPT_UNALIGNED32) || (LZO_OPT_UNALIGNED64)
            else if (t <= 16)
            {
                *op++ = LZO_BYTE(t - 3);
                UA_COPY8(op, ii);
                UA_COPY8(op+8, ii+8);
                op += t;
            }
#endif
            else
            {
                if (t <= 18)
                    *op++ = LZO_BYTE(t - 3);
                else
                {
                    lzo_uint tt = t - 18;
                    *op++ = 0;
                    while __lzo_unlikely(tt > 255)
                    {
                        tt -= 255;
                        UA_SET1(op, 0);
                        op++;
                    }
                    assert(tt > 0);
                    *op++ = LZO_BYTE(tt);
                }
#if (LZO_OPT_UNALIGNED32) || (LZO_OPT_UNALIGNED64)
                do {
                    UA_COPY8(op, ii);
                    UA_COPY8(op+8, ii+8);
                    op += 16; ii += 16; t -= 16;
                } while (t >= 16); if (t > 0)
#endif
                { do *op++ = *ii++; while (--t > 0); }
            }
        }
        }
        m_len = 4;
        {
#if (LZO_OPT_UNALIGNED64)
        lzo_uint64_t v;
        v = UA_GET_NE64(ip + m_len) ^ UA_GET_NE64(m_pos + m_len);
        if __lzo_unlikely(v == 0) {
            do {
                m_len += 8;
                v = UA_GET_NE64(ip + m_len) ^ UA_GET_NE64(m_pos + m_len);
                if __lzo_unlikely(ip + m_len >= ip_end)
                    goto m_len_done;
            } while (v == 0);
        }
#if (LZO_ABI_BIG_ENDIAN) && defined(lzo_bitops_ctlz64)
        m_len += lzo_bitops_ctlz64(v) / CHAR_BIT;
#elif (LZO_ABI_BIG_ENDIAN)
        if ((v >> (64 - CHAR_BIT)) == 0) do {
            v <<= CHAR_BIT;
            m_len += 1;
        } while ((v >> (64 - CHAR_BIT)) == 0);
#elif (LZO_ABI_LITTLE_ENDIAN) && defined(lzo_bitops_cttz64)
        m_len += lzo_bitops_cttz64(v) / CHAR_BIT;
#elif (LZO_ABI_LITTLE_ENDIAN)
        if ((v & UCHAR_MAX) == 0) do {
            v >>= CHAR_BIT;
            m_len += 1;
        } while ((v & UCHAR_MAX) == 0);
#else
        if (ip[m_len] == m_pos[m_len]) do {
            m_len += 1;
        } while (ip[m_len] == m_pos[m_len]);
#endif
#elif (LZO_OPT_UNALIGNED32)
        lzo_uint32_t v;
        v = UA_GET_NE32(ip + m_len) ^ UA_GET_NE32(m_pos + m_len);
        if __lzo_unlikely(v == 0) {
            do {
                m_len += 4;
                v = UA_GET_NE32(ip + m_len) ^ UA_GET_NE32(m_pos + m_len);
                if (v != 0)
                    break;
                m_len += 4;
                v = UA_GET_NE32(ip + m_len) ^ UA_GET_NE32(m_pos + m_len);
                if __lzo_unlikely(ip + m_len >= ip_end)
                    goto m_len_done;
            } while (v == 0);
        }
#if (LZO_ABI_BIG_ENDIAN) && defined(lzo_bitops_ctlz32)
        m_len += lzo_bitops_ctlz32(v) / CHAR_BIT;
#elif (LZO_ABI_BIG_ENDIAN)
        if ((v >> (32 - CHAR_BIT)) == 0) do {
            v <<= CHAR_BIT;
            m_len += 1;
        } while ((v >> (32 - CHAR_BIT)) == 0);
#elif (LZO_ABI_LITTLE_ENDIAN) && defined(lzo_bitops_cttz32)
        m_len += lzo_bitops_cttz32(v) / CHAR_BIT;
#elif (LZO_ABI_LITTLE_ENDIAN)
        if ((v & UCHAR_MAX) == 0) do {
            v >>= CHAR_BIT;
            m_len += 1;
        } while ((v & UCHAR_MAX) == 0);
#else
        if (ip[m_len] == m_pos[m_len]) do {
            m_len += 1;
        } while (ip[m_len] == m_pos[m_len]);
#endif
#else
        if __lzo_unlikely(ip[m_len] == m_pos[m_len]) {
            do {
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if (ip[m_len] != m_pos[m_len])
                    break;
                m_len += 1;
                if __lzo_unlikely(ip + m_len >= ip_end)
                    goto m_len_done;
            } while (ip[m_len] == m_pos[m_len]);
        }
#endif
        }
m_len_done:
        m_off = pd(ip,m_pos);
        ip += m_len;
        ii = ip;
        if (m_len <= M2_MAX_LEN && m_off <= M2_MAX_OFFSET)
        {
            m_off -= 1;
#if defined(LZO1X)
            *op++ = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
            *op++ = LZO_BYTE(m_off >> 3);
#elif defined(LZO1Y)
            *op++ = LZO_BYTE(((m_len + 1) << 4) | ((m_off & 3) << 2));
            *op++ = LZO_BYTE(m_off >> 2);
#endif
        }
        else if (m_off <= M3_MAX_OFFSET)
        {
            m_off -= 1;
            if (m_len <= M3_MAX_LEN)
                *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
            else
            {
                m_len -= M3_MAX_LEN;
                *op++ = M3_MARKER | 0;
                while __lzo_unlikely(m_len > 255)
                {
                    m_len -= 255;
                    UA_SET1(op, 0);
                    op++;
                }
                *op++ = LZO_BYTE(m_len);
            }
            *op++ = LZO_BYTE(m_off << 2);
            *op++ = LZO_BYTE(m_off >> 6);
        }
        else
        {
            m_off -= 0x4000;
            if (m_len <= M4_MAX_LEN)
                *op++ = LZO_BYTE(M4_MARKER | ((m_off >> 11) & 8) | (m_len - 2));
            else
            {
                m_len -= M4_MAX_LEN;
                *op++ = LZO_BYTE(M4_MARKER | ((m_off >> 11) & 8));
                while __lzo_unlikely(m_len > 255)
                {
                    m_len -= 255;
                    UA_SET1(op, 0);
                    op++;
                }
                *op++ = LZO_BYTE(m_len);
            }
            *op++ = LZO_BYTE(m_off << 2);
            *op++ = LZO_BYTE(m_off >> 6);
        }
        goto next;
    }

    *out_len = pd(op, out);
    return pd(in_end,ii-ti);
}


/***********************************************************************
// public entry point
************************************************************************/

LZO_PUBLIC(int)
DO_COMPRESS      ( const lzo_bytep in , lzo_uint  in_len,
                         lzo_bytep out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
    const lzo_bytep ip = in;
    lzo_bytep op = out;
    lzo_uint l = in_len;
    lzo_uint t = 0;

    while (l > 20)
    {
        lzo_uint ll = l;
        lzo_uintptr_t ll_end;
#if 0 || (LZO_DETERMINISTIC)
        ll = LZO_MIN(ll, 49152);
#endif
        ll_end = (lzo_uintptr_t)ip + ll;
        if ((ll_end + ((t + ll) >> 5)) <= ll_end || (const lzo_bytep)(ll_end + ((t + ll) >> 5)) <= ip + ll)
            break;
#if (LZO_DETERMINISTIC)
        lzo_memset(wrkmem, 0, ((lzo_uint)1 << D_BITS) * sizeof(lzo_dict_t));
#endif
        t = do_compress(ip,ll,op,out_len,t,wrkmem);
        ip += ll;
        op += *out_len;
        l  -= ll;
    }
    t += l;

    if (t > 0)
    {
        const lzo_bytep ii = in + in_len - t;

        if (op == out && t <= 238)
            *op++ = LZO_BYTE(17 + t);
        else if (t <= 3)
            op[-2] = LZO_BYTE(op[-2] | t);
        else if (t <= 18)
            *op++ = LZO_BYTE(t - 3);
        else
        {
            lzo_uint tt = t - 18;

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

    *op++ = M4_MARKER | 1;
    *op++ = 0;
    *op++ = 0;

    *out_len = pd(op, out);
    return LZO_E_OK;
}


/* vim:set ts=4 sw=4 et: */
