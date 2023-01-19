/* align.c -- test alignment (important for 16-bit systems)

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


#if 0
#include "src/lzo_conf.h"
#include "src/lzo_ptr.h"
#endif
#include <lzo/lzoconf.h>

/* utility layer */
#define WANT_LZO_MALLOC 1
#include "examples/portab.h"


static int opt_verbose = 0;


/*************************************************************************
//
**************************************************************************/

static unsigned long align_test(lzo_bytep block, lzo_uint len, lzo_uint step)
{
    lzo_bytep b1 = block;
    lzo_bytep b2 = block;
    lzo_bytep k1 = NULL;
    lzo_bytep k2 = NULL;
    lzo_bytep k;
    lzo_bytep x;
    lzo_uint offset = 0;
    unsigned long i = 0;

    assert(step > 0);
    assert(step <= 65536ul);
    assert((step & (step - 1)) == 0);

    for (offset = step; offset < len; offset += step)
    {
        k1 = LZO_PTR_ALIGN_UP(b1 + 1, step);
        k2 = b2 + offset;
        if (k1 != k2)
        {
            printf("error 1: i %lu step %ld offset %ld: "
                   "%p (%ld) %p (%ld)\n",
                   i, (long) step, (long) offset,
                   k1, (long) (k1 - block),
                   k2, (long) (k2 - block));
            return 0;
        }
        if (k1 - step != b1)
        {
            printf("error 2: i %lu step %ld offset %ld: "
                   "%p (%ld) %p (%ld)\n",
                   i, (long) step, (long) offset,
                   b1, (long) (b1 - block),
                   k1, (long) (k1 - block));
            return 0;
        }

        assert(k1 > b1);
        assert(k2 > b2);
        assert((lzo_uint)(k2 - b2) == offset);
        assert(k1 - offset == b2);
#if defined(PTR_ALIGNED_4)
        if (step == 4)
        {
            assert(PTR_ALIGNED_4(k1));
            assert(PTR_ALIGNED_4(k2));
            assert(PTR_ALIGNED2_4(k1,k2));
        }
#endif
#if defined(PTR_ALIGNED_8)
        if (step == 8)
        {
            assert(PTR_ALIGNED_8(k1));
            assert(PTR_ALIGNED_8(k2));
            assert(PTR_ALIGNED2_8(k1,k2));
        }
#endif
#if defined(PTR_LINEAR)
        assert((PTR_LINEAR(k1) & (step-1)) == 0);
        assert((PTR_LINEAR(k2) & (step-1)) == 0);
#endif

        for (k = b1 + 1; k <= k1; k++)
        {
            x = LZO_PTR_ALIGN_UP(k, step);
            if (x != k1)
            {
                printf("error 3: base: %p %p %p  i %lu step %ld offset %ld: "
                       "%p (%ld) %p (%ld) %p (%ld)\n",
                       block, b1, b2,
                       i, (long) step, (long) offset,
                       k1, (long) (k1 - block),
                       k, (long) (k - block),
                       x, (long) (x - block));
                return 0;
            }
        }

        b1 = k1;
        i++;
    }

    return i;
}


/*************************************************************************
//
**************************************************************************/

#define BLOCK_SIZE  (128*1024ul)

int main(int argc, char *argv[])
{
    lzo_bytep buf;
    lzo_uint step;

    if (argc >= 2 && strcmp(argv[1],"-v") == 0)
        opt_verbose = 1;

    if (lzo_init() != LZO_E_OK)
    {
        printf("lzo_init() failed !!!\n");
        return 3;
    }
    buf = (lzo_bytep) lzo_malloc(2*BLOCK_SIZE + 256);
    if (buf == NULL)
    {
        printf("out of memory\n");
        return 2;
    }

#if defined(lzo_uintptr_t)
    printf("Align init: %p ( 0x%lx )\n", buf, (unsigned long) (lzo_uintptr_t) buf);
#elif defined(__LZO_MMODEL_HUGE)
    printf("Align init: %p ( 0x%lx )\n", buf, (unsigned long) buf);
#else
    printf("Align init: %p ( 0x%lx )\n", buf, (unsigned long) (size_t) buf);
#endif

    for (step = 1; step <= 65536ul; step *= 2)
    {
        lzo_bytep block = buf;
        unsigned long n;
        unsigned gap;

        gap = __lzo_align_gap(block, step);
        block = LZO_PTR_ALIGN_UP(block, step);
        if (opt_verbose >= 1)
            printf("STEP %5lu: GAP: %5lu  %p %p %5lu\n",
                   (unsigned long) step, (unsigned long) gap, buf, block,
                   (unsigned long) (block - buf));
        n = align_test(block, BLOCK_SIZE, step);
        if (n == 0)
            return 1;
        if ((n + 1) * step != BLOCK_SIZE)
        {
            printf("error 4: %ld %lu\n", (long)step, n);
            return 1;
        }
    }

    lzo_free(buf);
    printf("Alignment test passed.\n");
    return 0;
}


/* vim:set ts=4 sw=4 et: */
