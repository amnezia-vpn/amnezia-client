dnl Serial 3 mfx/m4/acc.m4

AC_DEFUN([mfx_ACC_CHECK_ENDIAN], [
AC_C_BIGENDIAN([AC_DEFINE(ACC_ABI_BIG_ENDIAN,1,[Define to 1 if your machine is big endian.])],[AC_DEFINE(ACC_ABI_LITTLE_ENDIAN,1,[Define to 1 if your machine is little endian.])])
])

AC_DEFUN([mfx_ACC_CHECK_HEADERS], [
AC_HEADER_TIME
AC_CHECK_HEADERS([assert.h ctype.h dirent.h errno.h fcntl.h float.h limits.h malloc.h memory.h setjmp.h signal.h stdarg.h stddef.h stdint.h stdio.h stdlib.h string.h strings.h time.h unistd.h utime.h sys/mman.h sys/resource.h sys/stat.h sys/time.h sys/types.h sys/wait.h])
])

AC_DEFUN([mfx_ACC_CHECK_FUNCS], [
AC_CHECK_FUNCS(access alloca atexit atoi atol chmod chown clock_getcpuclockid clock_getres clock_gettime ctime difftime fstat getenv getpagesize getrusage gettimeofday gmtime isatty localtime longjmp lstat memcmp memcpy memmove memset mkdir mktime mmap mprotect munmap qsort raise rmdir setjmp signal snprintf strcasecmp strchr strdup strerror strftime stricmp strncasecmp strnicmp strrchr strstr time umask utime vsnprintf)
])

AC_DEFUN([mfx_ACC_CHECK_SIZEOF], [
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)

AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int16)
AC_CHECK_SIZEOF(__int32)
AC_CHECK_SIZEOF(__int64)

AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(ptrdiff_t)
])

AC_DEFUN([mfx_ACC_ACCCHK], [
mfx_tmp=$1
mfx_save_CPPFLAGS=$CPPFLAGS
dnl in Makefile.in $(INCLUDES) will be before $(CPPFLAGS), so we mimic this here
test "X$mfx_tmp" = "X" || CPPFLAGS="$mfx_tmp $CPPFLAGS"

AC_MSG_CHECKING([whether your compiler passes the ACC conformance test])

AC_LANG_CONFTEST([AC_LANG_PROGRAM(
[[#define ACC_CFG_NO_CONFIG_HEADER 1
#include "acc/acc.h"
#include "acc/acc_incd.h"

#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT_HEADER(expr)
#include "acc/acc_chk.ch"

#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT(expr)
static void test_acc_compile_time_assert(void) {
#include "acc/acc_chk.ch"
}

#undef NDEBUG
#include <assert.h>
#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     assert(expr);
static int test_acc_run_time_assert(int r) {
#include "acc/acc_chk.ch"
return r;
}
]], [[
test_acc_compile_time_assert();
if (test_acc_run_time_assert(1) != 1) return 1;
]]
)])

mfx_tmp=FAILED
_AC_COMPILE_IFELSE([], [mfx_tmp=yes])
rm -f conftest.$ac_ext conftest.$ac_objext

CPPFLAGS=$mfx_save_CPPFLAGS

AC_MSG_RESULT([$mfx_tmp])
case x$mfx_tmp in
  xpassed | xyes) ;;
  *)
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([Your compiler failed the ACC conformance test - for details see ])
    AC_MSG_NOTICE([`config.log'. Please check that log file and consider sending])
    AC_MSG_NOTICE([a patch or bug-report to <${PACKAGE_BUGREPORT}>.])
    AC_MSG_NOTICE([Thanks for your support.])
    AC_MSG_NOTICE([])
    AC_MSG_ERROR([ACC conformance test failed. Stop.])
dnl    AS_EXIT
    ;;
esac
])

dnl Serial 3 mfx/m4/acc_miniacc.m4

AC_DEFUN([mfx_MINIACC_ACCCHK], [
mfx_tmp=$1
mfx_save_CPPFLAGS=$CPPFLAGS
dnl in Makefile.in $(INCLUDES) will be before $(CPPFLAGS), so we mimic this here
test "X$mfx_tmp" = "X" || CPPFLAGS="$mfx_tmp $CPPFLAGS"

AC_MSG_CHECKING([whether your compiler passes the ACC conformance test])

AC_LANG_CONFTEST([AC_LANG_PROGRAM(
[[#define ACC_CFG_NO_CONFIG_HEADER 1
#define ACC_WANT_ACC_INCD_H 1
#include $2

#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT_HEADER(expr)
#define ACC_WANT_ACC_CHK_CH 1
#include $2

#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     ACC_COMPILE_TIME_ASSERT(expr)
static void test_acc_compile_time_assert(void) {
#define ACC_WANT_ACC_CHK_CH 1
#include $2
}

#undef NDEBUG
#include <assert.h>
#undef  ACCCHK_ASSERT
#define ACCCHK_ASSERT(expr)     assert(expr);
static int test_acc_run_time_assert(int r) {
#define ACC_WANT_ACC_CHK_CH 1
#include $2
return r;
}
]], [[
test_acc_compile_time_assert();
if (test_acc_run_time_assert(1) != 1) return 1;
]]
)])

mfx_tmp=FAILED
_AC_COMPILE_IFELSE([], [mfx_tmp=yes])
rm -f conftest.$ac_ext conftest.$ac_objext

CPPFLAGS=$mfx_save_CPPFLAGS

AC_MSG_RESULT([$mfx_tmp])
case x$mfx_tmp in
  xpassed | xyes) ;;
  *)
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([Your compiler failed the ACC conformance test - for details see ])
    AC_MSG_NOTICE([`config.log'. Please check that log file and consider sending])
    AC_MSG_NOTICE([a patch or bug-report to <${PACKAGE_BUGREPORT}>.])
    AC_MSG_NOTICE([Thanks for your support.])
    AC_MSG_NOTICE([])
    AC_MSG_ERROR([ACC conformance test failed. Stop.])
dnl    AS_EXIT
    ;;
esac
])

dnl Serial 2 mfx/m4/cppflags.m4

AC_DEFUN([mfx_PROG_CPPFLAGS], [
AC_MSG_CHECKING([whether the C preprocessor needs special flags])

AC_LANG_CONFTEST([AC_LANG_PROGRAM(
[[#include <limits.h>
#if (32767 >= 4294967295ul) || (65535u >= 4294967295ul)
#  include "your C preprocessor is broken 1"
#elif (0xffffu == 0xfffffffful)
#  include "your C preprocessor is broken 2"
#elif (32767 >= ULONG_MAX) || (65535u >= ULONG_MAX)
#  include "your C preprocessor is broken 3"
#endif
]], [[ ]]
)])

mfx_save_CPPFLAGS=$CPPFLAGS
mfx_tmp=ERROR
for mfx_arg in "" -no-cpp-precomp
do
  CPPFLAGS="$mfx_arg $mfx_save_CPPFLAGS"
  _AC_COMPILE_IFELSE([],
[mfx_tmp=$mfx_arg
break])
done
CPPFLAGS=$mfx_save_CPPFLAGS
rm -f conftest.$ac_ext conftest.$ac_objext
case x$mfx_tmp in
  x)
    AC_MSG_RESULT([none needed]) ;;
  xERROR)
    AC_MSG_RESULT([ERROR])
    AC_MSG_ERROR([your C preprocessor is broken - for details see config.log])
    ;;
  *)
    AC_MSG_RESULT([$mfx_tmp])
    CPPFLAGS="$mfx_tmp $CPPFLAGS"
    ;;
esac
])

dnl Serial 15  -*- Autoconf -*-
# Enable extensions on systems that normally disable them.

# Copyright (C) 2003, 2006-2017 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This definition of AC_USE_SYSTEM_EXTENSIONS is stolen from git
# Autoconf.  Perhaps we can remove this once we can assume Autoconf
# 2.70 or later everywhere, but since Autoconf mutates rapidly
# enough in this area it's likely we'll need to redefine
# AC_USE_SYSTEM_EXTENSIONS for quite some time.

# If autoconf reports a warning
#     warning: AC_COMPILE_IFELSE was called before AC_USE_SYSTEM_EXTENSIONS
# or  warning: AC_RUN_IFELSE was called before AC_USE_SYSTEM_EXTENSIONS
# the fix is
#   1) to ensure that AC_USE_SYSTEM_EXTENSIONS is never directly invoked
#      but always AC_REQUIREd,
#   2) to ensure that for each occurrence of
#        AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
#      or
#        AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
#      the corresponding gnulib module description has 'extensions' among
#      its dependencies. This will ensure that the gl_USE_SYSTEM_EXTENSIONS
#      invocation occurs in gl_EARLY, not in gl_INIT.

# AC_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
#
# Remember that #undef in AH_VERBATIM gets replaced with #define by
# AC_DEFINE.  The goal here is to define all known feature-enabling
# macros, then, if reports of conflicts are made, disable macros that
# cause problems on some platforms (such as __EXTENSIONS__).
AC_DEFUN_ONCE([AC_USE_SYSTEM_EXTENSIONS],
[AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl

  AC_CHECK_HEADER([minix/config.h], [MINIX=yes], [MINIX=])
  if test "$MINIX" = yes; then
    AC_DEFINE([_POSIX_SOURCE], [1],
      [Define to 1 if you need to in order for 'stat' and other
       things to work.])
    AC_DEFINE([_POSIX_1_SOURCE], [2],
      [Define to 2 if the system does not provide POSIX.1 features
       except with this defined.])
    AC_DEFINE([_MINIX], [1],
      [Define to 1 if on MINIX.])
    AC_DEFINE([_NETBSD_SOURCE], [1],
      [Define to 1 to make NetBSD features available.  MINIX 3 needs this.])
  fi

dnl Use a different key than __EXTENSIONS__, as that name broke existing
dnl configure.ac when using autoheader 2.62.
  AH_VERBATIM([USE_SYSTEM_EXTENSIONS],
[/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# undef _ALL_SOURCE
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
# undef _DARWIN_C_SOURCE
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# undef _POSIX_PTHREAD_SEMANTICS
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
# undef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
# undef __STDC_WANT_IEC_60559_BFP_EXT__
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
# undef __STDC_WANT_IEC_60559_DFP_EXT__
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
# undef __STDC_WANT_IEC_60559_FUNCS_EXT__
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
# undef __STDC_WANT_IEC_60559_TYPES_EXT__
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
# undef __STDC_WANT_LIB_EXT2__
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
# undef __STDC_WANT_MATH_SPEC_FUNCS__
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# undef _TANDEM_SOURCE
#endif
/* Enable X/Open extensions if necessary.  HP-UX 11.11 defines
   mbstate_t only if _XOPEN_SOURCE is defined to 500, regardless of
   whether compiling with -Ae or -D_HPUX_SOURCE=1.  */
#ifndef _XOPEN_SOURCE
# undef _XOPEN_SOURCE
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# undef __EXTENSIONS__
#endif
])
  AC_CACHE_CHECK([whether it is safe to define __EXTENSIONS__],
    [ac_cv_safe_to_define___extensions__],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
#         define __EXTENSIONS__ 1
          ]AC_INCLUDES_DEFAULT])],
       [ac_cv_safe_to_define___extensions__=yes],
       [ac_cv_safe_to_define___extensions__=no])])
  test $ac_cv_safe_to_define___extensions__ = yes &&
    AC_DEFINE([__EXTENSIONS__])
  AC_DEFINE([_ALL_SOURCE])
  AC_DEFINE([_DARWIN_C_SOURCE])
  AC_DEFINE([_GNU_SOURCE])
  AC_DEFINE([_POSIX_PTHREAD_SEMANTICS])
  AC_DEFINE([__STDC_WANT_IEC_60559_ATTRIBS_EXT__])
  AC_DEFINE([__STDC_WANT_IEC_60559_BFP_EXT__])
  AC_DEFINE([__STDC_WANT_IEC_60559_DFP_EXT__])
  AC_DEFINE([__STDC_WANT_IEC_60559_FUNCS_EXT__])
  AC_DEFINE([__STDC_WANT_IEC_60559_TYPES_EXT__])
  AC_DEFINE([__STDC_WANT_LIB_EXT2__])
  AC_DEFINE([__STDC_WANT_MATH_SPEC_FUNCS__])
  AC_DEFINE([_TANDEM_SOURCE])
  AC_CACHE_CHECK([whether _XOPEN_SOURCE should be defined],
    [ac_cv_should_define__xopen_source],
    [ac_cv_should_define__xopen_source=no
     AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
          #include <wchar.h>
          mbstate_t x;]])],
       [],
       [AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM([[
             #define _XOPEN_SOURCE 500
             #include <wchar.h>
             mbstate_t x;]])],
          [ac_cv_should_define__xopen_source=yes])])])
  test $ac_cv_should_define__xopen_source = yes &&
    AC_DEFINE([_XOPEN_SOURCE], [500])
])# AC_USE_SYSTEM_EXTENSIONS

# gl_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
AC_DEFUN_ONCE([gl_USE_SYSTEM_EXTENSIONS],
[
  dnl Require this macro before AC_USE_SYSTEM_EXTENSIONS.
  dnl gnulib does not need it. But if it gets required by third-party macros
  dnl after AC_USE_SYSTEM_EXTENSIONS is required, autoconf 2.62..2.63 emit a
  dnl warning: "AC_COMPILE_IFELSE was called before AC_USE_SYSTEM_EXTENSIONS".
  dnl Note: We can do this only for one of the macros AC_AIX, AC_GNU_SOURCE,
  dnl AC_MINIX. If people still use AC_AIX or AC_MINIX, they are out of luck.
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
])

dnl Serial 4 mfx/m4/limits.m4

AC_DEFUN([mfx_CHECK_HEADER_SANE_LIMITS_H], [
AC_CACHE_CHECK([whether limits.h is sane],
mfx_cv_header_sane_limits_h,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <limits.h>
#if (32767 >= 4294967295ul) || (65535u >= 4294967295ul)
#  if defined(__APPLE__) && defined(__GNUC__)
#    error "your preprocessor is broken - use compiler option -no-cpp-precomp"
#  else
#    include "your preprocessor is broken"
#  endif
#endif
#define MFX_0xffff          0xffff
#define MFX_0xffffffffL     4294967295ul
#if !defined(CHAR_BIT) || (CHAR_BIT != 8)
#  include "error CHAR_BIT"
#endif
#if !defined(UCHAR_MAX)
#  include "error UCHAR_MAX 1"
#endif
#if !defined(USHRT_MAX)
#  include "error USHRT_MAX 1"
#endif
#if !defined(UINT_MAX)
#  include "error UINT_MAX 1"
#endif
#if !defined(ULONG_MAX)
#  include "error ULONG_MAX 1"
#endif
#if !defined(SHRT_MAX)
#  include "error SHRT_MAX 1"
#endif
#if !defined(INT_MAX)
#  include "error INT_MAX 1"
#endif
#if !defined(LONG_MAX)
#  include "error LONG_MAX 1"
#endif
#if (UCHAR_MAX < 1)
#  include "error UCHAR_MAX 2"
#endif
#if (USHRT_MAX < 1)
#  include "error USHRT_MAX 2"
#endif
#if (UINT_MAX < 1)
#  include "error UINT_MAX 2"
#endif
#if (ULONG_MAX < 1)
#  include "error ULONG_MAX 2"
#endif
#if (UCHAR_MAX < 0xff)
#  include "error UCHAR_MAX 3"
#endif
#if (USHRT_MAX < MFX_0xffff)
#  include "error USHRT_MAX 3"
#endif
#if (UINT_MAX < MFX_0xffff)
#  include "error UINT_MAX 3"
#endif
#if (ULONG_MAX < MFX_0xffffffffL)
#  include "error ULONG_MAX 3"
#endif
#if (USHRT_MAX > UINT_MAX)
#  include "error USHRT_MAX vs UINT_MAX"
#endif
#if (UINT_MAX > ULONG_MAX)
#  include "error UINT_MAX vs ULONG_MAX"
#endif
]], [[
#if (USHRT_MAX == MFX_0xffff)
{ typedef char a_short2a[1 - 2 * !(sizeof(short) == 2)]; }
#elif (USHRT_MAX >= MFX_0xffff)
{ typedef char a_short2b[1 - 2 * !(sizeof(short) > 2)]; }
#endif
#if (UINT_MAX == MFX_0xffff)
{ typedef char a_int2a[1 - 2 * !(sizeof(int) == 2)]; }
#elif (UINT_MAX >= MFX_0xffff)
{ typedef char a_int2b[1 - 2 * !(sizeof(int) > 2)]; }
#endif
#if (ULONG_MAX == MFX_0xffff)
{ typedef char a_long2a[1 - 2 * !(sizeof(long) == 2)]; }
#elif (ULONG_MAX >= MFX_0xffff)
{ typedef char a_long2b[1 - 2 * !(sizeof(long) > 2)]; }
#endif
#if !defined(_CRAY1) /* CRAY PVP systems */
#if (USHRT_MAX == MFX_0xffffffffL)
{ typedef char a_short4a[1 - 2 * !(sizeof(short) == 4)]; }
#elif (USHRT_MAX >= MFX_0xffffffffL)
{ typedef char a_short4b[1 - 2 * !(sizeof(short) > 4)]; }
#endif
#endif /* _CRAY1 */
#if (UINT_MAX == MFX_0xffffffffL)
{ typedef char a_int4a[1 - 2 * !(sizeof(int) == 4)]; }
#elif (UINT_MAX >= MFX_0xffffffffL)
{ typedef char a_int4b[1 - 2 * !(sizeof(int) > 4)]; }
#endif
#if (ULONG_MAX == MFX_0xffffffffL)
{ typedef char a_long4a[1 - 2 * !(sizeof(long) == 4)]; }
#elif (ULONG_MAX >= MFX_0xffffffffL)
{ typedef char a_long4b[1 - 2 * !(sizeof(long) > 4)]; }
#endif
]])],
[mfx_cv_header_sane_limits_h=yes],
[mfx_cv_header_sane_limits_h=no])])
])

dnl Serial 2 mfx/m4/lzo.m4

AC_DEFUN([mfx_LZO_CHECK_ENDIAN], [
AC_C_BIGENDIAN([AC_DEFINE(LZO_ABI_BIG_ENDIAN,1,[Define to 1 if your machine is big endian.])],[AC_DEFINE(LZO_ABI_LITTLE_ENDIAN,1,[Define to 1 if your machine is little endian.])])
])

dnl Serial 3 mfx/m4/lzo_lzochk.m4

AC_DEFUN([mfx_LZO_LZOCHK], [
mfx_tmp=$1
mfx_save_CPPFLAGS=$CPPFLAGS
dnl in Makefile.in $(INCLUDES) will be before $(CPPFLAGS), so we mimic this here
test "X$mfx_tmp" = "X" || CPPFLAGS="$mfx_tmp $CPPFLAGS"

AC_MSG_CHECKING([whether your compiler passes the LZO conformance test])

AC_LANG_CONFTEST([AC_LANG_PROGRAM(
[[#include <limits.h>
#include <stddef.h>
#define LZO_CFG_NO_CONFIG_HEADER 1
#define LZO_WANT_ACC_INCD_H 1
$2
#include $3

#undef  LZOCHK_ASSERT
#define LZOCHK_ASSERT(expr)     LZO_COMPILE_TIME_ASSERT_HEADER(expr)
#define LZO_WANT_ACC_CHK_CH 1
#include $3

#undef  LZOCHK_ASSERT
#define LZOCHK_ASSERT(expr)     LZO_COMPILE_TIME_ASSERT(expr)
static void test_lzo_compile_time_assert(void) {
#define LZO_WANT_ACC_CHK_CH 1
#include $3
}

#undef NDEBUG
#include <assert.h>
#undef  LZOCHK_ASSERT
#define LZOCHK_ASSERT(expr)     assert(expr);
static int test_lzo_run_time_assert(int r) {
#define LZO_WANT_ACC_CHK_CH 1
#include $3
return r;
}
]], [[
test_lzo_compile_time_assert();
if (test_lzo_run_time_assert(1) != 1) return 1;
]]
)])

mfx_tmp=FAILED
_AC_COMPILE_IFELSE([], [mfx_tmp=yes])
rm -f conftest.$ac_ext conftest.$ac_objext

CPPFLAGS=$mfx_save_CPPFLAGS

AC_MSG_RESULT([$mfx_tmp])
case x$mfx_tmp in
  xpassed | xyes) ;;
  *)
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([Your compiler failed the LZO conformance test - for details see ])
    AC_MSG_NOTICE([`config.log'. Please check that log file and consider sending])
    AC_MSG_NOTICE([a patch or bug-report to <${PACKAGE_BUGREPORT}>.])
    AC_MSG_NOTICE([Thanks for your support.])
    AC_MSG_NOTICE([])
    AC_MSG_ERROR([LZO conformance test failed. Stop.])
dnl    AS_EXIT
    ;;
esac
])

dnl Serial 2 mfx/m4/mfx.m4

AC_DEFUN([mfx_CHECK_SIZEOF], [
AC_CHECK_SIZEOF(__int32)
AC_CHECK_SIZEOF(intmax_t)
AC_CHECK_SIZEOF(uintmax_t)
AC_CHECK_SIZEOF(intptr_t)
AC_CHECK_SIZEOF(uintptr_t)

AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)

AC_CHECK_SIZEOF(dev_t)
AC_CHECK_SIZEOF(fpos_t)
AC_CHECK_SIZEOF(mode_t)
AC_CHECK_SIZEOF(off_t)
AC_CHECK_SIZEOF(ssize_t)
AC_CHECK_SIZEOF(time_t)
])#

AC_DEFUN([mfx_CHECK_LIB_WINMM], [
if test "X$GCC" = Xyes; then
case $host_os in
cygwin* | mingw* | pw32*)
     test "X$LIBS" != "X" && LIBS="$LIBS "
     LIBS="${LIBS}-lwinmm" ;;
esac
fi
])

dnl Serial 2 mfx/m4/nrv.m4

AC_DEFUN([mfx_NRV_CHECK_ENDIAN], [
AC_C_BIGENDIAN([AC_DEFINE(NRV_ABI_BIG_ENDIAN,1,[Define to 1 if your machine is big endian.])],[AC_DEFINE(NRV_ABI_LITTLE_ENDIAN,1,[Define to 1 if your machine is little endian.])])
])
# Checks for stat-related time functions.

# Copyright (C) 1998-1999, 2001, 2003, 2005-2007, 2009-2017 Free Software
# Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert.

# st_atim.tv_nsec - Linux, Solaris, Cygwin
# st_atimespec.tv_nsec - FreeBSD, NetBSD, if ! defined _POSIX_SOURCE
# st_atimensec - FreeBSD, NetBSD, if defined _POSIX_SOURCE
# st_atim.st__tim.tv_nsec - UnixWare (at least 2.1.2 through 7.1)

# st_birthtimespec - FreeBSD, NetBSD (hidden on OpenBSD 3.9, anyway)
# st_birthtim - Cygwin 1.7.0+

AC_DEFUN([gl_STAT_TIME],
[
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_HEADERS_ONCE([sys/time.h])

  AC_CHECK_MEMBERS([struct stat.st_atim.tv_nsec],
    [AC_CACHE_CHECK([whether struct stat.st_atim is of type struct timespec],
       [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec],
       [AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
          [[
            #include <sys/types.h>
            #include <sys/stat.h>
            #if HAVE_SYS_TIME_H
            # include <sys/time.h>
            #endif
            #include <time.h>
            struct timespec ts;
            struct stat st;
          ]],
          [[
            st.st_atim = ts;
          ]])],
          [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec=yes],
          [ac_cv_typeof_struct_stat_st_atim_is_struct_timespec=no])])
     if test $ac_cv_typeof_struct_stat_st_atim_is_struct_timespec = yes; then
       AC_DEFINE([TYPEOF_STRUCT_STAT_ST_ATIM_IS_STRUCT_TIMESPEC], [1],
         [Define to 1 if the type of the st_atim member of a struct stat is
          struct timespec.])
     fi],
    [AC_CHECK_MEMBERS([struct stat.st_atimespec.tv_nsec], [],
       [AC_CHECK_MEMBERS([struct stat.st_atimensec], [],
          [AC_CHECK_MEMBERS([struct stat.st_atim.st__tim.tv_nsec], [], [],
             [#include <sys/types.h>
              #include <sys/stat.h>])],
          [#include <sys/types.h>
           #include <sys/stat.h>])],
       [#include <sys/types.h>
        #include <sys/stat.h>])],
    [#include <sys/types.h>
     #include <sys/stat.h>])
])

# Check for st_birthtime, a feature from UFS2 (FreeBSD, NetBSD, OpenBSD, etc.)
# and NTFS (Cygwin).
# There was a time when this field was named st_createtime (21 June
# 2002 to 16 July 2002) But that window is very small and applied only
# to development code, so systems still using that configuration are
# not supported.  See revisions 1.10 and 1.11 of FreeBSD's
# src/sys/ufs/ufs/dinode.h.
#
AC_DEFUN([gl_STAT_BIRTHTIME],
[
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_HEADERS_ONCE([sys/time.h])
  AC_CHECK_MEMBERS([struct stat.st_birthtimespec.tv_nsec], [],
    [AC_CHECK_MEMBERS([struct stat.st_birthtimensec], [],
      [AC_CHECK_MEMBERS([struct stat.st_birthtim.tv_nsec], [], [],
         [#include <sys/types.h>
          #include <sys/stat.h>])],
       [#include <sys/types.h>
        #include <sys/stat.h>])],
    [#include <sys/types.h>
     #include <sys/stat.h>])
])

dnl Serial 2 mfx/m4/ucl.m4

AC_DEFUN([mfx_UCL_CHECK_ENDIAN], [
AC_C_BIGENDIAN([AC_DEFINE(UCL_ABI_BIG_ENDIAN,1,[Define to 1 if your machine is big endian.])],[AC_DEFINE(UCL_ABI_LITTLE_ENDIAN,1,[Define to 1 if your machine is little endian.])])
])
