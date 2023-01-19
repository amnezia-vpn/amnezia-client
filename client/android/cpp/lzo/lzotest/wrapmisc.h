/* wrapmisc.h -- misc wrapper functions for the test driver

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
// compression levels of zlib
**************************************************************************/

#if defined(ALG_ZLIB)

#define ZLIB_MEM_COMPRESS       0
#define ZLIB_MEM_DECOMPRESS     0

static
int zlib_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem,
                                int method, int compression_level )
{
    int err;
    uLong destLen;

    assert(method == Z_DEFLATED);
    destLen = (uLong) *dst_len;
    err = compress2(dst, &destLen, src, (uLong) src_len, compression_level);
    *dst_len = destLen;
    LZO_UNUSED(method);
    LZO_UNUSED(wrkmem);
    return err;
}


M_PRIVATE(int)
zlib_decompress         ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    int err;
    uLong destLen;

    destLen = (uLong) *dst_len;
    err = uncompress(dst, &destLen, src, (uLong) src_len);
    *dst_len = destLen;
    LZO_UNUSED(wrkmem);
    return err;
}


M_PRIVATE(int)
zlib_8_1_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,1); }

M_PRIVATE(int)
zlib_8_2_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,2); }

M_PRIVATE(int)
zlib_8_3_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,3); }

M_PRIVATE(int)
zlib_8_4_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,4); }

M_PRIVATE(int)
zlib_8_5_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,5); }

M_PRIVATE(int)
zlib_8_6_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,6); }

M_PRIVATE(int)
zlib_8_7_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,7); }

M_PRIVATE(int)
zlib_8_8_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,8); }

M_PRIVATE(int)
zlib_8_9_compress       ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return zlib_compress(src,src_len,dst,dst_len,wrkmem,Z_DEFLATED,9); }


#endif /* ALG_ZLIB */


/*************************************************************************
// compression levels of bzip2
**************************************************************************/

#if defined(ALG_BZIP2)

#define BZIP2_MEM_COMPRESS      0
#define BZIP2_MEM_DECOMPRESS    0

static
int bzip2_compress      ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem,
                                int compression_level )
{
    int err;
    unsigned destLen;
    union { const m_bytep csrc; char *src; } u;

    u.csrc = src; /* UNCONST */
    destLen = *dst_len;
    err = BZ2_bzBuffToBuffCompress((char*)dst, &destLen, u.src, src_len, compression_level, 0, 0);
    *dst_len = destLen;
    LZO_UNUSED(wrkmem);
    return err;
}


M_PRIVATE(int)
bzip2_decompress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    int err;
    unsigned destLen;
    union { const m_bytep csrc; char *src; } u;

    u.csrc = src; /* UNCONST */
    destLen = *dst_len;
    err = BZ2_bzBuffToBuffDecompress((char*)dst, &destLen, u.src, src_len, 0, 0);
    *dst_len = destLen;
    LZO_UNUSED(wrkmem);
    return err;
}


M_PRIVATE(int)
bzip2_1_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,1); }

M_PRIVATE(int)
bzip2_2_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,2); }

M_PRIVATE(int)
bzip2_3_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,3); }

M_PRIVATE(int)
bzip2_4_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,4); }

M_PRIVATE(int)
bzip2_5_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,5); }

M_PRIVATE(int)
bzip2_6_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,6); }

M_PRIVATE(int)
bzip2_7_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,7); }

M_PRIVATE(int)
bzip2_8_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,8); }

M_PRIVATE(int)
bzip2_9_compress        ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{ return bzip2_compress(src,src_len,dst,dst_len,wrkmem,9); }


#endif /* ALG_BZIP2 */


/*************************************************************************
// other wrappers (for benchmarking the checksum algorithms)
**************************************************************************/

#if defined(ALG_ZLIB)

M_PRIVATE(int)
zlib_adler32_x_compress ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    uLong adler;
    adler = adler32(1L, src, (uInt) src_len);
    *dst_len = src_len;
    LZO_UNUSED(adler);
    LZO_UNUSED(dst);
    LZO_UNUSED(wrkmem);
    return 0;
}


M_PRIVATE(int)
zlib_crc32_x_compress   ( const m_bytep src, m_uint  src_len,
                                m_bytep dst, m_uintp dst_len,
                                m_voidp wrkmem )
{
    uLong crc;
    crc = crc32(0L, src, (uInt) src_len);
    *dst_len = src_len;
    LZO_UNUSED(crc);
    LZO_UNUSED(dst);
    LZO_UNUSED(wrkmem);
    return 0;
}

#endif /* ALG_ZLIB */


/* vim:set ts=4 sw=4 et: */
