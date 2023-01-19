/* lzo_asm.h -- LZO assembler stuff

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
// <asmconfig.h>
************************************************************************/

#if !defined(__i386__)
#  error
#endif

#if !defined(IN_CONFIGURE)
#if defined(LZO_HAVE_CONFIG_H)
#  include <config.h>
#else
   /* manual configuration - see defaults below */
#  if defined(__ELF__)
#    define MFX_ASM_HAVE_TYPE 1
#    define MFX_ASM_NAME_NO_UNDERSCORES 1
#  elif defined(__linux__)              /* Linux a.out */
#    define MFX_ASM_ALIGN_PTWO 1
#  elif defined(__DJGPP__)
#    define MFX_ASM_ALIGN_PTWO 1
#  elif defined(__GO32__)               /* djgpp v1 */
#    define MFX_ASM_CANNOT_USE_EBP 1
#  elif defined(__EMX__)
#    define MFX_ASM_ALIGN_PTWO 1
#    define MFX_ASM_CANNOT_USE_EBP 1
#  endif
#endif
#endif

#if 1 && defined(__ELF__)
.section .note.GNU-stack,"",@progbits
#endif
#if 0 && defined(__ELF__)
#undef i386
.arch i386
.code32
#endif


/***********************************************************************
// name always uses underscores
// [ OLD: name (default: with underscores) ]
************************************************************************/

#if !defined(LZO_ASM_NAME)
#  define LZO_ASM_NAME(n)       _ ## n
#if 0
#  if defined(MFX_ASM_NAME_NO_UNDERSCORES)
#    define LZO_ASM_NAME(n)     n
#  else
#    define LZO_ASM_NAME(n)     _ ## n
#  endif
#endif
#endif


/***********************************************************************
// .type (default: do not use)
************************************************************************/

#if !defined(LZO_PUBLIC)
#if defined(__LZO_DB__)
#  define LZO_PUBLIC(func) \
        .p2align 4 ; .byte 0,0,0,0,0,0,0 ; .ascii "LZO_START"
#  define LZO_PUBLIC_END(func) \
        .p2align 4,0x90 ; .ascii "LZO_END"
#elif defined(MFX_ASM_HAVE_TYPE)
#  define LZO_PUBLIC(func) \
        ALIGN3 ; .type LZO_ASM_NAME(func),@function ; \
        .globl LZO_ASM_NAME(func) ; LZO_ASM_NAME(func):
#  define LZO_PUBLIC_END(func) \
        .size LZO_ASM_NAME(func),.-LZO_ASM_NAME(func)
#else
#  define LZO_PUBLIC(func) \
        ALIGN3 ; .globl LZO_ASM_NAME(func) ; LZO_ASM_NAME(func):
#  define LZO_PUBLIC_END(func)
#endif
#endif


/***********************************************************************
// .align (default: bytes)
************************************************************************/

#if !defined(MFX_ASM_ALIGN_BYTES) && !defined(MFX_ASM_ALIGN_PTWO)
#  define MFX_ASM_ALIGN_BYTES 1
#endif

#if !defined(LZO_ASM_ALIGN)
#  if defined(MFX_ASM_ALIGN_PTWO)
#    define LZO_ASM_ALIGN(x)    .align x
#  else
#    define LZO_ASM_ALIGN(x)    .align (1 << (x))
#  endif
#endif

#define ALIGN1              LZO_ASM_ALIGN(1)
#define ALIGN2              LZO_ASM_ALIGN(2)
#define ALIGN3              LZO_ASM_ALIGN(3)


/***********************************************************************
// ebp usage (default: can use)
************************************************************************/

#if !defined(MFX_ASM_CANNOT_USE_EBP)
#  if 1 && !defined(N_3_EBP) && !defined(N_255_EBP)
#    define N_3_EBP 1
#  endif
#  if 0 && !defined(N_3_EBP) && !defined(N_255_EBP)
#    define N_255_EBP 1
#  endif
#endif

#if defined(N_3_EBP) && defined(N_255_EBP)
#  error
#endif
#if defined(MFX_ASM_CANNOT_USE_EBP)
#  if defined(N_3_EBP) || defined(N_255_EBP)
#    error
#  endif
#endif

#if !defined(N_3)
#  if defined(N_3_EBP)
#    define N_3         %ebp
#  else
#    define N_3         $3
#  endif
#endif

#if !defined(N_255)
#  if defined(N_255_EBP)
#    define N_255       %ebp
#    define NOTL_3(r)   xorl %ebp,r
#  else
#    define N_255       $255
#  endif
#endif

#if !defined(NOTL_3)
#  define NOTL_3(r)     xorl N_3,r
#endif


/***********************************************************************
//
************************************************************************/

#ifndef INP
#define INP      4+36(%esp)
#define INS      8+36(%esp)
#define OUTP    12+36(%esp)
#define OUTS    16+36(%esp)
#endif

#define INEND         4(%esp)
#define OUTEND        (%esp)


#if defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#  define TEST_IP_R(r)      cmpl r,INEND ; jb .L_input_overrun
#  define TEST_IP(addr,r)   leal addr,r ; TEST_IP_R(r)
#else
#  define TEST_IP_R(r)
#  define TEST_IP(addr,r)
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#  define TEST_OP_R(r)      cmpl r,OUTEND ; jb .L_output_overrun
#  define TEST_OP(addr,r)   leal addr,r ; TEST_OP_R(r)
#else
#  define TEST_OP_R(r)
#  define TEST_OP(addr,r)
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#  define TEST_LOOKBEHIND(r)    cmpl OUTP,r ; jb .L_lookbehind_overrun
#else
#  define TEST_LOOKBEHIND(r)
#endif


/***********************************************************************
//
************************************************************************/

#define LODSB           movb (%esi),%al ; incl %esi

#define MOVSB(r1,r2,x)  movb (r1),x ; incl r1 ; movb x,(r2) ; incl r2
#define MOVSW(r1,r2,x)  movb (r1),x ; movb x,(r2) ; \
                        movb 1(r1),x ; addl $2,r1 ; \
                        movb x,1(r2) ; addl $2,r2
#define MOVSL(r1,r2,x)  movl (r1),x ; addl $4,r1 ; movl x,(r2) ; addl $4,r2

#if defined(LZO_DEBUG)
#define COPYB_C(r1,r2,x,rc) \
                        cmpl $0,rc ; jz .L_assert_fail; \
                        9: MOVSB(r1,r2,x) ; decl rc ; jnz 9b
#define COPYL_C(r1,r2,x,rc) \
                        cmpl $0,rc ; jz .L_assert_fail; \
                        9: MOVSL(r1,r2,x) ; decl rc ; jnz 9b
#else
#define COPYB_C(r1,r2,x,rc) \
                        9: MOVSB(r1,r2,x) ; decl rc ; jnz 9b
#define COPYL_C(r1,r2,x,rc) \
                        9: MOVSL(r1,r2,x) ; decl rc ; jnz 9b
#endif

#define COPYB(r1,r2,x)  COPYB_C(r1,r2,x,%ecx)
#define COPYL(r1,r2,x)  COPYL_C(r1,r2,x,%ecx)


/***********************************************************************
// not used
************************************************************************/

#if 0

#if 0
#define REP_MOVSB(x)    rep ; movsb
#define REP_MOVSL(x)    shrl $2,%ecx ; rep ; movsl
#elif 1
#define REP_MOVSB(x)    COPYB(%esi,%edi,x)
#define REP_MOVSL(x)    shrl $2,%ecx ; COPYL(%esi,%edi,x)
#else
#define REP_MOVSB(x)    rep ; movsb
#define REP_MOVSL(x)    jmp 9f ; 8: movsb ; decl %ecx ; \
                        9: testl $3,%edi ; jnz 8b ; \
                        movl %ecx,x ; shrl $2,%ecx ; andl $3,x ; \
                        rep ; movsl ; movl x,%ecx ; rep ; movsb
#endif

#if 1
#define NEGL(x)         negl x
#else
#define NEGL(x)         xorl $-1,x ; incl x
#endif

#endif


/* vim:set ts=4 sw=4 et: */
