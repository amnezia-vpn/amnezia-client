/* overlap.c -- example program: overlapping (de)compression

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


/*************************************************************************
// This program shows how to do overlapping compression and
// in-place decompression.
//
// Please study LZO.FAQ and simple.c first.
**************************************************************************/

#include <lzo/lzoconf.h>
#include <lzo/lzo1x.h>

/* portability layer */
static const char *progname = NULL;
#define WANT_LZO_MALLOC 1
#define WANT_LZO_FREAD 1
#define WANT_LZO_WILDARGV 1
#define WANT_XMALLOC 1
#include "examples/portab.h"


/* Overhead (in bytes) for the in-place decompression buffer.
 * Most files need only 16 !
 * (try 'overlap -16 file' or even 'overlap -8 file')
 *
 * Worst case (for files that are compressible by only a few bytes)
 * is 'in_len / 16 + 64 + 3'. See step 5a) below.
 *
 * For overlapping compression '0xbfff + in_len / 16 + 64 + 3' bytes
 * will be needed. See step 4a) below.
 */

static long opt_overhead = 0;   /* assume worst case */

static unsigned long total_files = 0;
static unsigned long total_in = 0;


/*************************************************************************
//
**************************************************************************/

static int do_file(const char *in_name)
{
    int r;
    FILE *fp = NULL;
    long l;

    lzo_voidp wrkmem = NULL;

    lzo_bytep in = NULL;
    lzo_uint in_len;                /* uncompressed length */

    lzo_bytep out = NULL;
    lzo_uint out_len;               /* compressed length */

    lzo_bytep overlap = NULL;
    lzo_uint overhead;
    lzo_uint offset;

    lzo_uint new_len = 0;

/*
 * Step 1: open the input file
 */
    fp = fopen(in_name, "rb");
    if (fp == NULL)
    {
        printf("%s: %s: cannot open file\n", progname, in_name);
        goto next_file;
    }
    fseek(fp, 0, SEEK_END);
    l = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (l <= 0)
    {
        printf("%s: %s: empty file -- skipping\n", progname, in_name);
        goto next_file;
    }
    in_len = (lzo_uint) l;
    if ((long) in_len != l || l > 256L * 1024L * 1024L)
    {
        printf("%s: %s: file is too big -- skipping\n", progname, in_name);
        goto next_file;
    }

/*
 * Step 2: allocate compression buffers and read the file
 */
    in = (lzo_bytep) xmalloc(in_len);
    out = (lzo_bytep) xmalloc(in_len + in_len / 16 + 64 + 3);
    wrkmem = (lzo_voidp) xmalloc(LZO1X_1_MEM_COMPRESS);
    in_len = (lzo_uint) lzo_fread(fp, in, in_len);
    fclose(fp); fp = NULL;
    printf("%s: %s: read %lu bytes\n", progname, in_name, (unsigned long) in_len);

    total_files++;
    total_in += (unsigned long) in_len;

/*
 * Step 3: compress from 'in' to 'out' with LZO1X-1
 */
    r = lzo1x_1_compress(in, in_len, out, &out_len, wrkmem);
    if (r != LZO_E_OK || out_len > in_len + in_len / 16 + 64 + 3)
    {
        /* this should NEVER happen */
        printf("internal error - compression failed: %d\n", r);
        exit(1);
    }
    printf("%-25s %8lu -> %8lu\n", "LZO1X-1:", (unsigned long) in_len, (unsigned long) out_len);


/***** Step 4: overlapping compression *****/

/*
 * Step 4a: allocate the 'overlap' buffer for overlapping compression
 */
    overhead  = in_len > 0xbfff ? 0xbfff : in_len;
    overhead += in_len / 16 + 64 + 3;
    overlap = (lzo_bytep) xmalloc(in_len + overhead);

/*
 * Step 4b: prepare data in 'overlap' buffer.
 *          copy uncompressed data at the top of the overlap buffer
 */
    /*** offset = in_len + overhead - in_len; ***/
    offset = overhead;
    lzo_memcpy(overlap + offset, in, in_len);

/*
 * Step 4c: do an in-place compression within the 'overlap' buffer
 */
    r = lzo1x_1_compress(overlap + offset, in_len, overlap, &new_len, wrkmem);
    if (r != LZO_E_OK)
    {
        /* this should NEVER happen */
        printf("in-place compression failed: %d\n", r);
        exit(1);
    }

/*
 * Step 4d: verify overlapping compression
 */
    if (new_len != out_len || lzo_memcmp(out, overlap, out_len) != 0)
    {
        /* As compression is non-deterministic there can be a difference
         * in the representation of the compressed data (but this usually
         * happens very seldom). So we have to verify the overlapping
         * compression by doing a temporary decompression.
         */
        lzo_uint ll = in_len;
        lzo_bytep tmp = (lzo_bytep) xmalloc(ll);
        r = lzo1x_decompress_safe(overlap, new_len, tmp, &ll, NULL);
        if (r != LZO_E_OK || ll != in_len || lzo_memcmp(in, tmp, ll) != 0)
        {
            /* this should NEVER happen */
            printf("in-place compression data error\n");
            exit(1);
        }
        lzo_free(tmp);
    }

    printf("  in-place compression:   %8lu -> %8lu    overhead: %7lu\n",
            (unsigned long) in_len, (unsigned long) new_len, (unsigned long) overhead);
    lzo_free(overlap); overlap = NULL;


/***** Step 5: in-place decompression *****/

/*
 * Step 5a: allocate the 'overlap' buffer for in-place decompression
 */
    if (opt_overhead == 0 || out_len >= in_len)
        overhead = in_len / 16 + 64 + 3;
    else
        overhead = (lzo_uint) opt_overhead;
    overlap = (lzo_bytep) xmalloc(in_len + overhead);

/*
 * Step 5b: prepare data in 'overlap' buffer.
 *          copy compressed data at the top of the overlap buffer
 */
    offset = in_len + overhead - out_len;
    lzo_memcpy(overlap + offset, out, out_len);

/*
 * Step 5c: do an in-place decompression within the 'overlap' buffer
 */
    new_len = in_len;
    r = lzo1x_decompress_safe(overlap + offset, out_len, overlap, &new_len, NULL);
    if (r != LZO_E_OK)
    {
        /* this may happen if overhead is too small */
        printf("in-place decompression failed: %d - increase 'opt_overhead'\n", r);
        exit(1);
    }

/*
 * Step 5d: verify decompression
 */
    if (new_len != in_len || lzo_memcmp(in, overlap, in_len) != 0)
    {
        /* this may happen if overhead is too small */
        printf("in-place decompression data error - increase 'opt_overhead'\n");
        exit(1);
    }
    printf("  in-place decompression: %8lu -> %8lu    overhead: %7lu\n",
            (unsigned long) out_len, (unsigned long) new_len, (unsigned long) overhead);
    lzo_free(overlap); overlap = NULL;


next_file:
    lzo_free(overlap);
    lzo_free(wrkmem);
    lzo_free(out);
    lzo_free(in);
    if (fp) fclose(fp);

    return 0;
}


/*************************************************************************
//
**************************************************************************/

int __lzo_cdecl_main main(int argc, char *argv[])
{
    int r;
    int i = 1;

    lzo_wildargv(&argc, &argv);

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");

    progname = argv[0];
    if (i < argc && argv[i][0] == '-')
        opt_overhead = atol(&argv[i++][1]);
#if 1
    if (opt_overhead != 0 && opt_overhead < 4)
    {
        printf("%s: invalid overhead value %ld\n", progname, opt_overhead);
        exit(1);
    }
#endif
    if (i >= argc)
    {
        printf("usage: %s [-overhead_in_bytes] file..\n", progname);
        exit(1);
    }

/*
 * Step 1: initialize the LZO library
 */
    if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable '-DLZO_DEBUG' for diagnostics)\n");
        exit(1);
    }

/*
 * Step 2: process files
 */
    for (r = 0; r == 0 && i < argc; i++)
        r = do_file(argv[i]);

    printf("\nDone. Successfully processed %lu bytes in %lu files.\n",
            total_in, total_files);
    return r;
}


/* vim:set ts=4 sw=4 et: */
