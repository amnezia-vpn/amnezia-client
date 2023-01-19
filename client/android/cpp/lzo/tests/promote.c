/* promote.c -- test intergral promotion

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

#include <stdio.h>

#if defined(_MSC_VER) && (_MSC_VER+0 >= 1000)
   /* disable "unreachable code" warnings */
#  pragma warning(disable: 4702)
#endif

int main(int argc, char *argv[])
{
    unsigned char c;
    int s;

    if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
        return 0;

    c = (unsigned char) (1 << (8 * sizeof(char) - 1));
    s = 8 * (int) (sizeof(int) - sizeof(char));

    printf("Integral promotion: ");
    {
    const int u = (c << s) > 0;
    if (u)
    {
        printf("Classic C (unsigned-preserving)\n");
        printf("%d %d %uU\n", c, s, (unsigned)c << s);
        return 1;
    }
    else
    {
        printf("ANSI C (value-preserving)\n");
        printf("%d %d %d\n", c, s, c << s);
        return 0;
    }
    }
}

/* vim:set ts=4 sw=4 et: */
