/* lzo2a_d.ch -- implementation of the LZO2A decompression algorithm

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

#define _NEEDBYTE   NEED_IP(1)
#define _NEXTBYTE   (*ip++)

LZO_PUBLIC(int)
DO_DECOMPRESS    ( const lzo_bytep in , lzo_uint  in_len,
                         lzo_bytep out, lzo_uintp out_len,
                         lzo_voidp wrkmem )
{
    lzo_bytep op;
    const lzo_bytep ip;
    const lzo_bytep m_pos;

    lzo_uint t;
    const lzo_bytep const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_bytep const op_end = out + *out_len;
#endif

    lzo_uint32_t b = 0;     /* bit buffer */
    unsigned k = 0;         /* bits in bit buffer */

    LZO_UNUSED(wrkmem);

    op = out;
    ip = in;

    while (TEST_IP_AND_TEST_OP)
    {
        NEEDBITS(1);
        if (MASKBITS(1) == 0)
        {
            DUMPBITS(1);
            /* a literal */
            NEED_IP(1); NEED_OP(1);
            *op++ = *ip++;
            continue;
        }
        DUMPBITS(1);

        NEEDBITS(1);
        if (MASKBITS(1) == 0)
        {
            DUMPBITS(1);
            /* a M1 match */
            NEEDBITS(2);
            t = M1_MIN_LEN + (lzo_uint) MASKBITS(2);
            DUMPBITS(2);
            NEED_IP(1); NEED_OP(t);
            m_pos = op - 1 - *ip++;
            assert(m_pos >= out); assert(m_pos < op);
            TEST_LB(m_pos);
            MEMCPY_DS(op,m_pos,t);
            continue;
        }
        DUMPBITS(1);

        NEED_IP(2);
        t = *ip++;
        m_pos = op;
        m_pos -= (t & 31) | (((lzo_uint) *ip++) << 5);
        t >>= 5;
        if (t == 0)
        {
#if (SWD_N >= 8192)
            NEEDBITS(1);
            t = MASKBITS(1);
            DUMPBITS(1);
            if (t == 0)
                t = 10 - 1;
            else
            {
                /* a M3 match */
                m_pos -= 8192;      /* t << 13 */
                t = M3_MIN_LEN - 1;
            }
#else
            t = 10 - 1;
#endif
            NEED_IP(1);
            while (*ip == 0)
            {
                t += 255;
                ip++;
                TEST_OV(t);
                NEED_IP(1);
            }
            t += *ip++;
        }
        else
        {
#if defined(LZO_EOF_CODE)
            if (m_pos == op)
                goto eof_found;
#endif
            t += 2;
        }
        assert(m_pos >= out); assert(m_pos < op);
        TEST_LB(m_pos);
        NEED_OP(t);
        MEMCPY_DS(op,m_pos,t);
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
