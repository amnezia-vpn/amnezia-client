/* sizes.c -- print sizes of various types

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


#if (defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER)
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#endif

#include <lzo/lzoconf.h>
#include <stdio.h>

#if (LZO_CC_MSC && (_MSC_VER >= 1300))
   /* disable warning C4310: cast truncates constant value */
#  pragma warning(disable: 4310)
#endif


union _lzo_align1_t
{
    char    a_char;
};

struct _lzo_align2_t
{
    char    a_char;
};

struct _lzo_align3_t
{
    char    a_char;
    long    a_long;
};

struct _lzo_align4_t
{
    char    a_char;
    char *  a_char_p;
};

struct _lzo_align5_t
{
    char    a_char1;
    long    a_long;
    char    a_char2;
    char *  a_char_p;
};

union _lzo_align6_t
{
    char    a_char;
    long    a_long;
    char *  a_char_p;
    lzo_bytep   a_lzobytep;
};


#define print_size(type) \
    sprintf(s,"sizeof(%s)",#type); \
    printf("%-30s %2ld\n", s, (long)sizeof(type));

#define print_ssize(type,m) \
    sprintf(s,"sizeof(%s)",#type); \
    printf("%-30s %2ld %20ld\n", s, (long)sizeof(type), (long)(m));

#define print_usize(type,m) \
    sprintf(s,"sizeof(%s)",#type); \
    printf("%-30s %2ld %20lu\n", s, (long)sizeof(type), (unsigned long)(m));


int main(int argc, char *argv[])
{
    char s[80];

    print_ssize(char,CHAR_MAX);
    print_usize(unsigned char,UCHAR_MAX);
    print_ssize(short,SHRT_MAX);
    print_usize(unsigned short,USHRT_MAX);
    print_ssize(int,INT_MAX);
    print_usize(unsigned int,UINT_MAX);
    print_ssize(long,LONG_MAX);
    print_usize(unsigned long,ULONG_MAX);
    printf("\n");
    print_size(char *);
    print_size(void (*)(void));
    printf("\n");
    print_ssize(lzo_int,LZO_INT_MAX);
    print_usize(lzo_uint,LZO_UINT_MAX);
    print_size(lzo_bytep);
    printf("\n");
    print_size(union _lzo_align1_t);
    print_size(struct _lzo_align2_t);
    print_size(struct _lzo_align3_t);
    print_size(struct _lzo_align4_t);
    print_size(struct _lzo_align5_t);
    print_size(union _lzo_align6_t);

    if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
        return 0;
    return 0;
}


/* vim:set ts=4 sw=4 et: */
