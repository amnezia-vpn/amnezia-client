/* lzo1b_tm.ch -- implementation of the LZO1B compression algorithm

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
// test for a potential match
************************************************************************/


#if (DD_BITS == 0)

try_match:
#if !defined(NDEBUG) && (LZO_DICT_USE_PTR)
#if (LZO_DETERMINISTIC)
        assert(m_pos == NULL || m_pos >= in);
        assert(m_pos == NULL || m_pos < ip);
#endif
        m_pos_sav = m_pos;
#endif
        if (m_pos[0] == ip[0] && m_pos[1] == ip[1] && m_pos[2] == ip[2])
        {
            m_pos += 3;
            goto match;
        }


#else /* (DD_BITS == 0) */


    /* test potential match */

        if (m_len > M2_MIN_LEN)
            goto match;
        if (m_len == M2_MIN_LEN)
        {
#if (_MAX_OFFSET == _M2_MAX_OFFSET)
            goto match;
#else
            if (m_off <= M2_MAX_OFFSET)
                goto match;
#if 0 && (M3_MIN_LEN == M2_MIN_LEN)
            if (ip == ii)
                goto match;
#endif
#endif
        }
        goto literal;


#endif /* (DD_BITS == 0) */


/* vim:set ts=4 sw=4 et: */
