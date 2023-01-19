/* portab.h -- portability layer

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


#include <lzo/lzoconf.h>

#if (LZO_CC_MSC && (_MSC_VER >= 1000 && _MSC_VER < 1200))
   /* avoid '-W4' warnings in system header files */
#  pragma warning(disable: 4201 4214 4514)
#endif
#if (LZO_CC_MSC && (_MSC_VER >= 1300))
   /* avoid '-Wall' warnings in system header files */
#  pragma warning(disable: 4163 4255 4820)
   /* avoid warnings about inlining */
#  pragma warning(disable: 4710 4711)
#endif
/* disable silly warnings about using "deprecated" POSIX functions like "fopen" */
#if (LZO_CC_CLANG_MSC && LZO_CC_CLANG >= 0x030500)
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif (LZO_CC_INTELC_MSC && (__INTEL_COMPILER >= 1100))
#  pragma warning(disable: 1786)
#elif (LZO_CC_INTELC_MSC && (__INTEL_COMPILER >= 1000))
#  pragma warning(disable: 1478)
#elif (LZO_CC_MSC && (_MSC_VER >= 1400))
#  pragma warning(disable: 4996)
#endif
#if (LZO_CC_PELLESC && (__POCC__ >= 290))
#  pragma warn(disable:2002)
#endif


/*************************************************************************
//
**************************************************************************/

#if defined(LZO_WANT_ACCLIB_GETOPT) || !(defined(LZO_LIBC_ISOC90) || defined(LZO_LIBC_ISOC99))

#include "examples/portab_a.h"

#else

/* On any halfway modern machine we can use the following pure ANSI-C code. */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#if defined(CLK_TCK) && !defined(CLOCKS_PER_SEC)
#  define CLOCKS_PER_SEC CLK_TCK
#endif

#if defined(WANT_LZO_MALLOC)
#  define lzo_malloc(a)         (malloc(a))
#  define lzo_free(a)           (free(a))
#endif
#if defined(WANT_LZO_FREAD)
#  define lzo_fread(f,b,s)      (fread(b,1,s,f))
#  define lzo_fwrite(f,b,s)     (fwrite(b,1,s,f))
#endif
#if defined(WANT_LZO_PCLOCK)
#  define lzo_pclock_handle_t   int
#  define lzo_pclock_t          double
#  define lzo_pclock_open_default(a) ((void)(a))
#  define lzo_pclock_close(a)   ((void)(a))
#  define lzo_pclock_read(a,b)  *(b) = (clock() / (double)(CLOCKS_PER_SEC))
#  define lzo_pclock_get_elapsed(a,b,c) (*(c) - *(b))
#  define lzo_pclock_flush_cpu_cache(a,b)  ((void)(a))
#endif
#if defined(WANT_LZO_WILDARGV)
#  define lzo_wildargv(a,b)     ((void)0)
#endif

#endif


/*************************************************************************
// misc
**************************************************************************/

/* turn on assertions */
#undef NDEBUG
#include <assert.h>

/* just in case */
#undef xmalloc
#undef xfree
#undef xread
#undef xwrite
#undef xputc
#undef xgetc
#undef xread32
#undef xwrite32


#if defined(WANT_XMALLOC)
static lzo_voidp xmalloc(lzo_uint len)
{
    lzo_voidp p;
    lzo_uint align = (lzo_uint) sizeof(lzo_align_t);

    p = (lzo_voidp) lzo_malloc(len > 0 ? len : 1);
    if (p == NULL)
    {
        printf("%s: out of memory\n", progname);
        exit(1);
    }
    if (len >= align && __lzo_align_gap(p, align) != 0)
    {
        printf("%s: C library problem: malloc() returned misaligned pointer!\n", progname);
        lzo_free(p);
        exit(1);
    }
    return p;
}
#endif


/* vim:set ts=4 sw=4 et: */
