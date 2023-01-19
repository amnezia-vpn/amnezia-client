/* lzo1f_d.ch -- implementation of the LZO1F decompression algorithm

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


#include "lzo1_d.ch"


/***********************************************************************
// decompress a block of data.
************************************************************************/

LZO_PUBLIC(int)
DO_DECOMPRESS  ( const lzo_bytep in , lzo_uint  in_len,
                       lzo_bytep out, lzo_uintp out_len,
                       lzo_voidp wrkmem )
{
    lzo_bytep op;
    const lzo_bytep ip;
    lzo_uint t;
    const lzo_bytep m_pos;

    const lzo_bytep const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_bytep const op_end = out + *out_len;
#endif

    LZO_UNUSED(wrkmem);

    *out_len = 0;

    op = out;
    ip = in;

    while (TEST_IP_AND_TEST_OP)
    {
        t = *ip++;
        if (t > 31)
            goto match;

        /* a literal run */
        if (t == 0)
        {
            NEED_IP(1);
            while (*ip == 0)
            {
                t += 255;
                ip++;
                TEST_IV(t);
                NEED_IP(1);
            }
            t += 31 + *ip++;
        }
        /* copy literals */
        assert(t > 0); NEED_OP(t); NEED_IP(t+1);
#if (LZO_OPT_UNALIGNED32)
        if (t >= 4)
        {
            do {
                UA_COPY4(op, ip);
                op += 4; ip += 4; t -= 4;
            } while (t >= 4);
            if (t > 0) do *op++ = *ip++; while (--t > 0);
        }
        else
#endif
        do *op++ = *ip++; while (--t > 0);

        t = *ip++;

        while (TEST_IP_AND_TEST_OP)
        {
            /* handle matches */
            if (t < 32)
            {
                m_pos = op - 1 - 0x800;
                m_pos -= (t >> 2) & 7;
                m_pos -= *ip++ << 3;
                TEST_LB(m_pos); NEED_OP(3);
                *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos++;
            }
            else
            {
match:
                if (t < M3_MARKER)
                {
                    m_pos = op - 1;
                    m_pos -= (t >> 2) & 7;
                    m_pos -= *ip++ << 3;
                    t >>= 5;
                    TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
                    goto copy_match;
                }
                else
                {
                    t &= 31;
                    if (t == 0)
                    {
                        NEED_IP(1);
                        while (*ip == 0)
                        {
                            t += 255;
                            ip++;
                            TEST_OV(t);
                            NEED_IP(1);
                        }
                        t += 31 + *ip++;
                    }
                    NEED_IP(2);
                    m_pos = op;
#if (LZO_OPT_UNALIGNED16) && (LZO_ABI_LITTLE_ENDIAN)
                    m_pos -= UA_GET_LE16(ip) >> 2;
                    ip += 2;
#else
                    m_pos -= *ip++ >> 2;
                    m_pos -= *ip++ << 6;
#endif
                    if (m_pos == op)
                        goto eof_found;
                }

                /* copy match */
                TEST_LB(m_pos); assert(t > 0); NEED_OP(t+3-1);
#if (LZO_OPT_UNALIGNED32)
                if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
                {
                    UA_COPY4(op, m_pos);
                    op += 4; m_pos += 4; t -= 4 - (3 - 1);
                    do {
                        UA_COPY4(op, m_pos);
                        op += 4; m_pos += 4; t -= 4;
                    } while (t >= 4);
                    if (t > 0) do *op++ = *m_pos++; while (--t > 0);
                }
                else
#endif
                {
copy_match:
                *op++ = *m_pos++; *op++ = *m_pos++;
                do *op++ = *m_pos++; while (--t > 0);
                }
            }
            t = ip[-2] & 3;
            if (t == 0)
                break;

            /* copy literals */
            assert(t > 0); NEED_OP(t); NEED_IP(t+1);
            do *op++ = *ip++; while (--t > 0);
            t = *ip++;
        }
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    /* no EOF code was found */
    *out_len = pd(op, out);
    return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
    assert(t == 1);
    *out_len = pd(op, out);
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));


#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = pd(op, out);
    return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = pd(op, out);
    return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = pd(op, out);
    return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}


/* vim:set ts=4 sw=4 et: */
