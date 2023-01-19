/* lzo1b_sm.ch -- implementation of the LZO1B compression algorithm

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
   part of the implementation of the library and is subject
   to change.
 */



/***********************************************************************
// search for a match
************************************************************************/

#if (DD_BITS == 0)

        /* search ip in the dictionary */
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


#else /* (DD_BITS == 0) */



        /* search ip in the deepened dictionary */
        {
            lzo_dict_p d = &dict [ DINDEX(dv,ip) ];
            const lzo_bytep ip_sav;
            unsigned j = DD_SIZE;
            lzo_uint x_len;
            LZO_DEFINE_UNINITIALIZED_VAR(lzo_uint, x_off, 0);

            DVAL_ASSERT(dv,ip);

            ip_sav = ip;
            m_len = 0;
            do {
#if !defined(NDEBUG)
                const lzo_bytep z_pos = NULL;
#endif
#if (LZO_DICT_USE_PTR)
                m_pos = *d;
                assert((z_pos = m_pos) == *d);
#if (LZO_DETERMINISTIC)
                assert(m_pos == NULL || m_pos >= in);
                assert(m_pos == NULL || m_pos < ip);
#endif
#else
                x_off = *d;
#endif
                assert(ip == ip_sav);

                if (LZO_CHECK_MPOS(m_pos,x_off,in,ip,MAX_OFFSET))
#if (CLEVEL == 9)
                    *d = DENTRY(ip,in);
#else
                    ((void)(0));
#endif
                else if (m_pos[m_len] != ip[m_len])
                    ((void)(0));
                else if (*m_pos++ == *ip++ && *m_pos++ == *ip++ && *m_pos++ == *ip++)
                {
#if !(LZO_DICT_USE_PTR)
                    assert((z_pos = ip - 3 - x_off) == (m_pos - 3));
#endif
                    /* a match */
                    if (MATCH_M2)
                    {
                        x_len = pd((ip - 1), ip_sav);
                        if (x_len > m_len)
                        {
                            m_len = x_len;
                            m_off = x_off;
                            assert((m_pos_sav = z_pos) != NULL);
                        }
#if (CLEVEL == 9)
                        /* try to find a closer match */
                        else if (x_len == m_len && x_off < m_off)
                        {
                            m_off = x_off;
                            assert((m_pos_sav = z_pos) != NULL);
                        }
#endif
                    }
                    else
                    {
                        assert((ip - ip_sav) == M2_MAX_LEN + 1);
#if (CLEVEL == 9)
#if defined(MATCH_IP_END)
                        {
                            const lzo_bytep end;
                            end = MATCH_IP_END;
                            while (ip < end && *m_pos == *ip)
                                { m_pos++; ip++; }
                            assert(ip <= in_end);
                            x_len = pd(ip, ip_sav);
                        }
                        if (x_len > m_len)
                        {
                            m_len = x_len;
                            m_off = x_off;
                            assert((m_pos_sav = z_pos) != NULL);
                            if (ip >= MATCH_IP_END)
                            {
                                ip = ip_sav;
#if 0
                                /* not needed - we are at the end */
                                d -= DD_SIZE - j;
                                assert(d == &dict [ DINDEX(dv,ip) ]);
                                UPDATE_P(d,drun,ip,in);
#endif
                                goto match;
                            }
                        }
                        else if (x_len == m_len && x_off < m_off)
                        {
                            m_off = x_off;
                            assert((m_pos_sav = z_pos) != NULL);
                        }
#else
                        /* try to find a closer match */
                        if (m_len < M2_MAX_LEN + 1 || x_off < m_off)
                        {
                            m_len = M2_MAX_LEN + 1;
                            m_off = x_off;
                            assert((m_pos_sav = z_pos) != NULL);
                        }
#endif
#else
                        /* don't search for a longer/closer match */
                        m_len = M2_MAX_LEN + 1;
                        m_off = x_off;
                        assert((m_pos_sav = z_pos) != NULL);
                        ip = ip_sav;
                        d -= DD_SIZE - j;
                        assert(d == &dict [ DINDEX(dv,ip) ]);
                        UPDATE_P(d,drun,ip,in);
                        goto match;
#endif
                    }
                    ip = ip_sav;
                }
                else
                    ip = ip_sav;
                d++;
            } while (--j > 0);
            assert(ip == ip_sav);

            d -= DD_SIZE;
            assert(d == &dict [ DINDEX(dv,ip) ]);
            UPDATE_P(d,drun,ip,in);
        }

#endif /* (DD_BITS == 0) */


/* vim:set ts=4 sw=4 et: */
