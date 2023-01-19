/* lzopack.c -- LZO example program: a simple file packer

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
// NOTE: this is an example program, so do not use to backup your data.
//
// This program lacks things like sophisticated file handling but is
// pretty complete regarding compression - it should provide a good
// starting point for adaption for your applications.
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


static unsigned long total_in = 0;
static unsigned long total_out = 0;
static lzo_bool opt_debug = 0;

/* magic file header for lzopack-compressed files */
static const unsigned char magic[7] =
    { 0x00, 0xe9, 0x4c, 0x5a, 0x4f, 0xff, 0x1a };


/*************************************************************************
// file IO
**************************************************************************/

static lzo_uint xread(FILE *fp, lzo_voidp buf, lzo_uint len, lzo_bool allow_eof)
{
    lzo_uint l;

    l = (lzo_uint) lzo_fread(fp, buf, len);
    if (l > len)
    {
        fprintf(stderr, "\n%s: internal error - something is wrong with your C library !!!\n", progname);
        exit(1);
    }
    if (l != len && !allow_eof)
    {
        fprintf(stderr, "\n%s: read error - premature end of file\n", progname);
        exit(1);
    }
    total_in += (unsigned long) l;
    return l;
}

static lzo_uint xwrite(FILE *fp, const lzo_voidp buf, lzo_uint len)
{
    if (fp != NULL && lzo_fwrite(fp, buf, len) != len)
    {
        fprintf(stderr, "\n%s: write error  (disk full ?)\n", progname);
        exit(1);
    }
    total_out += (unsigned long) len;
    return len;
}


static int xgetc(FILE *fp)
{
    unsigned char c;
    xread(fp, (lzo_voidp) &c, 1, 0);
    return c;
}

static void xputc(FILE *fp, int c)
{
    unsigned char cc = (unsigned char) (c & 0xff);
    xwrite(fp, (const lzo_voidp) &cc, 1);
}

/* read and write portable 32-bit integers */

static lzo_uint32_t xread32(FILE *fp)
{
    unsigned char b[4];
    lzo_uint32_t v;

    xread(fp, b, 4, 0);
    v  = (lzo_uint32_t) b[3] <<  0;
    v |= (lzo_uint32_t) b[2] <<  8;
    v |= (lzo_uint32_t) b[1] << 16;
    v |= (lzo_uint32_t) b[0] << 24;
    return v;
}

static void xwrite32(FILE *fp, lzo_uint v)
{
    unsigned char b[4];

    b[3] = (unsigned char) ((v >>  0) & 0xff);
    b[2] = (unsigned char) ((v >>  8) & 0xff);
    b[1] = (unsigned char) ((v >> 16) & 0xff);
    b[0] = (unsigned char) ((v >> 24) & 0xff);
    xwrite(fp, b, 4);
}


/*************************************************************************
// compress
//
// possible improvement: we could use overlapping compression to
//   save some memory - see overlap.c. This would require some minor
//   changes in the decompression code as well, because if a block
//   turns out to be incompressible we would still have to store it in its
//   "compressed" (i.e. then slightly enlarged) form because the original
//   (uncompressed) data would have been lost during the overlapping
//   compression.
**************************************************************************/

static int do_compress(FILE *fi, FILE *fo, int compression_level, lzo_uint block_size)
{
    int r = 0;
    lzo_bytep in = NULL;
    lzo_bytep out = NULL;
    lzo_voidp wrkmem = NULL;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint wrkmem_size;
    lzo_uint32_t flags = 1;     /* do compute a checksum */
    int method = 1;             /* compression method: LZO1X */
    lzo_uint32_t checksum;

    total_in = total_out = 0;

/*
 * Step 1: write magic header, flags & block size, init checksum
 */
    xwrite(fo, magic, sizeof(magic));
    xwrite32(fo, flags);
    xputc(fo, method);              /* compression method */
    xputc(fo, compression_level);   /* compression level */
    xwrite32(fo, block_size);
    checksum = lzo_adler32(0, NULL, 0);

/*
 * Step 2: allocate compression buffers and work-memory
 */
    in = (lzo_bytep) xmalloc(block_size);
    out = (lzo_bytep) xmalloc(block_size + block_size / 16 + 64 + 3);
    if (compression_level == 9)
        wrkmem_size = LZO1X_999_MEM_COMPRESS;
    else
        wrkmem_size = LZO1X_1_MEM_COMPRESS;
    wrkmem = (lzo_voidp) xmalloc(wrkmem_size);
    if (in == NULL || out == NULL || wrkmem == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 1;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        /* read block */
        in_len = xread(fi, in, block_size, 1);
        if (in_len == 0)
            break;

        /* update checksum */
        if (flags & 1)
            checksum = lzo_adler32(checksum, in, in_len);

        /* clear wrkmem (not needed, only for debug/benchmark purposes) */
        if (opt_debug)
            lzo_memset(wrkmem, 0xff, wrkmem_size);

        /* compress block */
        if (compression_level == 9)
            r = lzo1x_999_compress(in, in_len, out, &out_len, wrkmem);
        else
            r = lzo1x_1_compress(in, in_len, out, &out_len, wrkmem);
        if (r != LZO_E_OK || out_len > in_len + in_len / 16 + 64 + 3)
        {
            /* this should NEVER happen */
            printf("internal error - compression failed: %d\n", r);
            r = 2;
            goto err;
        }

        /* write uncompressed block size */
        xwrite32(fo, in_len);

        if (out_len < in_len)
        {
            /* write compressed block */
            xwrite32(fo, out_len);
            xwrite(fo, out, out_len);
        }
        else
        {
            /* not compressible - write uncompressed block */
            xwrite32(fo, in_len);
            xwrite(fo, in, in_len);
        }
    }

    /* write EOF marker */
    xwrite32(fo, 0);

    /* write checksum */
    if (flags & 1)
        xwrite32(fo, checksum);

    r = 0;
err:
    lzo_free(wrkmem);
    lzo_free(out);
    lzo_free(in);
    return r;
}


/*************************************************************************
// decompress / test
//
// We are using overlapping (in-place) decompression to save some
// memory - see overlap.c.
**************************************************************************/

static int do_decompress(FILE *fi, FILE *fo)
{
    int r = 0;
    lzo_bytep buf = NULL;
    lzo_uint buf_len;
    unsigned char m [ sizeof(magic) ];
    lzo_uint32_t flags;
    int method;
    int compression_level;
    lzo_uint block_size;
    lzo_uint32_t checksum;

    total_in = total_out = 0;

/*
 * Step 1: check magic header, read flags & block size, init checksum
 */
    if (xread(fi, m, sizeof(magic), 1) != sizeof(magic) ||
        memcmp(m, magic, sizeof(magic)) != 0)
    {
        printf("%s: header error - this file is not compressed by lzopack\n", progname);
        r = 1;
        goto err;
    }
    flags = xread32(fi);
    method = xgetc(fi);
    compression_level = xgetc(fi);
    if (method != 1)
    {
        printf("%s: header error - invalid method %d (level %d)\n",
                progname, method, compression_level);
        r = 2;
        goto err;
    }
    block_size = xread32(fi);
    if (block_size < 1024 || block_size > 8L * 1024L * 1024L)
    {
        printf("%s: header error - invalid block size %ld\n",
                progname, (long) block_size);
        r = 3;
        goto err;
    }
    checksum = lzo_adler32(0, NULL, 0);

/*
 * Step 2: allocate buffer for in-place decompression
 */
    buf_len = block_size + block_size / 16 + 64 + 3;
    buf = (lzo_bytep) xmalloc(buf_len);
    if (buf == NULL)
    {
        printf("%s: out of memory\n", progname);
        r = 4;
        goto err;
    }

/*
 * Step 3: process blocks
 */
    for (;;)
    {
        lzo_bytep in;
        lzo_bytep out;
        lzo_uint in_len;
        lzo_uint out_len;

        /* read uncompressed size */
        out_len = xread32(fi);

        /* exit if last block (EOF marker) */
        if (out_len == 0)
            break;

        /* read compressed size */
        in_len = xread32(fi);

        /* sanity check of the size values */
        if (in_len > block_size || out_len > block_size ||
            in_len == 0 || in_len > out_len)
        {
            printf("%s: block size error - data corrupted\n", progname);
            r = 5;
            goto err;
        }

        /* place compressed block at the top of the buffer */
        in = buf + buf_len - in_len;
        out = buf;

        /* read compressed block data */
        xread(fi, in, in_len, 0);

        if (in_len < out_len)
        {
            /* decompress - use safe decompressor as data might be corrupted
             * during a file transfer */
            lzo_uint new_len = out_len;

            r = lzo1x_decompress_safe(in, in_len, out, &new_len, NULL);
            if (r != LZO_E_OK || new_len != out_len)
            {
                printf("%s: compressed data violation\n", progname);
                r = 6;
                goto err;
            }
            /* write decompressed block */
            xwrite(fo, out, out_len);
            /* update checksum */
            if (flags & 1)
                checksum = lzo_adler32(checksum, out, out_len);
        }
        else
        {
            /* write original (incompressible) block */
            xwrite(fo, in, in_len);
            /* update checksum */
            if (flags & 1)
                checksum = lzo_adler32(checksum, in, in_len);
        }
    }

    /* read and verify checksum */
    if (flags & 1)
    {
        lzo_uint32_t c = xread32(fi);
        if (c != checksum)
        {
            printf("%s: checksum error - data corrupted\n", progname);
            r = 7;
            goto err;
        }
    }

    r = 0;
err:
    lzo_free(buf);
    return r;
}


/*************************************************************************
//
**************************************************************************/

static void usage(void)
{
    printf("usage: %s [-9] input-file output-file  (compress)\n", progname);
    printf("usage: %s -d   input-file output-file  (decompress)\n", progname);
    printf("usage: %s -t   input-file...           (test)\n", progname);
    exit(1);
}


/* open input file */
static FILE *xopen_fi(const char *name)
{
    FILE *fp;

    fp = fopen(name, "rb");
    if (fp == NULL)
    {
        printf("%s: cannot open input file %s\n", progname, name);
        exit(1);
    }
#if defined(HAVE_STAT) && defined(S_ISREG)
    {
        struct stat st;
        int is_regular = 1;
        if (stat(name, &st) != 0 || !S_ISREG(st.st_mode))
            is_regular = 0;
        if (!is_regular)
        {
            printf("%s: %s is not a regular file\n", progname, name);
            fclose(fp); fp = NULL;
            exit(1);
        }
    }
#endif
    return fp;
}


/* open output file */
static FILE *xopen_fo(const char *name)
{
    FILE *fp;

#if 0
    /* this is an example program, so make sure we don't overwrite a file */
    fp = fopen(name, "rb");
    if (fp != NULL)
    {
        printf("%s: file %s already exists -- not overwritten\n", progname, name);
        fclose(fp); fp = NULL;
        exit(1);
    }
#endif
    fp = fopen(name, "wb");
    if (fp == NULL)
    {
        printf("%s: cannot open output file %s\n", progname, name);
        exit(1);
    }
    return fp;
}


/* close file */
static void xclose(FILE *fp)
{
    if (fp)
    {
        int err;
        err = ferror(fp);
        if (fclose(fp) != 0)
            err = 1;
        if (err)
        {
            printf("%s: error while closing file\n", progname);
            exit(1);
        }
    }
}


/*************************************************************************
//
**************************************************************************/

int __lzo_cdecl_main main(int argc, char *argv[])
{
    int i = 1;
    int r = 0;
    FILE *fi = NULL;
    FILE *fo = NULL;
    const char *in_name = NULL;
    const char *out_name = NULL;
    unsigned opt_decompress = 0;
    unsigned opt_test = 0;
    int opt_compression_level = 1;
    lzo_uint opt_block_size;
    const char *s;

    lzo_wildargv(&argc, &argv);

    progname = argv[0];
    for (s = progname; *s; s++)
        if ((*s == '/' || *s == '\\') && s[1])
            progname = s + 1;

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");

#if 0
    printf(
"*** DISCLAIMER ***\n"
"   This is an example program, do not use to backup your data !\n"
"   Get LZOP if you're interested into a full-featured packer.\n"
"   See http://www.oberhumer.com/opensource/lzop/\n"
"\n");
#endif


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
 * Step 2: setup memory
 */
    opt_block_size = 256 * 1024L;

#if defined(LZO_MM_AHSHIFT)
    /* reduce memory requirements for ancient 16-bit DOS 640kB real-mode */
    if (LZO_MM_AHSHIFT != 3)
        opt_block_size = 16 * 1024L;
#endif


/*
 * Step 3: get options
 */

    while (i < argc && argv[i][0] == '-')
    {
        if (strcmp(argv[i],"-d") == 0)
            opt_decompress = 1;
        else if (strcmp(argv[i],"-t") == 0)
            opt_test = 1;
        else if (strcmp(argv[i],"-9") == 0)
            opt_compression_level = 9;
        else if (argv[i][1] == 'b' && argv[i][2])
        {
            long b = atol(&argv[i][2]);
            if (b >= 1024L && b <= 8*1024*1024L)
                opt_block_size = (lzo_uint) b;
            else
            {
                printf("%s: invalid block_size in option '%s'.\n", progname, argv[i]);
                usage();
            }
        }
        else if (strcmp(argv[i],"--debug") == 0)
            opt_debug += 1;
        else
            usage();
        i++;
    }
    if (opt_test && i >= argc)
        usage();
    if (!opt_test && i + 2 != argc)
        usage();


/*
 * Step 4: process file(s)
 */

    if (opt_test)
    {
        while (i < argc && r == 0)
        {
            in_name = argv[i++];
            fi = xopen_fi(in_name);
            r = do_decompress(fi, NULL);
            if (r == 0)
                printf("%s: %s tested ok (%lu -> %lu bytes)\n",
                        progname, in_name, total_in, total_out);
            xclose(fi); fi = NULL;
        }
    }
    else if (opt_decompress)
    {
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_decompress(fi, fo);
        if (r == 0)
            printf("%s: decompressed %lu into %lu bytes\n",
                    progname, total_in, total_out);
    }
    else /* compress */
    {
        in_name = argv[i++];
        out_name = argv[i++];
        fi = xopen_fi(in_name);
        fo = xopen_fo(out_name);
        r = do_compress(fi, fo, opt_compression_level, opt_block_size);
        if (r == 0)
            printf("%s: compressed %lu into %lu bytes\n",
                    progname, total_in, total_out);
    }

    xclose(fi); fi = NULL;
    xclose(fo); fo = NULL;
    return r;
}


/* vim:set ts=4 sw=4 et: */
