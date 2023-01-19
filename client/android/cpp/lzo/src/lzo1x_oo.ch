/* lzo1x_oo.ch -- LZO1X compressed data optimizer

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


#define TEST_IP     (ip < ip_end)
#define TEST_OP     (op <= op_end)
#define TEST_IP_AND_TEST_OP (TEST_IP && TEST_OP)

#define NO_LIT      LZO_UINT_MAX


/***********************************************************************
//
************************************************************************/

static void copy2(lzo_bytep ip, const lzo_bytep m_pos, lzo_uint off)
{
    assert(off > 0);
    ip[0] = m_pos[0];
    if (off == 1)
        ip[1] = m_pos[0];
    else
        ip[1] = m_pos[1];
}


static void copy3(lzo_bytep ip, const lzo_bytep m_pos, lzo_uint off)
{
    assert(off > 0);
    ip[0] = m_pos[0];
    if (off == 1)
    {
        ip[2] = ip[1] = m_pos[0];
    }
    else if (off == 2)
    {
        ip[1] = m_pos[1];
        ip[2] = m_pos[0];
    }
    else
    {
        ip[1] = m_pos[1];
        ip[2] = m_pos[2];
    }
}


/***********************************************************************
// optimize a block of data.
************************************************************************/

LZO_PUBLIC(int)
DO_OPTIMIZE          (       lzo_bytep in , lzo_uint  in_len,
                             lzo_bytep out, lzo_uintp out_len,
                             lzo_voidp wrkmem )
{
    lzo_bytep op;
    lzo_bytep ip;
    lzo_uint t;
    lzo_bytep m_pos;
    lzo_bytep const ip_end = in + in_len;
    lzo_bytep const op_end = out + *out_len;
    lzo_bytep litp = NULL;
    lzo_uint lit = 0;
    lzo_uint next_lit = NO_LIT;
    lzo_uint nl;
    unsigned long o_m1_a = 0, o_m1_b = 0, o_m2 = 0, o_m3_a = 0, o_m3_b = 0;

    LZO_UNUSED(wrkmem);

    *out_len = 0;

    op = out;
    ip = in;

    assert(in_len >= 3);
    if (*ip > 17)
    {
        t = *ip++ - 17;
        if (t < 4)
            goto match_next;
        goto first_literal_run;
    }
    assert(*ip < 16 || (*ip == 17 && in_len == 3));

    while (TEST_IP_AND_TEST_OP)
    {
        t = *ip++;
        if (t >= 16)
            goto match;
        /* a literal run */
        litp = ip - 1;
        if (t == 0)
        {
            t = 15;
            while (*ip == 0)
                { t += 255; ip++; }
            t += *ip++;
        }
        lit = t + 3;
        /* copy literals */
copy_literal_run:
        *op++ = *ip++; *op++ = *ip++; *op++ = *ip++;
first_literal_run:
        do *op++ = *ip++; while (--t > 0);


        t = *ip++;

        if (t >= 16)
            goto match;
#if defined(LZO1X)
        m_pos = op - 1 - 0x800;
#elif defined(LZO1Y)
        m_pos = op - 1 - 0x400;
#endif
        m_pos -= t >> 2;
        m_pos -= *ip++ << 2;
        *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos++;
        lit = 0;
        goto match_done;


        /* handle matches */
        do {
            if (t < 16)                     /* a M1 match */
            {
                m_pos = op - 1;
                m_pos -= t >> 2;
                m_pos -= *ip++ << 2;

                if (litp == NULL)
                    goto copy_m1;

                /* assert that there was a match just before */
                assert(lit >= 1 && lit <= 3);
                assert(litp == ip - 2 - lit - 2);
                assert((lzo_uint)(*litp & 3) == lit);
                nl = ip[-2] & 3;
                /* test if a match follows */
                if (nl == 0 && lit == 1 && ip[0] >= 16)
                {
                    next_lit = nl;
                    /* adjust length of previous short run */
                    lit += 2;
                    *litp = LZO_BYTE((*litp & ~3) | lit);
                    /* copy over the 2 literals that replace the match */
                    copy2(ip-2,m_pos,pd(op,m_pos));
                    o_m1_a++;
                }
                /* test if a literal run follows */
                else if (nl == 0 && ip[0] < 16 && ip[0] != 0 &&
                         (lit + 2 + ip[0] < 16))
                {
                    t = *ip++;
                    /* remove short run */
                    *litp &= ~3;
                    /* copy over the 2 literals that replace the match */
                    copy2(ip-3+1,m_pos,pd(op,m_pos));
                    /* move literals 1 byte ahead */
                    litp += 2;
                    if (lit > 0)
                        lzo_memmove(litp+1,litp,lit);
                    /* insert new length of long literal run */
                    lit += 2 + t + 3; assert(lit <= 18);
                    *litp = LZO_BYTE(lit - 3);

                    o_m1_b++;
                    *op++ = *m_pos++; *op++ = *m_pos++;
                    goto copy_literal_run;
                }
copy_m1:
                *op++ = *m_pos++; *op++ = *m_pos++;
            }
            else
            {
match:
                if (t >= 64)                /* a M2 match */
                {
                    m_pos = op - 1;
#if defined(LZO1X)
                    m_pos -= (t >> 2) & 7;
                    m_pos -= *ip++ << 3;
                    t = (t >> 5) - 1;
#elif defined(LZO1Y)
                    m_pos -= (t >> 2) & 3;
                    m_pos -= *ip++ << 2;
                    t = (t >> 4) - 3;
#endif
                    if (litp == NULL)
                        goto copy_m;

                    nl = ip[-2] & 3;
                    /* test if in beetween two long literal runs */
                    if (t == 1 && lit > 3 && nl == 0 &&
                        ip[0] < 16 && ip[0] != 0 && (lit + 3 + ip[0] < 16))
                    {
                        assert(*litp == lit - 3);
                        t = *ip++;
                        /* copy over the 3 literals that replace the match */
                        copy3(ip-1-2,m_pos,pd(op,m_pos));
                        /* set new length of previous literal run */
                        lit += 3 + t + 3; assert(lit <= 18);
                        *litp = LZO_BYTE(lit - 3);
                        o_m2++;
                        *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos++;
                        goto copy_literal_run;
                    }
                }
                else
                {
                    if (t >= 32)            /* a M3 match */
                    {
                        t &= 31;
                        if (t == 0)
                        {
                            t = 31;
                            while (*ip == 0)
                                { t += 255; ip++; }
                            t += *ip++;
                        }
                        m_pos = op - 1;
                        m_pos -= *ip++ >> 2;
                        m_pos -= *ip++ << 6;
                    }
                    else                    /* a M4 match */
                    {
                        m_pos = op;
                        m_pos -= (t & 8) << 11;
                        t &= 7;
                        if (t == 0)
                        {
                            t = 7;
                            while (*ip == 0)
                                { t += 255; ip++; }
                            t += *ip++;
                        }
                        m_pos -= *ip++ >> 2;
                        m_pos -= *ip++ << 6;
                        if (m_pos == op)
                            goto eof_found;
                        m_pos -= 0x4000;
                    }
                    if (litp == NULL)
                        goto copy_m;

                    nl = ip[-2] & 3;
                    /* test if in beetween two matches */
                    if (t == 1 && lit == 0 && nl == 0 && ip[0] >= 16)
                    {
                        assert(litp == ip - 3 - lit - 2);
                        assert((lzo_uint)(*litp & 3) == lit);
                        next_lit = nl;
                        /* make a previous short run */
                        lit += 3;
                        *litp = LZO_BYTE((*litp & ~3) | lit);
                        /* copy over the 3 literals that replace the match */
                        copy3(ip-3,m_pos,pd(op,m_pos));
                        o_m3_a++;
                    }
                    /* test if a literal run follows */
                    else if (t == 1 && lit <= 3 && nl == 0 &&
                             ip[0] < 16 && ip[0] != 0 && (lit + 3 + ip[0] < 16))
                    {
                        assert(litp == ip - 3 - lit - 2);
                        assert((lzo_uint)(*litp & 3) == lit);
                        t = *ip++;
                        /* remove short run */
                        *litp &= ~3;
                        /* copy over the 3 literals that replace the match */
                        copy3(ip-4+1,m_pos,pd(op,m_pos));
                        /* move literals 1 byte ahead */
                        litp += 2;
                        if (lit > 0)
                            lzo_memmove(litp+1,litp,lit);
                        /* insert new length of long literal run */
                        lit += 3 + t + 3; assert(lit <= 18);
                        *litp = LZO_BYTE(lit - 3);

                        o_m3_b++;
                        *op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos++;
                        goto copy_literal_run;
                    }
                }
copy_m:
                *op++ = *m_pos++; *op++ = *m_pos++;
                do *op++ = *m_pos++; while (--t > 0);
            }

match_done:
            if (next_lit == NO_LIT)
            {
                t = ip[-2] & 3;
                lit = t;
                litp = ip - 2;
            }
            else
                t = next_lit;
            assert(t <= 3);
            next_lit = NO_LIT;
            if (t == 0)
                break;
            /* copy literals */
match_next:
            do *op++ = *ip++; while (--t > 0);
            t = *ip++;
        } while (TEST_IP_AND_TEST_OP);
    }

    /* no EOF code was found */
    *out_len = pd(op, out);
    return LZO_E_EOF_NOT_FOUND;

eof_found:
    assert(t == 1);
#if 0
    printf("optimize: %5lu %5lu   %5lu   %5lu %5lu\n",
           o_m1_a, o_m1_b, o_m2, o_m3_a, o_m3_b);
#endif
    LZO_UNUSED(o_m1_a); LZO_UNUSED(o_m1_b); LZO_UNUSED(o_m2);
    LZO_UNUSED(o_m3_a); LZO_UNUSED(o_m3_b);
    *out_len = pd(op, out);
    return (ip == ip_end ? LZO_E_OK :
           (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));
}


/* vim:set ts=4 sw=4 et: */
