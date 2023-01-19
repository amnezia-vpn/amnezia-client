/* lzotest.c -- very comprehensive test driver for the LZO library

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


/*************************************************************************
// util
**************************************************************************/

/* portability layer */
#define WANT_LZO_MALLOC 1
#define WANT_LZO_FREAD 1
#define WANT_LZO_WILDARGV 1
#define WANT_LZO_PCLOCK 1
#define LZO_WANT_ACCLIB_GETOPT 1
#include "examples/portab.h"

#if defined(HAVE_STRNICMP) && !defined(HAVE_STRNCASECMP)
#  define strncasecmp(a,b,c) strnicmp(a,b,c)
#  define HAVE_STRNCASECMP 1
#endif

#if 0
#  define is_digit(x)   (isdigit((unsigned char)(x)))
#  define is_space(x)   (isspace((unsigned char)(x)))
#else
#  define is_digit(x)   ((unsigned)(x) - '0' <= 9)
#  define is_space(x)   ((x)==' ' || (x)=='\t' || (x)=='\r' || (x)=='\n')
#endif


/*************************************************************************
// compression include section
**************************************************************************/

#define HAVE_LZO1_H 1
#define HAVE_LZO1A_H 1
#define HAVE_LZO1B_H 1
#define HAVE_LZO1C_H 1
#define HAVE_LZO1F_H 1
#define HAVE_LZO1X_H 1
#define HAVE_LZO1Y_H 1
#define HAVE_LZO1Z_H 1
#define HAVE_LZO2A_H 1

#if defined(NO_ZLIB_H) || (SIZEOF_INT < 4)
#undef HAVE_ZLIB_H
#endif
#if defined(NO_BZLIB_H) || (SIZEOF_INT != 4)
#undef HAVE_BZLIB_H
#endif

#if 0 && defined(LZO_OS_DOS16)
/* don't make this test program too big */
#undef HAVE_LZO1_H
#undef HAVE_LZO1A_H
#undef HAVE_LZO1C_H
#undef HAVE_LZO1Z_H
#undef HAVE_LZO2A_H
#undef HAVE_LZO2B_H
#undef HAVE_ZLIB_H
#endif


/* LZO algorithms */
#if defined(HAVE_LZO1_H)
#  include <lzo/lzo1.h>
#endif
#if defined(HAVE_LZO1A_H)
#  include <lzo/lzo1a.h>
#endif
#if defined(HAVE_LZO1B_H)
#  include <lzo/lzo1b.h>
#endif
#if defined(HAVE_LZO1C_H)
#  include <lzo/lzo1c.h>
#endif
#if defined(HAVE_LZO1F_H)
#  include <lzo/lzo1f.h>
#endif
#if defined(HAVE_LZO1X_H)
#  include <lzo/lzo1x.h>
#endif
#if defined(HAVE_LZO1Y_H)
#  include <lzo/lzo1y.h>
#endif
#if defined(HAVE_LZO1Z_H)
#  include <lzo/lzo1z.h>
#endif
#if defined(HAVE_LZO2A_H)
#  include <lzo/lzo2a.h>
#endif
#if defined(HAVE_LZO2B_H)
#  include <lzo/lzo2b.h>
#endif
/* other compressors */
#if defined(HAVE_ZLIB_H)
#  include <zlib.h>
#  define ALG_ZLIB 1
#endif
#if defined(HAVE_BZLIB_H)
#  include <bzlib.h>
#  define ALG_BZIP2 1
#endif


/*************************************************************************
// enumerate all methods
**************************************************************************/

enum {
/* compression algorithms */
    M_LZO1B_1     =     1,
    M_LZO1B_2, M_LZO1B_3, M_LZO1B_4, M_LZO1B_5,
    M_LZO1B_6, M_LZO1B_7, M_LZO1B_8, M_LZO1B_9,

    M_LZO1C_1     =    11,
    M_LZO1C_2, M_LZO1C_3, M_LZO1C_4, M_LZO1C_5,
    M_LZO1C_6, M_LZO1C_7, M_LZO1C_8, M_LZO1C_9,

    M_LZO1        =    21,
    M_LZO1A       =    31,

    M_LZO1B_99    =   901,
    M_LZO1B_999   =   902,
    M_LZO1C_99    =   911,
    M_LZO1C_999   =   912,
    M_LZO1_99     =   921,
    M_LZO1A_99    =   931,

    M_LZO1F_1     =    61,
    M_LZO1F_999   =   962,
    M_LZO1X_1     =    71,
    M_LZO1X_1_11  =   111,
    M_LZO1X_1_12  =   112,
    M_LZO1X_1_15  =   115,
    M_LZO1X_999   =   972,
    M_LZO1Y_1     =    81,
    M_LZO1Y_999   =   982,
    M_LZO1Z_999   =   992,

    M_LZO2A_999   =   942,
    M_LZO2B_999   =   952,

    M_LAST_LZO_COMPRESSOR = 998,

/* other compressors */
#if defined(ALG_ZLIB)
    M_ZLIB_8_1 =  1101,
    M_ZLIB_8_2, M_ZLIB_8_3, M_ZLIB_8_4, M_ZLIB_8_5,
    M_ZLIB_8_6, M_ZLIB_8_7, M_ZLIB_8_8, M_ZLIB_8_9,
#endif
#if defined(ALG_BZIP2)
    M_BZIP2_1  =  1201,
    M_BZIP2_2, M_BZIP2_3, M_BZIP2_4, M_BZIP2_5,
    M_BZIP2_6, M_BZIP2_7, M_BZIP2_8, M_BZIP2_9,
#endif

/* dummy compressor - for benchmarking */
    M_MEMCPY      =   999,

    M_LAST_COMPRESSOR = 4999,

/* dummy algorithms - for benchmarking */
    M_MEMSET      =  5001,

/* checksum algorithms - for benchmarking */
    M_ADLER32     =  6001,
    M_CRC32       =  6002,
#if defined(ALG_ZLIB)
    M_Z_ADLER32   =  6011,
    M_Z_CRC32     =  6012,
#endif

    M_UNUSED
};


/*************************************************************************
// command line options
**************************************************************************/

int opt_verbose = 2;

long opt_c_loops = 0;
long opt_d_loops = 0;
const char *opt_corpus_path = NULL;
const char *opt_dump_compressed_data = NULL;

lzo_bool opt_use_safe_decompressor = 0;
lzo_bool opt_use_asm_decompressor = 0;
lzo_bool opt_use_asm_fast_decompressor = 0;
lzo_bool opt_optimize_compressed_data = 0;

int opt_dict = 0;
lzo_uint opt_max_dict_len = LZO_UINT_MAX;
const char *opt_dictionary_file = NULL;

lzo_bool opt_read_from_stdin = 0;

/* set these to 1 to measure the speed impact of a checksum */
lzo_bool opt_compute_adler32 = 0;
lzo_bool opt_compute_crc32 = 0;
static lzo_uint32_t adler_in, adler_out;
static lzo_uint32_t crc_in, crc_out;

lzo_bool opt_execution_time = 0;
int opt_pclock = -1;
lzo_bool opt_clear_wrkmem = 0;

static const lzo_bool opt_try_to_compress_0_bytes = 1;


/*************************************************************************
// misc globals
**************************************************************************/

static const char *progname = "";
static lzo_pclock_handle_t pch;

/* for statistics and benchmark */
int opt_totals = 0;
static unsigned long total_n = 0;
static unsigned long total_c_len = 0;
static unsigned long total_d_len = 0;
static unsigned long total_blocks = 0;
static double total_perc = 0.0;
static const char *total_method_name = NULL;
static unsigned total_method_names = 0;
/* Note: the average value of a rate (e.g. compression speed) is defined
 * by the Harmonic Mean (and _not_ by the Arithmethic Mean ) */
static unsigned long total_c_mbs_n = 0;
static unsigned long total_d_mbs_n = 0;
static double total_c_mbs_harmonic = 0.0;
static double total_d_mbs_harmonic = 0.0;
static double total_c_mbs_sum = 0.0;
static double total_d_mbs_sum = 0.0;


#if defined(HAVE_LZO1X_H)
int default_method = M_LZO1X_1;
#elif defined(HAVE_LZO1B_H)
int default_method = M_LZO1B_1;
#elif defined(HAVE_LZO1C_H)
int default_method = M_LZO1C_1;
#elif defined(HAVE_LZO1F_H)
int default_method = M_LZO1F_1;
#elif defined(HAVE_LZO1Y_H)
int default_method = M_LZO1Y_1;
#else
int default_method = M_MEMCPY;
#endif


static const int benchmark_methods[] = {
    M_LZO1B_1, M_LZO1B_9,
    M_LZO1C_1, M_LZO1C_9,
    M_LZO1F_1,
    M_LZO1X_1,
    0
};

static const int x1_methods[] = {
    M_LZO1, M_LZO1A, M_LZO1B_1, M_LZO1C_1, M_LZO1F_1, M_LZO1X_1, M_LZO1Y_1,
    0
};

static const int x99_methods[] = {
    M_LZO1_99, M_LZO1A_99, M_LZO1B_99, M_LZO1C_99,
    0
};

static const int x999_methods[] = {
    M_LZO1B_999, M_LZO1C_999, M_LZO1F_999, M_LZO1X_999, M_LZO1Y_999,
    M_LZO1Z_999,
    M_LZO2A_999,
    0
};


/* exit codes of this test program */
#define EXIT_OK         0
#define EXIT_USAGE      1
#define EXIT_FILE       2
#define EXIT_MEM        3
#define EXIT_ADLER      4
#define EXIT_LZO_ERROR  5
#define EXIT_LZO_INIT   6
#define EXIT_INTERNAL   7


/*************************************************************************
// memory setup
**************************************************************************/

static lzo_uint opt_block_size;
static lzo_uint opt_max_data_len;

typedef struct {
    lzo_bytep   ptr;
    lzo_uint    len;
    lzo_uint32_t adler;
    lzo_uint32_t crc;
    lzo_bytep   alloc_ptr;
    lzo_uint    alloc_len;
    lzo_uint    saved_len;
} mblock_t;

static mblock_t file_data;      /* original uncompressed data */
static mblock_t block_c;        /* compressed data */
static mblock_t block_d;        /* decompressed data */
static mblock_t block_w;        /* wrkmem */
static mblock_t dict;


static void mb_alloc_extra(mblock_t *mb, lzo_uint len, lzo_uint extra_bottom, lzo_uint extra_top)
{
    lzo_uint align = (lzo_uint) sizeof(lzo_align_t);

    mb->alloc_ptr = mb->ptr = NULL;
    mb->alloc_len = mb->len = 0;

    mb->alloc_len = extra_bottom + len + extra_top;
    if (mb->alloc_len == 0) mb->alloc_len = 1;
    mb->alloc_ptr = (lzo_bytep) lzo_malloc(mb->alloc_len);

    if (mb->alloc_ptr == NULL) {
        fprintf(stderr, "%s: out of memory (wanted %lu bytes)\n", progname, (unsigned long)mb->alloc_len);
        exit(EXIT_MEM);
    }
    if (mb->alloc_len >= align && __lzo_align_gap(mb->alloc_ptr, align) != 0) {
        fprintf(stderr, "%s: C library problem: malloc() returned misaligned pointer!\n", progname);
        exit(EXIT_MEM);
    }

    mb->ptr = mb->alloc_ptr + extra_bottom;
    mb->len = mb->saved_len = len;
    mb->adler = 1;
    mb->crc = 0;
}


static void mb_alloc(mblock_t *mb, lzo_uint len)
{
    mb_alloc_extra(mb, len, 0, 0);
}


static void mb_free(mblock_t *mb)
{
    if (!mb) return;
    if (mb->alloc_ptr) lzo_free(mb->alloc_ptr);
    mb->alloc_ptr = mb->ptr = NULL;
    mb->alloc_len = mb->len = 0;
}


static lzo_uint get_max_compression_expansion(int m, lzo_uint bl)
{
    if (m == M_MEMCPY || m >= M_LAST_COMPRESSOR)
        return 0;
    if (m == M_LZO2A_999 || m == M_LZO2B_999)
        return bl / 8 + 256;
    if (m > 0  && m < M_LAST_LZO_COMPRESSOR)
        return bl / 16 +  64 + 3;
    return bl / 8 + 256;
}

static lzo_uint get_max_decompression_overrun(int m, lzo_uint bl)
{
    LZO_UNUSED(m);
    LZO_UNUSED(bl);
    /* may overwrite 3 bytes past the end of the decompressed block */
    if (opt_use_asm_fast_decompressor)
        return  (lzo_uint) sizeof(lzo_voidp) - 1;
    return 0;
}


/*************************************************************************
// dictionary support
**************************************************************************/

static void dict_alloc(lzo_uint max_dict_len)
{
    lzo_uint l = 0xbfff;    /* MAX_DICT_LEN */
    if (max_dict_len > 0 && l > max_dict_len)
        l = max_dict_len;
    mb_alloc(&dict, l);
}


/* this default dictionary does not provide good contexts... */
static void dict_set_default(void)
{
    lzo_uint d = 0;
    unsigned i, j;

    dict.len = 16 * 256;
    if (dict.len > dict.alloc_len)
        dict.len = dict.alloc_len;

    lzo_memset(dict.ptr, 0, dict.len);

    for (i = 0; i < 256; i++)
        for (j = 0; j < 16; j++) {
            if (d >= dict.len)
                goto done;
            dict.ptr[d++] = (unsigned char) i;
        }

done:
    dict.adler = lzo_adler32(1, dict.ptr, dict.len);
}


static void dict_load(const char *file_name)
{
    FILE *fp;

    dict.len = 0;
    fp = fopen(file_name, "rb");
    if (fp)
    {
        dict.len = (lzo_uint) lzo_fread(fp, dict.ptr, dict.alloc_len);
        (void) fclose(fp);
        dict.adler = lzo_adler32(1, dict.ptr, dict.len);
    }
}


/*************************************************************************
// compression database
**************************************************************************/

typedef struct
{
    const char *            name;
    int                     id;
    lzo_uint32_t            mem_compress;
    lzo_uint32_t            mem_decompress;
    lzo_compress_t          compress;
    lzo_optimize_t          optimize;
    lzo_decompress_t        decompress;
    lzo_decompress_t        decompress_safe;
    lzo_decompress_t        decompress_asm;
    lzo_decompress_t        decompress_asm_safe;
    lzo_decompress_t        decompress_asm_fast;
    lzo_decompress_t        decompress_asm_fast_safe;
    lzo_compress_dict_t     compress_dict;
    lzo_decompress_dict_t   decompress_dict_safe;
}
compress_t;

#include "asm.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "wrap.h"
#define M_PRIVATE       LZO_PRIVATE
#define m_uint          lzo_uint
#define m_uint32_t      lzo_uint32_t
#define m_voidp         lzo_voidp
#define m_bytep         lzo_bytep
#define m_uintp         lzo_uintp
#include "wrapmisc.h"
#ifdef __cplusplus
} /* extern "C" */
#endif

static const compress_t compress_database[] = {
#include "db.h"
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};


/*************************************************************************
// method info
**************************************************************************/

static
lzo_decompress_t get_decomp_info ( const compress_t *c, const char **nn )
{
    lzo_decompress_t d = 0;
    const char *n = NULL;

    /* safe has priority over asm/fast */
    if (!d && opt_use_safe_decompressor && opt_use_asm_fast_decompressor)
    {
        d = c->decompress_asm_fast_safe;
        n = " [fs]";
    }
    if (!d && opt_use_safe_decompressor && opt_use_asm_decompressor)
    {
        d = c->decompress_asm_safe;
        n = " [as]";
    }
    if (!d && opt_use_safe_decompressor)
    {
        d = c->decompress_safe;
        n = " [s]";
    }
    if (!d && opt_use_asm_fast_decompressor)
    {
        d = c->decompress_asm_fast;
        n = " [f]";
    }
    if (!d && opt_use_asm_decompressor)
    {
        d = c->decompress_asm;
        n = " [a]";
    }
    if (!d)
    {
        d = c->decompress;
        n = "";
    }
    if (!d)
        n = "(null)";

    if (opt_dict && c->decompress_dict_safe)
        n = "";

    if (nn)
        *nn = n;
    return d;
}


static
const compress_t *find_method_by_id ( int method )
{
    const compress_t *db;
    size_t size = sizeof(compress_database) / sizeof(*(compress_database));
    size_t i;

    db = compress_database;
    for (i = 0; i < size && db->name != NULL; i++, db++)
    {
        if (method == db->id)
            return db;
    }
    return NULL;
}


static
const compress_t *find_method_by_name ( const char *name )
{
    const compress_t *db;
    size_t size = sizeof(compress_database) / sizeof(*(compress_database));
    size_t i;

    db = compress_database;
    for (i = 0; i < size && db->name != NULL; i++, db++)
    {
        size_t n = strlen(db->name);

#if defined(HAVE_STRNCASECMP)
        if (strncasecmp(name,db->name,n) == 0 && (!name[n] || name[n] == ','))
            return db;
#else
        if (strncmp(name,db->name,n) == 0 && (!name[n] || name[n] == ','))
            return db;
#endif
    }
    return NULL;
}


static
lzo_bool is_compressor ( const compress_t *c )
{
    return (c->id <= M_LAST_COMPRESSOR || c->id >= 9721);
}


/*************************************************************************
// check that memory gets accessed within bounds
**************************************************************************/

static void memchecker_init ( mblock_t *mb, lzo_uint l, unsigned char random_byte )
{
    lzo_uint i;
    lzo_uint len = (lzo_uint) l;
    lzo_bytep p;

    assert(len <= mb->len);

    /* bottom */
    p = mb->ptr;
    for (i = 0; i < 16 && p > mb->alloc_ptr; i++)
        *--p = random_byte++;
    /* top */
    p = mb->ptr + len;
    for (i = 0; i < 16 && p < mb->alloc_ptr + mb->alloc_len; i++)
        *p++ = random_byte++;
#if 0 || defined(LZO_DEBUG)
    /* fill in garbage */
    p = mb->ptr;
    random_byte |= 1;
    for (i = 0; i < len; i++, random_byte += 2)
        *p++ = random_byte;
#endif
}


static int memchecker_check ( mblock_t *mb, lzo_uint l, unsigned char random_byte )
{
    lzo_uint i;
    lzo_uint len = (lzo_uint) l;
    lzo_bytep p;

    assert(len <= mb->len);

    /* bottom */
    p = mb->ptr;
    for (i = 0; i < 16 && p > mb->alloc_ptr; i++)
        if (*--p != random_byte++)
            return -1;
    /* top */
    p = mb->ptr + len;
    for (i = 0; i < 16 && p < mb->alloc_ptr + mb->alloc_len; i++)
        if (*p++ != random_byte++)
            return -1;
    return 0;
}


/*************************************************************************
// compress a block
**************************************************************************/

static
int call_compressor   ( const compress_t *c,
                        const lzo_bytep src, lzo_uint  src_len,
                              lzo_bytep dst, lzo_uintp dst_len )
{
    int r = -100;

    if (c && c->compress && block_w.len >= c->mem_compress)
    {
        unsigned char random_byte = (unsigned char) src_len;
        memchecker_init(&block_w, c->mem_compress, random_byte);
        if (opt_clear_wrkmem)
            lzo_memset(block_w.ptr, 0, c->mem_compress);

        if (opt_dict && c->compress_dict)
            r = c->compress_dict(src,src_len,dst,dst_len,block_w.ptr,dict.ptr,dict.len);
        else
            r = c->compress(src,src_len,dst,dst_len,block_w.ptr);

        if (memchecker_check(&block_w, c->mem_compress, random_byte) != 0)
            printf("WARNING: wrkmem overwrite error (compress) !!!\n");
    }

    if (r == 0 && opt_compute_adler32)
    {
        lzo_uint32_t adler;
        adler = lzo_adler32(0, NULL, 0);
        adler = lzo_adler32(adler, src, src_len);
        adler_in = adler;
    }
    if (r == 0 && opt_compute_crc32)
    {
        lzo_uint32_t crc;
        crc = lzo_crc32(0, NULL, 0);
        crc = lzo_crc32(crc, src, src_len);
        crc_in = crc;
    }

    return r;
}


/*************************************************************************
// decompress a block
**************************************************************************/

static
int call_decompressor ( const compress_t *c, lzo_decompress_t d,
                        const lzo_bytep src, lzo_uint  src_len,
                              lzo_bytep dst, lzo_uintp dst_len )
{
    int r = -100;

    if (c && d && block_w.len >= c->mem_decompress)
    {
        unsigned char random_byte = (unsigned char) src_len;
        memchecker_init(&block_w, c->mem_decompress, random_byte);
        if (opt_clear_wrkmem)
            lzo_memset(block_w.ptr, 0, c->mem_decompress);

        if (opt_dict && c->decompress_dict_safe)
            r = c->decompress_dict_safe(src,src_len,dst,dst_len,block_w.ptr,dict.ptr,dict.len);
        else
            r = d(src,src_len,dst,dst_len,block_w.ptr);

        if (memchecker_check(&block_w, c->mem_decompress, random_byte) != 0)
            printf("WARNING: wrkmem overwrite error (decompress) !!!\n");
    }

    if (r == 0 && opt_compute_adler32)
        adler_out = lzo_adler32(1, dst, *dst_len);
    if (r == 0 && opt_compute_crc32)
        crc_out = lzo_crc32(0, dst, *dst_len);

    return r;
}


/*************************************************************************
// optimize a block
**************************************************************************/

static
int call_optimizer   ( const compress_t *c,
                             lzo_bytep src, lzo_uint  src_len,
                             lzo_bytep dst, lzo_uintp dst_len )
{
    if (c && c->optimize && block_w.len >= c->mem_decompress)
        return c->optimize(src,src_len,dst,dst_len,block_w.ptr);
    return 0;
}


/***********************************************************************
// read a file
************************************************************************/

static int load_file(const char *file_name, lzo_uint max_data_len)
{
    FILE *fp;
#if (HAVE_FTELLO)
    off_t ll = -1;
#else
    long ll = -1;
#endif
    lzo_uint l;
    int r;
    mblock_t *mb = &file_data;

    mb_free(mb);

    fp = fopen(file_name, "rb");
    if (fp == NULL)
    {
        fflush(stdout); fflush(stderr);
        fprintf(stderr, "%s: ", file_name);
        fflush(stderr);
        perror("fopen");
        fflush(stdout); fflush(stderr);
        return EXIT_FILE;
    }
    r = fseek(fp, 0, SEEK_END);
    if (r == 0)
    {
#if (HAVE_FTELLO)
        ll = ftello(fp);
#else
        ll = ftell(fp);
#endif
        r = fseek(fp, 0, SEEK_SET);
    }
    if (r != 0 || ll < 0)
    {
        fflush(stdout); fflush(stderr);
        fprintf(stderr, "%s: ", file_name);
        fflush(stderr);
        perror("fseek");
        fflush(stdout); fflush(stderr);
        (void) fclose(fp);
        return EXIT_FILE;
    }

    l = (lzo_uint) ll;
    if (l > max_data_len) l = max_data_len;
#if (HAVE_FTELLO)
    if ((off_t) l != ll) l = max_data_len;
#else
    if ((long) l != ll) l = max_data_len;
#endif

    mb_alloc(mb, l);
    mb->len = (lzo_uint) lzo_fread(fp, mb->ptr, mb->len);

    r = ferror(fp);
    if (fclose(fp) != 0 || r != 0)
    {
        mb_free(mb);
        fflush(stdout); fflush(stderr);
        fprintf(stderr, "%s: ", file_name);
        fflush(stderr);
        perror("fclose");
        fflush(stdout); fflush(stderr);
        return EXIT_FILE;
    }

    return EXIT_OK;
}


/***********************************************************************
// print some compression statistics
************************************************************************/

static double t_div(double a, double b)
{
    return b > 0.00001 ? a / b : 0;
}

static double set_perc_d(double perc, char *s)
{
    if (perc <= 0.0) {
        strcpy(s, "0.0");
        return 0;
    }
    if (perc <= 100 - 1.0 / 16) {
        sprintf(s, "%4.1f", perc);
    }
    else {
        long p = (long) (perc + 0.5);
        if (p < 100)
            strcpy(s, "???");
        else if (p >= 9999)
            strcpy(s, "9999");
        else
            sprintf(s, "%ld", p);
    }
    return perc;
}

static double set_perc(unsigned long c_len, unsigned long d_len, char *s)
{
    double perc = 0.0;
    if (d_len > 0)
        perc = c_len * 100.0 / d_len;
    return set_perc_d(perc, s);
}


static
void print_stats ( const char *method_name, const char *file_name,
                   long t_loops, long c_loops, long d_loops,
                   double t_secs, double c_secs, double d_secs,
                   unsigned long c_len, unsigned long d_len,
                   unsigned long blocks )
{
    unsigned long x_len = d_len;
    unsigned long t_bytes, c_bytes, d_bytes;
    double c_mbs, d_mbs, t_mbs;
    double perc;
    char perc_str[4+1];

    perc = set_perc(c_len, d_len, perc_str);

    c_bytes = x_len * c_loops * t_loops;
    d_bytes = x_len * d_loops * t_loops;
    t_bytes = c_bytes + d_bytes;

    if (opt_pclock == 0)
        c_secs = d_secs = t_secs = 0.0;

    /* speed in uncompressed megabytes per second (1 megabyte = 1.000.000 bytes) */
    c_mbs = (c_secs > 0.001) ? (c_bytes / c_secs) / 1000000.0 : 0;
    d_mbs = (d_secs > 0.001) ? (d_bytes / d_secs) / 1000000.0 : 0;
    t_mbs = (t_secs > 0.001) ? (t_bytes / t_secs) / 1000000.0 : 0;

    total_n++;
    total_c_len += c_len;
    total_d_len += d_len;
    total_blocks += blocks;
    total_perc += perc;
    if (c_mbs > 0) {
        total_c_mbs_n += 1;
        total_c_mbs_harmonic += 1.0 / c_mbs;
        total_c_mbs_sum += c_mbs;
    }
    if (d_mbs > 0) {
        total_d_mbs_n += 1;
        total_d_mbs_harmonic += 1.0 / d_mbs;
        total_d_mbs_sum += d_mbs;
    }

    if (opt_verbose >= 2)
    {
        printf("  compressed into %lu bytes,  %s%%  (%s%.3f bits/byte)\n",
               c_len, perc_str, "", perc * 0.08);

#if 0
        printf("%-15s %5ld: ","overall", t_loops);
        printf("%10lu bytes, %8.2f secs, %8.3f MB/sec\n",
               t_bytes, t_secs, t_mbs);
#else
        LZO_UNUSED(t_mbs);
#endif
        printf("%-15s %5ld: ","compress", c_loops);
        printf("%10lu bytes, %8.2f secs, %8.3f MB/sec\n",
               c_bytes, c_secs, c_mbs);
        printf("%-15s %5ld: ","decompress", d_loops);
        printf("%10lu bytes, %8.2f secs, %8.3f MB/sec\n",
               d_bytes, d_secs, d_mbs);
        printf("\n");
    }

    /* create a line for util/table.pl */
    if (opt_verbose >= 1)
    {
        /* get basename */
        const char *n, *nn, *b;
        for (nn = n = b = file_name; *nn; nn++)
            if (*nn == '/' || *nn == '\\' || *nn == ':')
                b = nn + 1;
            else
                n = b;

        printf("%-13s| %-14s %8lu %4lu %9lu %4s %s%8.3f %8.3f |\n",
               method_name, n, d_len, blocks, c_len, perc_str, "", c_mbs, d_mbs);
    }

    if (opt_verbose >= 2)
        printf("\n");
}


static
void print_totals ( void )
{
    char perc_str[4+1];

    if ((opt_verbose >= 1 && total_n > 1) || (opt_totals >= 2))
    {
        unsigned long n = total_n > 0 ? total_n : 1;
        const char *t1 = "-------";
        const char *t2 = total_method_names == 1 ? total_method_name : "";
#if 1 && defined(__LZOLIB_PCLOCK_CH_INCLUDED)
        char pclock_mode[32+1];
        sprintf(pclock_mode, "[clock=%d]", pch.mode);
        t1 = pclock_mode;
        if (opt_pclock == 0) t1 = t2;
#endif

#if 1
        set_perc_d(total_perc / n, perc_str);
        printf("%-13s  %-12s %10lu %4.1f %9lu %4s %8.3f %8.3f\n",
               t1, "***AVG***",
               total_d_len / n, total_blocks * 1.0 / n, total_c_len / n, perc_str,
               t_div((double)total_c_mbs_n, total_c_mbs_harmonic),
               t_div((double)total_d_mbs_n, total_d_mbs_harmonic));
#endif
        set_perc(total_c_len, total_d_len, perc_str);
        printf("%-13s  %-12s %10lu %4lu %9lu %4s %s%8.3f %8.3f\n",
               t2, "***TOTALS***",
               total_d_len, total_blocks, total_c_len, perc_str, "",
               t_div((double)total_c_mbs_n, total_c_mbs_harmonic),
               t_div((double)total_d_mbs_n, total_d_mbs_harmonic));
    }
}


/*************************************************************************
// compress and decompress a file
**************************************************************************/

static __lzo_noinline
int process_file ( const compress_t *c, lzo_decompress_t decompress,
                   const char *method_name,
                   const char *file_name,
                   long t_loops, long c_loops, long d_loops )
{
    long t_i;
    unsigned long blocks = 0;
    unsigned long compressed_len = 0;
    double t_time = 0, c_time = 0, d_time = 0;
    lzo_pclock_t t_start, t_stop, x_start, x_stop;
    FILE *fp_dump = NULL;

    if (opt_dump_compressed_data)
        fp_dump = fopen(opt_dump_compressed_data,"wb");

/* process the file */

    lzo_pclock_flush_cpu_cache(&pch, 0);
    lzo_pclock_read(&pch, &t_start);
    for (t_i = 0; t_i < t_loops; t_i++)
    {
        lzo_uint len, c_len, c_len_max, d_len = 0;
        const lzo_bytep d = file_data.ptr;

        len = file_data.len;
        c_len = 0;
        blocks = 0;

        /* process blocks */
        if (len > 0 || opt_try_to_compress_0_bytes) do
        {
            lzo_uint bl;
            long c_i;
            int r;
            unsigned char random_byte = (unsigned char) file_data.len;
#if 1 && defined(CLOCKS_PER_SEC)
            random_byte = (unsigned char) (random_byte ^ clock());
#endif
            blocks++;

            bl = len > opt_block_size ? opt_block_size : len;
            /* update lengths for memchecker_xxx() */
            block_c.len = bl + get_max_compression_expansion(c->id, bl);
            block_d.len = bl + get_max_decompression_overrun(c->id, bl);
#if defined(__LZO_CHECKER)
            /* malloc a block of the exact size to detect any overrun */
            assert(block_c.alloc_ptr == NULL);
            assert(block_d.alloc_ptr == NULL);
            mb_alloc(&block_c, block_c.len);
            mb_alloc(&block_d, block_d.len);
#endif
            assert(block_c.len <= block_c.saved_len);
            assert(block_d.len <= block_d.saved_len);

            memchecker_init(&block_c, block_c.len, random_byte);
            memchecker_init(&block_d, block_d.len, random_byte);

        /* compress the block */
            c_len = c_len_max = 0;
            lzo_pclock_flush_cpu_cache(&pch, 0);
            lzo_pclock_read(&pch, &x_start);
            for (r = 0, c_i = 0; c_i < c_loops; c_i++)
            {
                c_len = block_c.len;
                r = call_compressor(c, d, bl, block_c.ptr, &c_len);
                if (r != 0)
                    break;
                if (c_len > c_len_max)
                    c_len_max = c_len;
                if (c_len > block_c.len)
                    goto compress_overrun;
            }
            lzo_pclock_read(&pch, &x_stop);
            c_time += lzo_pclock_get_elapsed(&pch, &x_start, &x_stop);
            if (r != 0)
            {
                printf("  compression failed in block %lu (%d) (%lu %lu)\n",
                       blocks, r, (unsigned long)c_len, (unsigned long)bl);
                return EXIT_LZO_ERROR;
            }
            if (memchecker_check(&block_c, block_c.len, random_byte) != 0)
            {
compress_overrun:
                printf("  compression overwrite error in block %lu "
                       "(%lu %lu %lu %lu)\n",
                       blocks, (unsigned long)c_len, (unsigned long)d_len, (unsigned long)bl, (unsigned long)block_c.len);
                return EXIT_LZO_ERROR;
            }

        /* optimize the compressed block */
            if (c_len < bl && opt_optimize_compressed_data)
            {
                d_len = bl;
                r = call_optimizer(c, block_c.ptr, c_len, block_d.ptr, &d_len);
                if (r != 0 || d_len != bl)
                {
                    printf("  optimization failed in block %lu (%d) "
                           "(%lu %lu %lu)\n", blocks, r,
                           (unsigned long)c_len, (unsigned long)d_len, (unsigned long)bl);
                    return EXIT_LZO_ERROR;
                }
                if (memchecker_check(&block_c, block_c.len, random_byte) != 0 ||
                    memchecker_check(&block_d, block_d.len, random_byte) != 0)
                {
                    printf("  optimize overwrite error in block %lu "
                           "(%lu %lu %lu %lu)\n",
                           blocks, (unsigned long)c_len, (unsigned long)d_len, (unsigned long)bl, (unsigned long)block_c.len);
                    return EXIT_LZO_ERROR;
                }
            }

            /* dump compressed data to disk */
            if (fp_dump)
            {
                lzo_uint l = (lzo_uint) lzo_fwrite(fp_dump, block_c.ptr, c_len);
                if (l != c_len || fflush(fp_dump) != 0) {
                    /* write error */
                    (void) fclose(fp_dump); fp_dump = NULL;
                }
            }

        /* decompress the block and verify */
            lzo_pclock_flush_cpu_cache(&pch, 0);
            lzo_pclock_read(&pch, &x_start);
            for (r = 0, c_i = 0; c_i < d_loops; c_i++)
            {
                d_len = bl;
                r = call_decompressor(c, decompress, block_c.ptr, c_len, block_d.ptr, &d_len);
                if (r != 0 || d_len != bl)
                    break;
            }
            lzo_pclock_read(&pch, &x_stop);
            d_time += lzo_pclock_get_elapsed(&pch, &x_start, &x_stop);
            if (r != 0)
            {
                printf("  decompression failed in block %lu (%d) "
                       "(%lu %lu %lu)\n", blocks, r,
                       (unsigned long)c_len, (unsigned long)d_len, (unsigned long)bl);
                return EXIT_LZO_ERROR;
            }
            if (d_len != bl)
            {
                printf("  decompression size error in block %lu (%lu %lu %lu)\n",
                       blocks, (unsigned long)c_len, (unsigned long)d_len, (unsigned long)bl);
                return EXIT_LZO_ERROR;
            }
            if (is_compressor(c))
            {
                if (lzo_memcmp(d, block_d.ptr, bl) != 0)
                {
                    lzo_uint x = 0;
                    while (x < bl && block_d.ptr[x] == d[x])
                        x++;
                    printf("  decompression data error in block %lu at offset "
                           "%lu (%lu %lu)\n", blocks, (unsigned long)x,
                           (unsigned long)c_len, (unsigned long)d_len);
                    if (opt_compute_adler32)
                        printf("      checksum: 0x%08lx 0x%08lx\n",
                               (unsigned long)adler_in, (unsigned long)adler_out);
#if 0
                    printf("Orig:  ");
                    r = (x >= 10) ? -10 : 0 - (int) x;
                    for (j = r; j <= 10 && x + j < bl; j++)
                        printf(" %02x", (int)d[x+j]);
                    printf("\nDecomp:");
                    for (j = r; j <= 10 && x + j < bl; j++)
                        printf(" %02x", (int)block_d.ptr[x+j]);
                    printf("\n");
#endif
                    return EXIT_LZO_ERROR;
                }
                if ((opt_compute_adler32 && adler_in != adler_out) ||
                    (opt_compute_crc32 && crc_in != crc_out))
                {
                    printf("  checksum error in block %lu (%lu %lu)\n",
                           blocks, (unsigned long)c_len, (unsigned long)d_len);
                    printf("      adler32: 0x%08lx 0x%08lx\n",
                           (unsigned long)adler_in, (unsigned long)adler_out);
                    printf("      crc32: 0x%08lx 0x%08lx\n",
                           (unsigned long)crc_in, (unsigned long)crc_out);
                    return EXIT_LZO_ERROR;
                }
            }

            if (memchecker_check(&block_d, block_d.len, random_byte) != 0)
            {
                printf("  decompression overwrite error in block %lu "
                       "(%lu %lu %lu %lu)\n",
                       blocks, (unsigned long)c_len, (unsigned long)d_len,
                       (unsigned long)bl, (unsigned long)block_d.len);
                return EXIT_LZO_ERROR;
            }

#if defined(__LZO_CHECKER)
            /* free in reverse order of allocations */
            mb_free(&block_d);
            mb_free(&block_c);
#endif

            d += bl;
            len -= bl;
            compressed_len += (unsigned long) c_len_max;
        }
        while (len > 0);
    }
    lzo_pclock_read(&pch, &t_stop);
    t_time += lzo_pclock_get_elapsed(&pch, &t_start, &t_stop);

    if (fp_dump) {
        (void) fclose(fp_dump); fp_dump = NULL;
    }
    opt_dump_compressed_data = NULL;    /* only dump the first file */

    print_stats(method_name, file_name,
                t_loops, c_loops, d_loops,
                t_time, c_time, d_time,
                compressed_len, (unsigned long) file_data.len, blocks);
    if (total_method_name != c->name) {
        total_method_name = c->name;
        total_method_names += 1;
    }

    return EXIT_OK;
}



static
int do_file ( int method, const char *file_name,
              long c_loops, long d_loops,
              lzo_uint32_tp p_adler, lzo_uint32_tp p_crc )
{
    int r;
    const compress_t *c;
    lzo_decompress_t decompress;
    lzo_uint32_t adler, crc;
    char method_name[256+1];
    const char *n;
    const long t_loops = 1;

    adler_in = adler_out = 0;
    crc_in = crc_out = 0;
    if (p_adler)
        *p_adler = 0;
    if (p_crc)
        *p_crc = 0;

    c = find_method_by_id(method);
    if (c == NULL || c->name == NULL || c->compress == NULL)
        return EXIT_INTERNAL;
    decompress = get_decomp_info(c,&n);
    if (!decompress || n == NULL || block_w.len < c->mem_decompress)
        return EXIT_INTERNAL;
    strcpy(method_name,c->name);
    strcat(method_name,n);

    if (c_loops < 1)  c_loops = 1;
    if (d_loops < 1)  d_loops = 1;

    fflush(stdout); fflush(stderr);

    /* read the whole file */
    r = load_file(file_name, opt_max_data_len);
    if (r != 0)
        return r;

    /* compute some checksums */
    adler = lzo_adler32(0, NULL, 0);
    adler = lzo_adler32(adler, file_data.ptr, file_data.len);
    if (p_adler)
        *p_adler = adler;
    crc = lzo_crc32(0, NULL, 0);
    crc = lzo_crc32(crc, file_data.ptr, file_data.len);
    if (p_crc)
        *p_crc = crc;

    if (opt_verbose >= 2)
    {
        printf("File %s: %lu bytes   (0x%08lx, 0x%08lx)\n",
               file_name, (unsigned long) file_data.len, (unsigned long) adler, (unsigned long) crc);
        printf("  compressing %lu bytes (%ld/%ld/%ld loops, %lu block-size)\n",
               (unsigned long) file_data.len, t_loops, c_loops, d_loops, (unsigned long) opt_block_size);
        printf("  %s\n", method_name);
    }

    r = process_file(c, decompress, method_name, file_name,
                     t_loops, c_loops, d_loops);

    return r;
}


/*************************************************************************
// Calgary Corpus and Silesia Corpus test suite driver
**************************************************************************/

struct corpus_entry_t
{
    const char *name;
    long loops;
    lzo_uint32_t adler;
    lzo_uint32_t crc;
};

const struct corpus_entry_t *opt_corpus = NULL;

static const struct corpus_entry_t calgary_corpus[] =
{
    { "bib",       8,  0x4bd09e98L, 0xb856ebe8L },
    { "book1",     1,  0xd4d3613eL, 0x24e19972L },
    { "book2",     1,  0x6fe14cc3L, 0xba0f3f26L },
    { "geo",       6,  0xf3cc5be0L, 0x4d3a6ed0L },
    { "news",      2,  0x2ed405b8L, 0xcafac853L },
    { "obj1",     35,  0x3887dd2cL, 0xc7b0cd26L },
    { "obj2",      4,  0xf89407c4L, 0x3ae33007L },
    { "paper1",   17,  0xfe65ce62L, 0x2b6baca0L },
    { "paper2",   11,  0x1238b7c2L, 0xf76cba72L },
    { "pic",       4,  0xf61a5702L, 0x4b17e59cL },
    { "progc",    25,  0x4c00ba45L, 0x6fb16094L },
    { "progl",    20,  0x4cba738eL, 0xddbf6baaL },
    { "progp",    28,  0x7495b92bL, 0x493a1809L },
    { "trans",    15,  0x52a2cec8L, 0xcdec06a6L },
    { NULL,        0,  0x00000000L, 0x00000000L }
};

static const struct corpus_entry_t silesia_corpus[] =
{
    { "dickens",   1,  0x170f606fL, 0xaf3a6b76L },
    { "mozilla",   1,  0x1188dd4eL, 0x7fb0ab7dL },
    { "mr",        1,  0xaea14b97L, 0xa341883fL },
    { "nci",       1,  0x0af16f1fL, 0x60ff63d3L },
    { "ooffice",   1,  0x83c8f689L, 0xa023e1faL },
    { "osdb",      1,  0xb825b790L, 0xa0ca388cL },
    { "reymont",   1,  0xce5c82caL, 0x50d35f03L },
    { "samba",     1,  0x19dbb9f5L, 0x2beac5f3L },
    { "sao",       1,  0x7edfc4a9L, 0xfda125bfL },
    { "webster",   1,  0xf2962fc6L, 0x01f5a2e9L },
    { "xml",       1,  0xeccd03d6L, 0xff8f3051L },
    { "x-ray",     1,  0xc95435a0L, 0xc86a35c6L },
    { NULL,        0,  0x00000000L, 0x00000000L }
};


static
int do_corpus ( const struct corpus_entry_t *corpus, int method, const char *path,
                long c_loops, long d_loops )
{
    size_t i, n;
    char name[256];

    if (path == NULL || strlen(path) >= sizeof(name) - 12)
        return EXIT_USAGE;

    strcpy(name,path);
    n = strlen(name);
    if (n > 0 && name[n-1] != '/' && name[n-1] != '\\' && name[n-1] != ':')
    {
        strcat(name,"/");
        n++;
    }

    for (i = 0; corpus[i].name != NULL; i++)
    {
        lzo_uint32_t adler, crc;
        long c = c_loops * corpus[i].loops;
        long d = d_loops * corpus[i].loops;
        int r;

        strcpy(name+n,corpus[i].name);
        r = do_file(method, name, c, d, &adler, &crc);
        if (r != 0)
            return r;
        if (adler != corpus[i].adler)
        {
            printf("  invalid test suite\n");
            return EXIT_ADLER;
        }
        if (corpus[i].crc && crc != corpus[i].crc)
        {
            printf("  internal checksum error !!  (0x%08lx 0x%08lx)\n",
                    (unsigned long) crc, (unsigned long) corpus[i].crc);
            return EXIT_INTERNAL;
        }
    }
    return EXIT_OK;
}


/*************************************************************************
// usage
**************************************************************************/

static
void usage ( const char *name, int exit_code, lzo_bool show_methods )
{
    FILE *fp;
    int i;

    fp = stdout;

    fflush(stdout); fflush(stderr);

    fprintf(fp,"Usage: %s [option..] file...\n", name);
    fprintf(fp,"\n");
    fprintf(fp,"Options:\n");
    fprintf(fp,"  -m#     compression method\n");
    fprintf(fp,"  -b#     set input block size (default %lu, max %lu)\n",
            (unsigned long) opt_block_size, (unsigned long) opt_max_data_len);
    fprintf(fp,"  -n#     number of compression/decompression runs\n");
    fprintf(fp,"  -c#     number of compression runs\n");
    fprintf(fp,"  -d#     number of decompression runs\n");
    fprintf(fp,"  -S      use safe decompressor (if available)\n");
    fprintf(fp,"  -A      use assembler decompressor (if available)\n");
    fprintf(fp,"  -F      use fast assembler decompressor (if available)\n");
    fprintf(fp,"  -O      optimize compressed data (if available)\n");
    fprintf(fp,"  -s DIR  process Calgary Corpus test suite in directory `DIR'\n");
    fprintf(fp,"  -@      read list of files to compress from stdin\n");
    fprintf(fp,"  -q      be quiet\n");
    fprintf(fp,"  -Q      be very quiet\n");
    fprintf(fp,"  -v      be verbose\n");
    fprintf(fp,"  -L      display software license\n");

    if (show_methods)
    {
#if defined(__LZOLIB_PCLOCK_CH_INCLUDED)
        lzo_pclock_t t_dummy;
        lzo_pclock_read(&pch, &t_dummy);
        (void) lzo_pclock_get_elapsed(&pch, &t_dummy, &t_dummy);
        fprintf(fp,"\nAll timings are recorded using pclock mode %d %s.\n", pch.mode, pch.name);
#endif
        fprintf(fp,"\n\n");
        fprintf(fp,"The following compression methods are available:\n");
        fprintf(fp,"\n");
        fprintf(fp,"  usage   name           memory          available extras\n");
        fprintf(fp,"  -----   ----           ------          ----------------\n");

        for (i = 0; i <= M_LAST_COMPRESSOR; i++)
        {
            const compress_t *c;
            c = find_method_by_id(i);
            if (c)
            {
                char n[16];
                const char *sep = "          ";
                unsigned long m = c->mem_compress;

                sprintf(n,"-m%d",i);
                fprintf(fp,"  %-6s  %-13s",n,c->name);
#if 1
                fprintf(fp,"%9lu", m);
#else
                m = (m + 1023) / 1024;
                fprintf(fp,"%6lu KiB", m);
#endif

                if (c->decompress_safe)
                    { fprintf(fp, "%s%s", sep, "safe"); sep = ", "; }
                if (c->decompress_asm)
                    { fprintf(fp, "%s%s", sep, "asm"); sep = ", "; }
                if (c->decompress_asm_safe)
                    { fprintf(fp, "%s%s", sep, "asm+safe"); sep = ", "; }
                if (c->decompress_asm_fast)
                    { fprintf(fp, "%s%s", sep, "fastasm"); sep = ", "; }
                if (c->decompress_asm_fast_safe)
                    { fprintf(fp, "%s%s", sep, "fastasm+safe"); sep = ", "; }
                if (c->optimize)
                    { fprintf(fp, "%s%s", sep, "optimize"); /*sep = ", ";*/ }
                fprintf(fp, "\n");
            }
        }
    }
    else
    {
        fprintf(fp,"\n");
        fprintf(fp,"Type '%s -m' to list all available methods.\n", name);
    }

    fflush(fp);
    if (exit_code < 0)
        exit_code = EXIT_USAGE;
    exit(exit_code);
}


static
void license(void)
{
    FILE *fp;

    fp = stdout;
    fflush(stdout); fflush(stderr);

fprintf(fp,
"   The LZO library is free software; you can redistribute it and/or\n"
"   modify it under the terms of the GNU General Public License as\n"
"   published by the Free Software Foundation; either version 2 of\n"
"   the License, or (at your option) any later version.\n"
"\n"
"   The LZO library is distributed in the hope that it will be useful,\n"
"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"   GNU General Public License for more details.\n"
    );
fprintf(fp,
"\n"
"   You should have received a copy of the GNU General Public License\n"
"   along with the LZO library; see the file COPYING.\n"
"   If not, write to the Free Software Foundation, Inc.,\n"
"   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.\n"
"\n"
"   Markus F.X.J. Oberhumer\n"
"   <markus@oberhumer.com>\n"
"   http://www.oberhumer.com/opensource/lzo/\n"
"\n"
    );

    fflush(fp);
    exit(EXIT_OK);
}


/*************************************************************************
// parse method option '-m'
**************************************************************************/

static int methods[256+1];
static int methods_n = 0;

static void add_method(int m)
{
    int i;

    if (m > 0)
    {
        if (!find_method_by_id(m)) {
            fprintf(stdout,"%s: invalid method %d\n",progname,m);
            exit(EXIT_USAGE);
        }

        for (i = 0; i < methods_n; i++)
            if (methods[i] == m)
                return;

        if (methods_n >= 256)
        {
            fprintf(stderr,"%s: too many methods\n",progname);
            exit(EXIT_USAGE);
        }

        methods[methods_n++] = m;
        methods[methods_n] = 0;
    }
}


static void add_methods(const int *ml)
{
    while (*ml != 0)
        add_method(*ml++);
}


static void add_all_methods(int first, int last)
{
    int m;

    for (m = first; m <= last; m++)
        if (find_method_by_id(m) != NULL)
            add_method(m);
}


static int m_strcmp(const char *a, const char *b)
{
    size_t n;

    if (a[0] == 0 || b[0] == 0)
        return 1;
    n = strlen(b);
    if (strncmp(a,b,n) == 0 && (a[n] == 0 || a[n] == ','))
        return 0;
    return 1;
}


static lzo_bool m_strisdigit(const char *s)
{
    for (;;)
    {
        if (!is_digit(*s))
            return 0;
        s++;
        if (*s == 0 || *s == ',')
            break;
    }
    return 1;
}


static void parse_methods(const char *p)
{
    const compress_t *c;

    for (;;)
    {
        if (p == NULL || p[0] == 0)
            usage(progname,-1,1);
        else if ((c = find_method_by_name(p)) != NULL)
            add_method(c->id);
        else if (m_strcmp(p,"all") == 0 || m_strcmp(p,"avail") == 0)
            add_all_methods(1,M_LAST_COMPRESSOR);
        else if (m_strcmp(p,"ALL") == 0)
        {
            add_all_methods(1,M_LAST_COMPRESSOR);
            add_all_methods(9721,9729);
            add_all_methods(9781,9789);
        }
        else if (m_strcmp(p,"lzo") == 0)
            add_all_methods(1,M_MEMCPY);
        else if (m_strcmp(p,"bench") == 0)
            add_methods(benchmark_methods);
        else if (m_strcmp(p,"m1") == 0)
            add_methods(x1_methods);
        else if (m_strcmp(p,"m99") == 0)
            add_methods(x99_methods);
        else if (m_strcmp(p,"m999") == 0)
            add_methods(x999_methods);
        else if (m_strcmp(p,"1x999") == 0)
            add_all_methods(9721,9729);
        else if (m_strcmp(p,"1y999") == 0)
            add_all_methods(9821,9829);
#if defined(ALG_ZLIB)
        else if (m_strcmp(p,"zlib") == 0)
            add_all_methods(M_ZLIB_8_1,M_ZLIB_8_9);
#endif
#if defined(ALG_BZIP2)
        else if (m_strcmp(p,"bzip2") == 0)
            add_all_methods(M_BZIP2_1,M_BZIP2_9);
#endif
        else if (m_strisdigit(p))
            add_method(atoi(p));
        else
        {
            printf("%s: invalid method '%s'\n\n",progname,p);
            exit(EXIT_USAGE);
        }

        while (*p && *p != ',')
            p++;
        while (*p == ',')
            p++;
        if (*p == 0)
            return;
    }
}


/*************************************************************************
// options
**************************************************************************/

enum {
    OPT_LONGOPT_ONLY = 512,
    OPT_ADLER32,
    OPT_CALGARY_CORPUS,
    OPT_CLEAR_WRKMEM,
    OPT_CRC32,
    OPT_DICT,
    OPT_DUMP,
    OPT_EXECUTION_TIME,
    OPT_MAX_DATA_LEN,
    OPT_MAX_DICT_LEN,
    OPT_SILESIA_CORPUS,
    OPT_PCLOCK,
    OPT_UNUSED
};

static const struct lzo_getopt_longopt_t longopts[] =
{
 /* { name  has_arg  *flag  val } */
    {"help",             0, 0, 'h'+256}, /* give help */
    {"license",          0, 0, 'L'},     /* display software license */
    {"quiet",            0, 0, 'q'},     /* quiet mode */
    {"verbose",          0, 0, 'v'},     /* verbose mode */
    {"version",          0, 0, 'V'+256}, /* display version number */

    {"adler32",          0, 0, OPT_ADLER32},
    {"calgary-corpus",   1, 0, OPT_CALGARY_CORPUS},
    {"clear-wrkmem",     0, 0, OPT_CLEAR_WRKMEM},
    {"clock",            1, 0, OPT_PCLOCK},
    {"corpus",           1, 0, OPT_CALGARY_CORPUS},
    {"crc32",            0, 0, OPT_CRC32},
    {"dict",             1, 0, OPT_DICT},
    {"dump-compressed",  1, 0, OPT_DUMP},
    {"execution-time",   0, 0, OPT_EXECUTION_TIME},
    {"max-data-length",  1, 0, OPT_MAX_DATA_LEN},
    {"max-dict-length",  1, 0, OPT_MAX_DICT_LEN},
    {"silesia-corpus",   1, 0, OPT_SILESIA_CORPUS},
    {"uclock",           1, 0, OPT_PCLOCK},
    {"methods",          1, 0, 'm'},
    {"totals",           0, 0, 'T'},

    { 0, 0, 0, 0 }
};


static int do_option(lzo_getopt_p g, int optc)
{
#define mfx_optarg      g->optarg
    switch (optc)
    {
    case 'A':
        opt_use_asm_decompressor = 1;
        break;
    case 'b':
        opt_block_size = 0; /* set to opt_max_data_len later */
        if (mfx_optarg)
        {
            if (!mfx_optarg || !is_digit(mfx_optarg[0]))
                return optc;
            opt_block_size = atol(mfx_optarg);
        }
        break;
    case 'c':
    case 'C':
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_c_loops = atol(mfx_optarg);
        break;
    case 'd':
    case 'D':
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_d_loops = atol(mfx_optarg);
        break;
    case 'F':
        opt_use_asm_fast_decompressor = 1;
        break;
    case 'h':
    case 'H':
    case '?':
    case 'h'+256:
        usage(progname,EXIT_OK,0);
        break;
    case 'L':
        license();
        break;
    case 'm':
        parse_methods(mfx_optarg);
        break;
    case 'n':
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_c_loops = opt_d_loops = atol(mfx_optarg);
        break;
    case 'O':
        opt_optimize_compressed_data = 1;
        break;
    case 'q':
        opt_verbose -= 1;
        break;
    case 'Q':
        opt_verbose = 0;
        break;
    case 's':
    case OPT_CALGARY_CORPUS:
        if (!mfx_optarg || !mfx_optarg[0])
            return optc;
        opt_corpus_path = mfx_optarg;
        opt_corpus = calgary_corpus;
        break;
    case OPT_SILESIA_CORPUS:
        if (!mfx_optarg || !mfx_optarg[0])
            return optc;
        opt_corpus_path = mfx_optarg;
        opt_corpus = silesia_corpus;
        break;
    case 'S':
        opt_use_safe_decompressor = 1;
        break;
    case 'T':
        opt_totals += 1;
        break;
    case 'v':
        opt_verbose += 1;
        break;
    case 'V':
    case 'V'+256:
        exit(EXIT_OK);
        break;
    case '@':
        opt_read_from_stdin = 1;
        break;

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9':
        /* this is a dirty hack... */
        if (g->shortpos == 0) {
            char m[2]; m[0] = (char) optc; m[1] = 0;
            parse_methods(m);
        } else {
            const char *m = &g->argv[g->optind][g->shortpos-1];
            parse_methods(m);
            ++g->optind; g->shortpos = 0;
        }
        break;

    case OPT_ADLER32:
        opt_compute_adler32 = 1;
        break;
    case OPT_CLEAR_WRKMEM:
        opt_clear_wrkmem = 1;
        break;
    case OPT_CRC32:
        opt_compute_crc32 = 1;
        break;
    case OPT_DICT:
        opt_dict = 1;
        opt_dictionary_file = mfx_optarg;
        break;
    case OPT_EXECUTION_TIME:
        opt_execution_time = 1;
        break;
    case OPT_DUMP:
        opt_dump_compressed_data = mfx_optarg;
        break;
    case OPT_MAX_DATA_LEN:
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_max_data_len = atol(mfx_optarg);
        break;
    case OPT_MAX_DICT_LEN:
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_max_dict_len = atol(mfx_optarg);
        break;
    case OPT_PCLOCK:
        if (!mfx_optarg || !is_digit(mfx_optarg[0]))
            return optc;
        opt_pclock = atoi(mfx_optarg);
#if defined(__LZOLIB_PCLOCK_CH_INCLUDED)
        if (opt_pclock > 0)
            pch.mode = opt_pclock;
#endif
        break;

    case '\0':
        return -1;
    case ':':
        return -2;
    default:
        fprintf(stderr,"%s: internal error in getopt (%d)\n",progname,optc);
        return -3;
    }
    return 0;
#undef mfx_optarg
}


static void handle_opterr(lzo_getopt_p g, const char *f, void *v)
{
    struct A { va_list ap; };
    struct A *a = (struct A *) v;
    fprintf( stderr, "%s: ", g->progname);
    if (a)
        vfprintf(stderr, f, a->ap);
    else
        fprintf( stderr, "UNKNOWN GETOPT ERROR");
    fprintf( stderr, "\n");
}


static int get_options(int argc, char **argv)
{
    lzo_getopt_t mfx_getopt;
    int optc;
    static const char shortopts[] =
        "Ab::c:C:d:D:FhHLm::n:OqQs:STvV@123456789";

    lzo_getopt_init(&mfx_getopt, 1, argc, argv);
    mfx_getopt.progname = progname;
    mfx_getopt.opterr = handle_opterr;
    while ((optc = lzo_getopt(&mfx_getopt, shortopts, longopts, NULL)) >= 0)
    {
        if (do_option(&mfx_getopt, optc) != 0)
            exit(EXIT_USAGE);
    }

    return mfx_getopt.optind;
}


/*************************************************************************
// main
**************************************************************************/

int __lzo_cdecl_main main(int argc, char *argv[])
{
    int r = EXIT_OK;
    int i, ii;
    int m;
    time_t t_total;
    const char *s;

    lzo_wildargv(&argc, &argv);
    lzo_pclock_open_default(&pch);

    progname = argv[0];
    for (s = progname; *s; s++)
        if ((*s == '/' || *s == '\\') && s[1])
            progname = s + 1;

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");


/*
 * Step 1: initialize the LZO library
 */

    if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
        exit(1);
    }


/*
 * Step 2: setup default options
 */

    opt_max_data_len = 64 * 1024L * 1024L;
    opt_block_size = 256 * 1024L;

#if (LZO_ARCH_M68K && LZO_OS_TOS)
    /* reduce memory requirements for 14 MB machines */
    opt_max_data_len = 8 * 1024L * 1024L;
#endif



/*
 * Step 3: parse options
 */

    if (argc < 2)
        usage(progname,-1,0);
    i = get_options(argc,argv);

    if (methods_n == 0)
        add_method(default_method);
    if (methods_n > 1 && opt_read_from_stdin)
    {
        printf("%s: cannot use multiple methods and '-@'\n", progname);
        exit(EXIT_USAGE);
    }

    if (opt_block_size == 0)
        opt_block_size = opt_max_data_len;
    if (opt_block_size > opt_max_data_len)
        opt_block_size = opt_max_data_len;

    if (opt_c_loops < 1)
        opt_c_loops = 1;
    if (opt_d_loops < 1)
        opt_d_loops = 1;


/*
 * Step 4: start work
 */

    block_w.len = 0;
    for (ii = 0; ii < methods_n; ii++) {
        const compress_t *c = find_method_by_id(methods[ii]);
        assert(c != NULL);
        if (c->mem_compress > block_w.len)
            block_w.len = c->mem_compress;
        if (c->mem_decompress > block_w.len)
            block_w.len = c->mem_decompress;
    }

    mb_alloc(&block_w, block_w.len);
    lzo_memset(block_w.ptr, 0, block_w.len);

#if !defined(__LZO_CHECKER)
    mb_alloc_extra(&block_c, opt_block_size + get_max_compression_expansion(-1, opt_block_size), 16, 16);
    mb_alloc_extra(&block_d, opt_block_size + get_max_decompression_overrun(-1, opt_block_size), 16, 16);
#endif

    if (opt_dict)
    {
        opt_optimize_compressed_data = 0;
        dict_alloc(opt_max_dict_len);
        if (opt_dictionary_file)
        {
            dict_load(opt_dictionary_file);
            if (dict.len > 0)
                printf("Using dictionary '%s', %lu bytes, ID 0x%08lx.\n",
                       opt_dictionary_file,
                       (unsigned long) dict.len, (unsigned long) dict.adler);
        }
        if (dict.len == 0)
        {
            dict_set_default();
            printf("Using default dictionary, %lu bytes, ID 0x%08lx.\n",
                   (unsigned long) dict.len, (unsigned long) dict.adler);
        }
    }

    t_total = time(NULL);
    ii = i;
    for (m = 0; m < methods_n && r == EXIT_OK; m++)
    {
        int method = methods[m];

        i = ii;
        if (i >= argc && opt_corpus_path == NULL && !opt_read_from_stdin)
            usage(progname,-1,0);
        if (m == 0 && opt_verbose >= 1)
            printf("%lu block-size\n\n", (unsigned long) opt_block_size);

        assert(find_method_by_id(method) != NULL);

        if (opt_corpus_path != NULL)
            r = do_corpus(opt_corpus, method, opt_corpus_path,
                          opt_c_loops, opt_d_loops);
        else
        {
            for ( ; i < argc && r == EXIT_OK; i++)
            {
                r = do_file(method,argv[i],opt_c_loops,opt_d_loops,NULL,NULL);
                if (r == EXIT_FILE)     /* ignore file errors */
                    r = EXIT_OK;
            }
            if (opt_read_from_stdin)
            {
                char buf[512], *p;

                while (r == EXIT_OK && fgets(buf,sizeof(buf)-1,stdin) != NULL)
                {
                    buf[sizeof(buf)-1] = 0;
                    p = buf + strlen(buf);
                    while (p > buf && is_space(p[-1]))
                            *--p = 0;
                    p = buf;
                    while (*p && is_space(*p))
                        p++;
                    if (*p)
                        r = do_file(method,p,opt_c_loops,opt_d_loops,NULL,NULL);
                    if (r == EXIT_FILE)     /* ignore file errors */
                        r = EXIT_OK;
                }
                opt_read_from_stdin = 0;
            }
        }
    }
    t_total = time(NULL) - t_total;

    if (opt_totals)
        print_totals();
    if (opt_execution_time || (methods_n > 1 && opt_verbose >= 1))
        printf("\n%s: execution time: %lu seconds\n", progname, (unsigned long) t_total);
    if (r != EXIT_OK)
        printf("\n%s: exit code: %d\n", progname, r);

    lzo_pclock_close(&pch);
    return r;
}


/* vim:set ts=4 sw=4 et: */
