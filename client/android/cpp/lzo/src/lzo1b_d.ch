/* lzo1b_d.ch -- implementation of the LZO1B decompression algorithm

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
DO_DECOMPRESS    ( const lzo_bytep in , lzo_uint  in_len,
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

    op = out;
    ip = in;

    while (TEST_IP_AND_TEST_OP)
    {
        t = *ip++;      /* get marker */

        if (t < R0MIN)      /* a literal run */
        {
            if (t == 0)             /* a R0 literal run */
            {
                NEED_IP(1);
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

                    NEED_IP(t); NEED_OP(t);
#if 1 && (LZO_OPT_UNALIGNED32)
                    do {
                        UA_COPY4(op+0, ip+0);
                        UA_COPY4(op+4, ip+4);
                        op += 8; ip += 8;
                        t -= 8;
                    } while (t > 0);
#else
                    MEMCPY8_DS(op,ip,t);
#endif
                    continue;
                }
                t += R0MIN;                         /* a short R0 run */
            }

            NEED_IP(t); NEED_OP(t);
            /* copy literal run */
#if 1 && (LZO_OPT_UNALIGNED32)
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
            {
#if (M3O_BITS < 7)
literal1:
#endif
                do *op++ = *ip++; while (--t > 0);
            }

#if (M3O_BITS == 7)
literal2:
#endif

            /* after a literal a match must follow */
            while (TEST_IP_AND_TEST_OP)
            {
                t = *ip++;          /* get R1 marker */
                if (t >= R0MIN)
                    goto match;

                NEED_IP(2); NEED_OP(M2_MIN_LEN + 1);

            /* R1 match - a M2_MIN_LEN match + 1 byte literal */
                assert((t & M2O_MASK) == t);
                m_pos = op - M2_MIN_OFFSET;
                m_pos -= t | (((lzo_uint) *ip++) << M2O_BITS);
                assert(m_pos >= out); assert(m_pos < op);
                TEST_LB(m_pos);
                COPY_M2;
                *op++ = *ip++;
            }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
            break;
#endif
        }

match:

        if (t >= M2_MARKER)             /* a M2 match */
        {
            /* get match offset */
            NEED_IP(1);
            m_pos = op - M2_MIN_OFFSET;
            m_pos -= (t & M2O_MASK) | (((lzo_uint) *ip++) << M2O_BITS);
            assert(m_pos >= out); assert(m_pos < op);
            TEST_LB(m_pos);

            /* get match len */
            t = (t >> M2O_BITS) - 1;
            NEED_OP(t + M2_MIN_LEN - 1);
            COPY_M2X;
            MEMCPY_DS(op,m_pos,t);
        }
        else                            /* a M3 or M4 match */
        {
            /* get match len */
            t &= M3L_MASK;
            if (t == 0)         /* a M4 match */
            {
                NEED_IP(1);
                while (*ip == 0)
                {
                    t += 255;
                    ip++;
                    TEST_OV(t);
                    NEED_IP(1);
                }
                t += (M4_MIN_LEN - M3_MIN_LEN) + *ip++;
            }

            /* get match offset */
            NEED_IP(2);
            m_pos = op - (M3_MIN_OFFSET - M3_EOF_OFFSET);
            m_pos -= *ip++ & M3O_MASK;
            m_pos -= (lzo_uint)(*ip++) << M3O_BITS;
#if defined(LZO_EOF_CODE)
            if (m_pos == op)
                goto eof_found;
#endif

            /* copy match */
            assert(m_pos >= out); assert(m_pos < op);
            TEST_LB(m_pos); NEED_OP(t + M3_MIN_LEN - 1);
#if (LZO_OPT_UNALIGNED32)
            if (t >= 2 * 4 - (M3_MIN_LEN - 1) && (op - m_pos) >= 4)
            {
                UA_COPY4(op, m_pos);
                op += 4; m_pos += 4; t -= 4 - (M3_MIN_LEN - 1);
                do {
                    UA_COPY4(op, m_pos);
                    op += 4; m_pos += 4; t -= 4;
                } while (t >= 4);
                if (t > 0) do *op++ = *m_pos++; while (--t > 0);
            }
            else
#endif
            {
            COPY_M3X;
            MEMCPY_DS(op,m_pos,t);
            }


#if (M3O_BITS < 7)
            t = ip[-2] >> M3O_BITS;
            if (t)
            {
                NEED_IP(t); NEED_OP(t);
                goto literal1;
            }
#elif (M3O_BITS == 7)
            /* optimized version */
            if (ip[-2] & (1 << M3O_BITS))
            {
                NEED_IP(1); NEED_OP(1);
                *op++ = *ip++;
                goto literal2;
            }
#endif
        }
    }


#if defined(LZO_EOF_CODE)
#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    /* no EOF code was found */
    *out_len = pd(op, out);
    return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
    assert(t == 1);
#endif
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
