/* lzo1f_d.ash -- assembler implementation of the LZO1F decompression algorithm

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


/***********************************************************************
//
************************************************************************/

        ALIGN3
.L0:
        xorl    %eax,%eax
        movb    (%esi),%al
        incl    %esi
        cmpb    $31,%al
        ja      .LM2

        orb     %al,%al
        movl    %eax,%ecx
        jnz     .L2
1:
        LODSB
        orb     %al,%al
        jnz     2f
        addl    N_255,%ecx
        jmp     1b
2:
        lea     31(%eax,%ecx),%ecx
.L2:
        TEST_OP((%edi,%ecx),%ebx)
        TEST_IP((%esi,%ecx),%ebx)
        movb    %cl,%al
        shrl    $2,%ecx
        rep
        movsl
        andb    $3,%al
        jz      1f
        movl    (%esi),%ebx
        addl    %eax,%esi
        movl    %ebx,(%edi)
        addl    %eax,%edi
1:
        movb    (%esi),%al
        incl    %esi
.LM1:
        cmpb    $31,%al
        jbe     .LM21

.LM2:
        cmpb    $223,%al
        ja      .LM3

        movl    %eax,%ecx
        shrl    $2,%eax
        lea     -1(%edi),%edx
        andb    $7,%al
        shrl    $5,%ecx
        movl    %eax,%ebx

        movb    (%esi),%al
        leal    (%ebx,%eax,8),%eax
        incl    %esi
.LM5:
        subl    %eax,%edx
        addl    $2,%ecx
        xchgl   %edx,%esi
        TEST_LOOKBEHIND(%esi)
        TEST_OP((%edi,%ecx),%ebx)
        cmpl    $6,%ecx
        jb      1f
        cmpl    $4,%eax
        jb      1f
        movb    %cl,%al
        shrl    $2,%ecx
        rep
        movsl
        andb    $3,%al
        movb    %al,%cl
1:
        rep
        movsb
        movl    %edx,%esi
.LN1:
        movb    -2(%esi),%cl
        andl    $3,%ecx
        jz      .L0
        movl    (%esi),%eax
        addl    %ecx,%esi
        movl    %eax,(%edi)
        addl    %ecx,%edi
        xorl    %eax,%eax
        movb    (%esi),%al
        incl    %esi
        jmp     .LM1
.LM21:
        TEST_OP(3(%edi),%edx)
        shrl    $2,%eax
        leal    -0x801(%edi),%edx
        movl    %eax,%ecx
        movb    (%esi),%al
        incl    %esi
        leal    (%ecx,%eax,8),%eax
        subl    %eax,%edx
        TEST_LOOKBEHIND(%edx)
        movl    (%edx),%eax
        movl    %eax,(%edi)
        addl    $3,%edi
        jmp     .LN1
1:
        LODSB
        orb     %al,%al
        jnz     2f
        addl    N_255,%ecx
        jmp     1b
2:
        lea     31(%eax,%ecx),%ecx
        jmp     .LM4

        ALIGN3
.LM3:
        andb    $31,%al
        movl    %eax,%ecx
        jz      1b
.LM4:
        movl    %edi,%edx
        movw    (%esi),%ax
        addl    $2,%esi
        shrl    $2,%eax
        jnz     .LM5

.LEOF:
/****   xorl    %eax,%eax          eax=0 from above */

        cmpl    $1,%ecx         /* ecx must be 1 */
        setnz   %al


/* vim:set ts=4 sw=4 et: */
