/* lzo1_d.ch -- common decompression stuff

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



#if defined(LZO_TEST_OVERRUN)
#  if !defined(LZO_TEST_OVERRUN_INPUT)
#    define LZO_TEST_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_OVERRUN_OUTPUT)
#    define LZO_TEST_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_OVERRUN_LOOKBEHIND  1
#  endif
#endif


/***********************************************************************
// Overrun detection is internally handled by these macros:
//
//   TEST_IP    test input overrun at loop begin
//   NEED_IP    test input overrun at every input byte
//
//   TEST_OP    test output overrun at loop begin
//   NEED_OP    test output overrun at every output byte
//
//   TEST_LB    test match position
//
// The fastest decompressor results when testing for no overruns
// and using LZO_EOF_CODE.
************************************************************************/

#undef TEST_IP
#undef TEST_OP
#undef TEST_IP_AND_TEST_OP
#undef TEST_LB
#undef TEST_LBO
#undef NEED_IP
#undef NEED_OP
#undef TEST_IV
#undef TEST_OV
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP


#if defined(LZO_TEST_OVERRUN_INPUT)
#  if (LZO_TEST_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
            if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#    define TEST_IV(x)          if ((x) >  (lzo_uint)0 - (511)) goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_OUTPUT)
#  if (LZO_TEST_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP              /* don't need both of the tests here */
#    define NEED_OP(x) \
            if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#    define TEST_OV(x)          if ((x) >  (lzo_uint)0 - (511)) goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_OVERRUN_LOOKBEHIND)
#  define TEST_LB(m_pos)        if (PTR_LT(m_pos,out) || PTR_GE(m_pos,op)) goto lookbehind_overrun
#  define TEST_LBO(m_pos,o)     if (PTR_LT(m_pos,out) || PTR_GE(m_pos,op-(o))) goto lookbehind_overrun
#else
#  define TEST_LB(m_pos)        ((void) 0)
#  define TEST_LBO(m_pos,o)     ((void) 0)
#endif


#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
   /* if we have no EOF code, we have to test for the end of the input */
#  define TEST_IP               (ip < ip_end)
#endif


#if defined(TEST_IP)
#  define HAVE_TEST_IP 1
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP 1
#else
#  define TEST_OP               1
#endif

#if defined(HAVE_TEST_IP) && defined(HAVE_TEST_OP)
#  define TEST_IP_AND_TEST_OP   (TEST_IP && TEST_OP)
#elif defined(HAVE_TEST_IP)
#  define TEST_IP_AND_TEST_OP   TEST_IP
#elif defined(HAVE_TEST_OP)
#  define TEST_IP_AND_TEST_OP   TEST_OP
#else
#  define TEST_IP_AND_TEST_OP   1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP 1
#else
#  define NEED_IP(x)            ((void) 0)
#  define TEST_IV(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP 1
#else
#  define NEED_OP(x)            ((void) 0)
#  define TEST_OV(x)            ((void) 0)
#endif


#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP 1
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP 1
#endif


/* vim:set ts=4 sw=4 et: */
