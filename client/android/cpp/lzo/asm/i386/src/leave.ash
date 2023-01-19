/* leave.ash -- LZO assembler stuff

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

/* check uncompressed size */
#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
        cmpl    OUTEND,%edi
        ja      .L_output_overrun
#endif

/* check compressed size */
        movl    INP,%edx
        addl    INS,%edx
        cmpl    %edx,%esi       /* check compressed size */
        ja      .L_input_overrun
        jb      .L_input_not_consumed

.L_leave:
        subl    OUTP,%edi       /* write back the uncompressed size */
        movl    OUTS,%edx
        movl    %edi,(%edx)

        negl    %eax
        addl    $12,%esp
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %esi
        popl    %edi
        popl    %ebp
#if 1
        ret
#else
        jmp     .L_end
#endif


.L_error:
        movl    $1,%eax         /* LZO_E_ERROR */
        jmp     .L_leave

.L_input_not_consumed:
        movl    $8,%eax         /* LZO_E_INPUT_NOT_CONSUMED */
        jmp     .L_leave

.L_input_overrun:
        movl    $4,%eax         /* LZO_E_INPUT_OVERRUN */
        jmp     .L_leave

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
.L_output_overrun:
        movl    $5,%eax         /* LZO_E_OUTPUT_OVERRUN */
        jmp     .L_leave
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
.L_lookbehind_overrun:
        movl    $6,%eax         /* LZO_E_LOOKBEHIND_OVERRUN */
        jmp     .L_leave
#endif

#if defined(LZO_DEBUG)
.L_assert_fail:
        movl    $99,%eax
        jmp     .L_leave
#endif

.L_end:


/* vim:set ts=4 sw=4 et: */
