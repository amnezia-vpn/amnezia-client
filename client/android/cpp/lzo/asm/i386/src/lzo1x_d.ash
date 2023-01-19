/* lzo1x_d.ash -- assembler implementation of the LZO1X decompression algorithm

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


#if !defined(LZO1X) && !defined(LZO1Y)
#  define LZO1X
#endif

#if defined(LZO_FAST)
#  define NN    3
#else
#  define NN    0
#endif


/***********************************************************************
// init
************************************************************************/

        xorl    %eax,%eax
        xorl    %ebx,%ebx       /* high bits 9-32 stay 0 */
        lodsb
        cmpb    $17,%al
        jbe     .L01
        subb    $17-NN,%al
#if defined(LZO_FAST)
        jmp     .LFLR
#else
        cmpb    $4,%al
        jae     .LFLR
#if 1
        TEST_OP((%edi,%eax),%edx)
        TEST_IP((%esi,%eax),%edx)
        movl    %eax,%ecx
        jmp     .LFLR2
#else
        jmp     .LFLR3
#endif
#endif


/***********************************************************************
// literal run
************************************************************************/

0:      addl    N_255,%eax
        TEST_IP(18(%esi,%eax),%edx)     /* minimum */
1:      movb    (%esi),%bl
        incl    %esi
        orb     %bl,%bl
        jz      0b
        leal    18+NN(%eax,%ebx),%eax
        jmp     3f


        ALIGN3
.L00:
#ifdef LZO_DEBUG
    andl $0xffffff00,%eax ; jnz .L_assert_fail
    andl $0xffffff00,%ebx ; jnz .L_assert_fail
    xorl %eax,%eax ; xorl %ebx,%ebx
    xorl %ecx,%ecx ; xorl %edx,%edx
#endif
        TEST_IP_R(%esi)
        LODSB
.L01:
        cmpb    $16,%al
        jae     .LMATCH

/* a literal run */
        orb     %al,%al
        jz      1b
        addl    $3+NN,%eax
3:
.LFLR:
        TEST_OP(-NN(%edi,%eax),%edx)
        TEST_IP(-NN(%esi,%eax),%edx)
#if defined(LZO_FAST)
        movl    %eax,%ecx
        NOTL_3(%eax)
        shrl    $2,%ecx
        andl    N_3,%eax
        COPYL(%esi,%edi,%edx)
        subl    %eax,%esi
        subl    %eax,%edi
#else
        movl    %eax,%ecx
        shrl    $2,%eax
        andl    N_3,%ecx
        COPYL_C(%esi,%edi,%edx,%eax)
.LFLR2:
        rep
        movsb
#endif

#ifdef LZO_DEBUG
    andl $0xffffff00,%eax ; jnz .L_assert_fail
    andl $0xffffff00,%ebx ; jnz .L_assert_fail
    xorl %eax,%eax ; xorl %ebx,%ebx
    xorl %ecx,%ecx ; xorl %edx,%edx
#endif
        LODSB
        cmpb    $16,%al
        jae     .LMATCH


/***********************************************************************
// R1
************************************************************************/

        TEST_OP(3(%edi),%edx)
        shrl    $2,%eax
        movb    (%esi),%bl
#if defined(LZO1X)
        leal    -0x801(%edi),%edx
#elif defined(LZO1Y)
        leal    -0x401(%edi),%edx
#endif
        leal    (%eax,%ebx,4),%eax
        incl    %esi
        subl    %eax,%edx
        TEST_LOOKBEHIND(%edx)
#if defined(LZO_FAST)
        movl    (%edx),%ecx
        movl    %ecx,(%edi)
#else
        movb    (%edx),%al
        movb    %al,(%edi)
        movb    1(%edx),%al
        movb    %al,1(%edi)
        movb    2(%edx),%al
        movb    %al,2(%edi)
#endif
        addl    N_3,%edi
        jmp     .LMDONE


/***********************************************************************
// M2
************************************************************************/

        ALIGN3
.LMATCH:
        cmpb    $64,%al
        jb      .LM3MATCH

/* a M2 match */
        movl    %eax,%ecx
        shrl    $2,%eax
        leal    -1(%edi),%edx
#if defined(LZO1X)
        andl    $7,%eax
        movb    (%esi),%bl
        shrl    $5,%ecx
        leal    (%eax,%ebx,8),%eax
#elif defined(LZO1Y)
        andl    N_3,%eax
        movb    (%esi),%bl
        shrl    $4,%ecx
        leal    (%eax,%ebx,4),%eax
#endif
        incl    %esi
        subl    %eax,%edx

#if defined(LZO_FAST)
#if defined(LZO1X)
        addl    $1+3,%ecx
#elif defined(LZO1Y)
        addl    $2,%ecx
#endif
#else
#if defined(LZO1X)
        incl    %ecx
#elif defined(LZO1Y)
        decl    %ecx
#endif
#endif

        cmpl    N_3,%eax
        jae     .LCOPYLONG
        jmp     .LCOPYBYTE


/***********************************************************************
// M3
************************************************************************/

0:      addl    N_255,%eax
        TEST_IP(3(%esi),%edx)       /* minimum */
1:      movb    (%esi),%bl
        incl    %esi
        orb     %bl,%bl
        jz      0b
        leal    33+NN(%eax,%ebx),%ecx
        xorl    %eax,%eax
        jmp     3f


        ALIGN3
.LM3MATCH:
        cmpb    $32,%al
        jb      .LM4MATCH

/* a M3 match */
        andl    $31,%eax
        jz      1b
        lea     2+NN(%eax),%ecx
3:
#ifdef LZO_DEBUG
    andl $0xffff0000,%eax ; jnz .L_assert_fail
#endif
        movw    (%esi),%ax
        leal    -1(%edi),%edx
        shrl    $2,%eax
        addl    $2,%esi
        subl    %eax,%edx

        cmpl    N_3,%eax
        jb      .LCOPYBYTE


/***********************************************************************
// copy match
************************************************************************/

        ALIGN1
.LCOPYLONG:                      /* copy match using longwords */
        TEST_LOOKBEHIND(%edx)
#if defined(LZO_FAST)
        leal    -3(%edi,%ecx),%eax
        shrl    $2,%ecx
        TEST_OP_R(%eax)
        COPYL(%edx,%edi,%ebx)
        movl    %eax,%edi
        xorl    %ebx,%ebx
#else
        TEST_OP((%edi,%ecx),%eax)
        movl    %ecx,%ebx
        shrl    $2,%ebx
        jz      2f
        COPYL_C(%edx,%edi,%eax,%ebx)
        andl    N_3,%ecx
        jz      1f
2:      COPYB_C(%edx,%edi,%al,%ecx)
1:
#endif

.LMDONE:
        movb    -2(%esi),%al
        andl    N_3,%eax
        jz      .L00
.LFLR3:
        TEST_OP((%edi,%eax),%edx)
        TEST_IP((%esi,%eax),%edx)
#if defined(LZO_FAST)
        movl    (%esi),%edx
        addl    %eax,%esi
        movl    %edx,(%edi)
        addl    %eax,%edi
#else
        COPYB_C(%esi,%edi,%cl,%eax)
#endif

#ifdef LZO_DEBUG
    andl $0xffffff00,%eax ; jnz .L_assert_fail
    andl $0xffffff00,%ebx ; jnz .L_assert_fail
    xorl %eax,%eax ; xorl %ebx,%ebx
    xorl %ecx,%ecx ; xorl %edx,%edx
#endif
        LODSB
        jmp     .LMATCH


        ALIGN3
.LCOPYBYTE:                      /* copy match using bytes */
        TEST_LOOKBEHIND(%edx)
        TEST_OP(-NN(%edi,%ecx),%eax)
        xchgl   %edx,%esi
#if defined(LZO_FAST)
        subl    N_3,%ecx
#endif
        rep
        movsb
        movl    %edx,%esi
        jmp     .LMDONE


/***********************************************************************
// M4
************************************************************************/

0:      addl    N_255,%ecx
        TEST_IP(3(%esi),%edx)       /* minimum */
1:      movb    (%esi),%bl
        incl    %esi
        orb     %bl,%bl
        jz      0b
        leal    9+NN(%ebx,%ecx),%ecx
        jmp     3f


        ALIGN3
.LM4MATCH:
        cmpb    $16,%al
        jb      .LM1MATCH

/* a M4 match */
        movl    %eax,%ecx
        andl    $8,%eax
        shll    $13,%eax        /* save in bit 16 */
        andl    $7,%ecx
        jz      1b
        addl    $2+NN,%ecx
3:
#ifdef LZO_DEBUG
    movl %eax,%edx ; andl $0xfffe0000,%edx ; jnz .L_assert_fail
#endif
        movw    (%esi),%ax
        addl    $2,%esi
        leal    -0x4000(%edi),%edx
        shrl    $2,%eax
        jz      .LEOF
        subl    %eax,%edx
        jmp     .LCOPYLONG


/***********************************************************************
// M1
************************************************************************/

        ALIGN3
.LM1MATCH:
/* a M1 match */
        TEST_OP(2(%edi),%edx)
        shrl    $2,%eax
        movb    (%esi),%bl
        leal    -1(%edi),%edx
        leal    (%eax,%ebx,4),%eax
        incl    %esi
        subl    %eax,%edx
        TEST_LOOKBEHIND(%edx)

        movb    (%edx),%al      /* we must use this because edx can be edi-1 */
        movb    %al,(%edi)
        movb    1(%edx),%bl
        movb    %bl,1(%edi)
        addl    $2,%edi
        jmp     .LMDONE


/***********************************************************************
//
************************************************************************/

.LEOF:
/****   xorl    %eax,%eax          eax=0 from above */

        cmpl    $3+NN,%ecx      /* ecx must be 3/6 */
        setnz   %al


/* vim:set ts=4 sw=4 et: */
