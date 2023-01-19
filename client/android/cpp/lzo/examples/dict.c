/* dict.c -- example program: how to use preset dictionaries

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
// This program shows how to use preset dictionaries.
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


#define DICT_LEN    0xbfff
static lzo_bytep    dict;
static lzo_uint     dict_len = 0;
static lzo_uint32_t dict_adler32;


/*************************************************************************
//
**************************************************************************/

static lzo_uint total_n = 0;
static lzo_uint total_c_len = 0;
static lzo_uint total_d_len = 0;

static void print_info(const char *name, lzo_uint d_len, lzo_uint c_len)
{
    double perc;

    perc = (d_len > 0) ? c_len * 100.0 / d_len : 0.0;
    printf("  |  %-30s   %9ld -> %9ld   %7.2f%%  |\n",
            name, (long) d_len, (long) c_len, perc);

    total_n++;
    total_c_len += c_len;
    total_d_len += d_len;
}


/*************************************************************************
//
**************************************************************************/

static int do_file(const char *in_name, int compression_level)
{
    int r;
    lzo_bytep in;
    lzo_bytep out;
    lzo_bytep newb;
    lzo_voidp wrkmem;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint new_len;
    long l;
    FILE *fp;

/*
 * Step 1: open the input file
 */
    fp = fopen(in_name,"rb");
    if (fp == NULL)
    {
        printf("%s: %s: cannot open file\n", progname, in_name);
        return 0;   /* no error */
    }
    fseek(fp, 0, SEEK_END);
    l = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (l <= 0)
    {
        printf("%s: %s: empty file -- skipping\n", progname, in_name);
        fclose(fp); fp = NULL;
        return 0;   /* no error */
    }
    in_len = (lzo_uint) l;
    if ((long) in_len != l || l > 256L * 1024L * 1024L)
    {
        printf("%s: %s: file is too big -- skipping\n", progname, in_name);
        fclose(fp); fp = NULL;
        return 0;   /* no error */
    }

/*
 * Step 2: allocate compression buffers and read the file
 */
    in = (lzo_bytep) xmalloc(in_len);
    out = (lzo_bytep) xmalloc(in_len + in_len / 16 + 64 + 3);
    newb = (lzo_bytep) xmalloc(in_len);
    wrkmem = (lzo_voidp) xmalloc(LZO1X_999_MEM_COMPRESS);
    if (in == NULL || out == NULL || newb == NULL || wrkmem == NULL)
    {
        printf("%s: out of memory\n", progname);
        exit(1);
    }
    in_len = (lzo_uint) lzo_fread(fp, in, in_len);
    fclose(fp); fp = NULL;

/*
 * Step 3: compress from 'in' to 'out' with LZO1X-999
 */
    r = lzo1x_999_compress_level(in, in_len, out, &out_len, wrkmem,
                                 dict, dict_len, 0, compression_level);
    if (r != LZO_E_OK)
    {
        /* this should NEVER happen */
        printf("internal error - compression failed: %d\n", r);
        return 1;
    }

    print_info(in_name, in_len, out_len);

/*
 * Step 4: decompress again, now going from 'out' to 'newb'
 */
    new_len = in_len;
    r = lzo1x_decompress_dict_safe(out, out_len, newb, &new_len, NULL, dict, dict_len);
    if (r != LZO_E_OK)
    {
        /* this should NEVER happen */
        printf("internal error - decompression failed: %d\n", r);
        return 1;
    }

/*
 * Step 5: verify decompression
 */
    if (new_len != in_len || lzo_memcmp(in, newb, in_len) != 0)
    {
        /* this should NEVER happen */
        printf("internal error - decompression data error\n");
        return 1;
    }

    /* free buffers in reverse order to help malloc() */
    lzo_free(wrkmem);
    lzo_free(newb);
    lzo_free(out);
    lzo_free(in);
    return 0;
}


/*************************************************************************
//
**************************************************************************/

int __lzo_cdecl_main main(int argc, char *argv[])
{
    int i = 1;
    int r = 0;
    const char *dict_name;
    FILE *fp;
    time_t t_total;
    int compression_level = 7;

    lzo_wildargv(&argc, &argv);

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");

    progname = argv[0];

    if (i < argc && argv[i][0] == '-' && isdigit(argv[i][1]))
        compression_level = atoi(&argv[i++][1]);

    if (i + 1 >= argc || compression_level < 1 || compression_level > 9)
    {
        printf("usage: %s [-level] [ dictionary-file | -n ]  file...\n", progname);
        exit(1);
    }
    printf("Compression level is LZO1X-999/%d\n", compression_level);

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
 * Step 2: prepare the dictionary
 */
    dict = (lzo_bytep) xmalloc(DICT_LEN);
    if (dict == NULL)
    {
        printf("%s: out of memory\n", progname);
        exit(1);
    }
    dict_name = argv[i++];
    if (strcmp(dict_name,"-n") == 0)
    {
        dict_name = "empty";
        dict_len = 0;
    }
    else
    {
        fp = fopen(dict_name,"rb");
        if (fp == NULL)
        {
            printf("%s: cannot open dictionary file %s\n", progname, dict_name);
            exit(1);
        }
        dict_len = (lzo_uint) lzo_fread(fp, dict, DICT_LEN);
        fclose(fp); fp = NULL;
    }

    dict_adler32 = lzo_adler32(0, NULL, 0);
    dict_adler32 = lzo_adler32(dict_adler32, dict, dict_len);
    printf("Using dictionary '%s', %ld bytes, ID 0x%08lx.\n",
           dict_name, (long) dict_len, (unsigned long) dict_adler32);

/*
 * Step 3: process files
 */
    t_total = time(NULL);
    for ( ; i < argc; i++) {
        if (do_file(argv[i], compression_level) != 0) {
            r = 1;
            break;
        }
    }
    t_total = time(NULL) - t_total;

    lzo_free(dict);

    if (total_n > 1)
        print_info("***TOTALS***", total_d_len, total_c_len);

    printf("Dictionary compression test %s, execution time %lu seconds.\n",
           r == 0 ? "passed" : "FAILED", (unsigned long) t_total);
    return r;
}


/* vim:set ts=4 sw=4 et: */
