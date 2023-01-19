/* lzo1c_d.ash -- assembler implementation of the LZO1C decompression algorithm

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
.L1:
        xorl    %eax,%eax
        movb    (%esi),%al
        incl    %esi
        cmpb    $32,%al
        jnb     .LMATCH

        orb     %al,%al
        jz      .L12
        movl    %eax,%ecx
.LIT:
        TEST_OP((%edi,%ecx),%ebx)
        TEST_IP((%esi,%ecx),%ebx)
        rep
        movsb
.LM1:
        movb    (%esi),%al
        incl    %esi

        cmpb    $32,%al
        jb      .LM2
.LMATCH:
        cmpb    $64,%al
        jb      .LN3

        movl    %eax,%ecx
        andb    $31,%al
        leal    -1(%edi),%edx
        shrl    $5,%ecx
        subl    %eax,%edx
        movb    (%esi),%al
        incl    %esi

        shll    $5,%eax
        subl    %eax,%edx
        incl    %ecx
        xchgl   %esi,%edx
        TEST_LOOKBEHIND(%esi)
        TEST_OP((%edi,%ecx),%ebx)
        rep
        movsb
        movl    %edx,%esi
        jmp     .L1

        ALIGN3
.L12:
        LODSB
        leal    32(%eax),%ecx
        cmpb    $248,%al
        jb      .LIT

        movl    $280,%ecx
        subb    $248,%al
        jz      .L11
        xchgl   %eax,%ecx
        xorb    %al,%al
        shll    %cl,%eax
        xchgl   %eax,%ecx
.L11:
        TEST_OP((%edi,%ecx),%ebx)
        TEST_IP((%esi,%ecx),%ebx)
        rep
        movsb
        jmp     .L1

        ALIGN3
.LM2:
        leal    -1(%edi),%edx
        subl    %eax,%edx
        LODSB
        shll    $5,%eax
        subl    %eax,%edx
        xchgl   %esi,%edx
        TEST_LOOKBEHIND(%esi)
        TEST_OP(4(%edi),%ebx)
        movsb
        movsb
        movsb
        movl    %edx,%esi
        movsb
        xorl    %eax,%eax
        jmp     .LM1
.LN3:
        andb    $31,%al
        movl    %eax,%ecx
        jnz     .LN6
        movb    $31,%cl
.LN4:
        LODSB
        orb     %al,%al
        jnz     .LN5
        addl    N_255,%ecx
        jmp     .LN4

        ALIGN3
.LN5:
        addl    %eax,%ecx
.LN6:
        movb    (%esi),%al
        incl    %esi

        movl    %eax,%ebx
        andb    $63,%al
        movl    %edi,%edx
        subl    %eax,%edx

        movb    (%esi),%al
        incl    %esi

        shll    $6,%eax
        subl    %eax,%edx
        cmpl    %edi,%edx
        jz      .LEOF

        xchgl   %edx,%esi
        leal    3(%ecx),%ecx
        TEST_LOOKBEHIND(%esi)
        TEST_OP((%edi,%ecx),%eax)
        rep
        movsb

        movl    %edx,%esi
        xorl    %eax,%eax
        shrl    $6,%ebx
        movl    %ebx,%ecx
        jnz     .LIT
        jmp     .L1

.LEOF:
/****   xorl    %eax,%eax          eax=0 from above */

        cmpl    $1,%ecx         /* ecx must be 1 */
        setnz   %al


/* vim:set ts=4 sw=4 et: */
