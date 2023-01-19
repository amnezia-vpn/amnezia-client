#
#  configure.ac -- autoconf configuration for the LZO library
#
#  This file is part of the LZO data compression library.
#
#  Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
#  All Rights Reserved.
#
#  The LZO library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of
#  the License, or (at your option) any later version.
#
#  The LZO library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with the LZO library; see the file COPYING.
#  If not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#  Markus F.X.J. Oberhumer
#  <markus@oberhumer.com>
#  http://www.oberhumer.com/opensource/lzo/
#


# /***********************************************************************
# // Init
# ************************************************************************/

AC_COPYRIGHT([Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer.
All Rights Reserved.
This configure script may be copied, distributed and modified under the
terms of the GNU General Public License; see COPYING for more details.])

AC_PREREQ(2.69)
AC_INIT([LZO],[2.10],[markus@oberhumer.com],[lzo],[http://www.oberhumer.com/opensource/lzo/])
AC_MSG_NOTICE([Configuring LZO $PACKAGE_VERSION])
AC_CONFIG_SRCDIR(src/lzo_init.c)
AC_CONFIG_AUX_DIR(autoconf)
m4_include([autoconf/local.m4])
AC_PREFIX_DEFAULT(/usr/local)
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_MAINTAINER_MODE

if test -z "$ac_abs_top_srcdir"; then
    _AC_SRCDIRS([.])
fi
if test -z "$ac_abs_top_srcdir"; then
    as_fn_error 1 "internal error: ac_abs_top_srcdir is not set"
fi
if test -r .Conf.settings1; then
    . ./.Conf.settings1
fi

AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AR
AM_PROG_AS
AC_PROG_CPP
mfx_PROG_CPPFLAGS
AC_C_CONST
mfx_LZO_CHECK_ENDIAN
AC_SYS_LARGEFILE

AM_INIT_AUTOMAKE([1.15 gnu subdir-objects -Wall -Werror])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config.h:config.hin])
AH_TOP([#ifndef __LZO_AUTOCONF_CONFIG_H_INCLUDED
#define __LZO_AUTOCONF_CONFIG_H_INCLUDED 1])
AH_BOTTOM([#endif /* already included */])

AC_ENABLE_STATIC
AC_DISABLE_SHARED
LT_INIT
PKG_INSTALLDIR


# /***********************************************************************
# // Checks for header files
# ************************************************************************/

mfx_ACC_CHECK_HEADERS
dnl AC_CHECK_HEADERS([sys/param.h sys/resource.h sys/times.h])
if test "X$ac_cv_header_limits_h" != Xyes; then
    AC_MSG_ERROR([<limits.h> header not found])
fi
mfx_CHECK_HEADER_SANE_LIMITS_H
if test "X$mfx_cv_header_sane_limits_h" != Xyes; then
    AC_MSG_ERROR([your <limits.h> header is broken - for details see config.log])
fi


# /***********************************************************************
# // Checks for typedefs and structures
# ************************************************************************/

AC_TYPE_OFF_T
AC_CHECK_TYPE(ptrdiff_t,long)
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL

mfx_ACC_CHECK_SIZEOF
mfx_CHECK_SIZEOF


# /***********************************************************************
# // Checks for library functions
# ************************************************************************/

mfx_ACC_CHECK_FUNCS
mfx_CHECK_LIB_WINMM


# /***********************************************************************
# // Checks for assembler
# ************************************************************************/

AC_ARG_ENABLE(asm, AS_HELP_STRING(--disable-asm,disable assembly versions))

asm_arch=
asm_dir=
asm_msg_amd64=no
asm_msg_i386=no

case "$host_cpu-$ac_cv_sizeof_void_p" in
    amd64-8 | x86_64-8) asm_arch="amd64"; asm_dir="asm/amd64/src_gas/elf64"; asm_mode="amd64_src_gas_elf64" ;;
    i?86-4) asm_arch="i386"; asm_dir="asm/i386/src_gas"; asm_mode="i386_src_gas" ;;
    *) enable_asm=no ;;
esac
LZO_USE_ASM_i386_src_gas=no

AC_MSG_CHECKING([whether to build assembly versions])
if test "X$enable_asm" != Xno; then
mfx_compile_S='${CCAS} ${CCASFLAGS} -c conftest.S 1>&AS_MESSAGE_LOG_FD'
cat > conftest.S <<EOF
#if !defined(__i386__) && !defined(__i386)
#error
this+is+an+error
#endif
#include "$ac_abs_top_srcdir/$asm_dir/lzo1x_f1.S"
EOF
enable_asm=no
if AC_TRY_EVAL(mfx_compile_S); then
    if AC_TRY_COMMAND([test -s conftest.$ac_objext]); then
        enable_asm=yes
        eval asm_msg_$asm_arch="'yes [[$asm_dir]]'"
        eval LZO_USE_ASM_$asm_mode=yes
    fi
fi
if test "X$enable_asm" = Xno; then
    echo "configure: failed program was:" >&AS_MESSAGE_LOG_FD
    cat conftest.S >&AS_MESSAGE_LOG_FD
fi
rm -rf conftest*
fi
AC_MSG_RESULT([$enable_asm])

AM_CONDITIONAL(LZO_USE_ASM_i386_src_gas, [test "X$LZO_USE_ASM_i386_src_gas" != Xno])
AM_CONDITIONAL(LZO_USE_ASM_i386_obj_elf32, [false])


# /***********************************************************************
# // Write output files
# ************************************************************************/

mfx_LZO_LZOCHK(["-I$srcdir/include -I$srcdir"],[#include <lzo/lzoconf.h>],[<src/lzo_supp.h>])

if test -r .Conf.settings2; then
    . ./.Conf.settings2
fi

my_DEFS="-DLZO_HAVE_CONFIG_H=1"
if test "X$enable_asm" != Xno; then
    my_DEFS="$my_DEFS -DLZO_USE_ASM=1"
fi

AC_CONFIG_COMMANDS_PRE([
    test "X$DEFS" = "X-DHAVE_CONFIG_H" && DEFS=
    test "X$DEFS" != "X" && DEFS="$DEFS "
    DEFS="${DEFS}${my_DEFS}"
])
AC_CONFIG_FILES([Makefile lzo2.pc])
AC_OUTPUT


##   enable AMD64 assembly code : ${asm_msg_amd64}
cat <<EOF

   LZO configuration summary
   -------------------------
   LZO version                : ${PACKAGE_VERSION}
   configured for host        : ${host_cpu}-${host_vendor}-${host_os}
   source code location       : ${srcdir}
   compiler                   : ${CC}
   preprocessor definitions   : ${DEFS}
   preprocessor flags         : ${CPPFLAGS}
   compiler flags             : ${CFLAGS}
   build static library       : ${enable_static}
   build shared library       : ${enable_shared}
   enable i386 assembly code  : ${asm_msg_i386}


   LZO ${PACKAGE_VERSION} configured.

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

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/


Type 'make' to build LZO.
Type 'make check' and 'make test' to test LZO.
Type 'make install' to install LZO.
After installing LZO, please have a look at 'examples/simple.c'.

EOF

# vim:set ts=4 sw=4 et:
