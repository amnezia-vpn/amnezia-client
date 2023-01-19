/* lzo_supp.h -- architecture, OS and compiler specific defines

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


#ifndef __LZO_SUPP_H_INCLUDED
#define __LZO_SUPP_H_INCLUDED 1
#if (LZO_CFG_NO_CONFIG_HEADER)
#elif defined(LZO_CFG_CONFIG_HEADER)
#else
#if !(LZO_CFG_AUTO_NO_HEADERS)
#if (LZO_LIBC_NAKED)
#elif (LZO_LIBC_FREESTANDING)
#  define HAVE_LIMITS_H 1
#  define HAVE_STDARG_H 1
#  define HAVE_STDDEF_H 1
#elif (LZO_LIBC_MOSTLY_FREESTANDING)
#  define HAVE_LIMITS_H 1
#  define HAVE_SETJMP_H 1
#  define HAVE_STDARG_H 1
#  define HAVE_STDDEF_H 1
#  define HAVE_STDIO_H 1
#  define HAVE_STRING_H 1
#else
#define STDC_HEADERS 1
#define HAVE_ASSERT_H 1
#define HAVE_CTYPE_H 1
#define HAVE_DIRENT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_SETJMP_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_TIME_H 1
#define HAVE_UNISTD_H 1
#define HAVE_UTIME_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#if (LZO_OS_POSIX)
#  if (LZO_OS_POSIX_AIX)
#    define HAVE_SYS_RESOURCE_H 1
#  elif (LZO_OS_POSIX_DARWIN || LZO_OS_POSIX_FREEBSD || LZO_OS_POSIX_NETBSD || LZO_OS_POSIX_OPENBSD)
#    define HAVE_STRINGS_H 1
#    undef HAVE_MALLOC_H
#  elif (LZO_OS_POSIX_HPUX || LZO_OS_POSIX_INTERIX)
#    define HAVE_ALLOCA_H 1
#  elif (LZO_OS_POSIX_DARWIN && LZO_LIBC_MSL)
#    undef HAVE_SYS_TIME_H
#    undef HAVE_SYS_TYPES_H
#  elif (LZO_OS_POSIX_SOLARIS || LZO_OS_POSIX_SUNOS)
#    define HAVE_ALLOCA_H 1
#  endif
#  if (LZO_LIBC_DIETLIBC || LZO_LIBC_GLIBC || LZO_LIBC_UCLIBC)
#    define HAVE_STRINGS_H 1
#    define HAVE_SYS_MMAN_H 1
#    define HAVE_SYS_RESOURCE_H 1
#    define HAVE_SYS_WAIT_H 1
#  endif
#  if (LZO_LIBC_NEWLIB)
#    undef HAVE_STRINGS_H
#  endif
#elif (LZO_OS_CYGWIN)
#  define HAVE_IO_H 1
#elif (LZO_OS_EMX)
#  define HAVE_ALLOCA_H 1
#  define HAVE_IO_H 1
#elif (LZO_ARCH_M68K && LZO_OS_TOS && LZO_CC_GNUC)
#  if !defined(__MINT__)
#    undef HAVE_MALLOC_H
#  endif
#elif (LZO_ARCH_M68K && LZO_OS_TOS && (LZO_CC_PUREC || LZO_CC_TURBOC))
#  undef HAVE_DIRENT_H
#  undef HAVE_FCNTL_H
#  undef HAVE_MALLOC_H
#  undef HAVE_MEMORY_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#endif
#if (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_WIN16 || LZO_OS_WIN32 || LZO_OS_WIN64)
#define HAVE_CONIO_H 1
#define HAVE_DIRECT_H 1
#define HAVE_DOS_H 1
#define HAVE_IO_H 1
#define HAVE_SHARE_H 1
#if (LZO_CC_AZTECC)
#  undef HAVE_CONIO_H
#  undef HAVE_DIRECT_H
#  undef HAVE_DIRENT_H
#  undef HAVE_MALLOC_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (LZO_CC_BORLANDC)
#  undef HAVE_UNISTD_H
#  undef HAVE_SYS_TIME_H
#  if (LZO_OS_WIN32 || LZO_OS_WIN64)
#    undef HAVE_DIRENT_H
#  endif
#  if (__BORLANDC__ < 0x0400)
#    undef HAVE_DIRENT_H
#    undef HAVE_UTIME_H
#  endif
#elif (LZO_CC_DMC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  define HAVE_SYS_DIRENT_H 1
#elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#elif (LZO_OS_DOS32 && LZO_CC_HIGHC)
#  define HAVE_ALLOCA_H 1
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#elif (LZO_CC_IBMC && LZO_OS_OS2)
#  undef HAVE_DOS_H
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (LZO_CC_CLANG_C2 || LZO_CC_CLANG_MSC || LZO_CC_GHS || LZO_CC_INTELC_MSC || LZO_CC_MSC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (LZO_CC_LCCWIN32)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_UNISTD_H
#  undef HAVE_SYS_TIME_H
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__MINGW32__)
#  undef HAVE_UTIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (LZO_OS_WIN32 && LZO_LIBC_MSL)
#  define HAVE_ALLOCA_H 1
#  undef HAVE_DOS_H
#  undef HAVE_SHARE_H
#  undef HAVE_SYS_TIME_H
#elif (LZO_CC_NDPC)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#elif (LZO_CC_PACIFICC)
#  undef HAVE_DIRECT_H
#  undef HAVE_DIRENT_H
#  undef HAVE_FCNTL_H
#  undef HAVE_IO_H
#  undef HAVE_MALLOC_H
#  undef HAVE_MEMORY_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (LZO_OS_WIN32 && LZO_CC_PELLESC)
#  undef HAVE_DIRENT_H
#  undef HAVE_DOS_H
#  undef HAVE_MALLOC_H
#  undef HAVE_SHARE_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  if (__POCC__ < 280)
#  else
#    define HAVE_SYS_UTIME_H 1
#  endif
#elif (LZO_OS_WIN32 && LZO_CC_PGI) && defined(__MINGW32__)
#  undef HAVE_UTIME_H
#  define HAVE_SYS_UTIME_H 1
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
#elif (LZO_CC_SYMANTECC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  if (__SC__ < 0x700)
#    undef HAVE_UTIME_H
#    undef HAVE_SYS_TIME_H
#  endif
#elif (LZO_CC_TOPSPEEDC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#elif (LZO_CC_TURBOC)
#  undef HAVE_UNISTD_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#  if (LZO_OS_WIN32 || LZO_OS_WIN64)
#    undef HAVE_DIRENT_H
#  endif
#  if (__TURBOC__ < 0x0200)
#    undef HAVE_SIGNAL_H
#  endif
#  if (__TURBOC__ < 0x0400)
#    undef HAVE_DIRECT_H
#    undef HAVE_DIRENT_H
#    undef HAVE_MALLOC_H
#    undef HAVE_MEMORY_H
#    undef HAVE_UTIME_H
#  endif
#elif (LZO_CC_WATCOMC)
#  undef HAVE_DIRENT_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#  define HAVE_SYS_UTIME_H 1
#  if (__WATCOMC__ < 950)
#    undef HAVE_UNISTD_H
#  endif
#elif (LZO_CC_ZORTECHC)
#  undef HAVE_DIRENT_H
#  undef HAVE_MEMORY_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_TIME_H
#endif
#endif
#if (LZO_OS_CONSOLE)
#  undef HAVE_DIRENT_H
#endif
#if (LZO_OS_EMBEDDED)
#  undef HAVE_DIRENT_H
#endif
#if (LZO_LIBC_ISOC90 || LZO_LIBC_ISOC99)
#  undef HAVE_DIRENT_H
#  undef HAVE_FCNTL_H
#  undef HAVE_MALLOC_H
#  undef HAVE_UNISTD_H
#  undef HAVE_UTIME_H
#  undef HAVE_SYS_STAT_H
#  undef HAVE_SYS_TIME_H
#  undef HAVE_SYS_TYPES_H
#endif
#if (LZO_LIBC_GLIBC >= 0x020100ul)
#  define HAVE_STDINT_H 1
#elif (LZO_LIBC_DIETLIBC)
#  undef HAVE_STDINT_H
#elif (LZO_LIBC_UCLIBC)
#  define HAVE_STDINT_H 1
#elif (LZO_CC_BORLANDC) && (__BORLANDC__ >= 0x560)
#  undef HAVE_STDINT_H
#elif (LZO_CC_DMC) && (__DMC__ >= 0x825)
#  define HAVE_STDINT_H 1
#endif
#if (HAVE_SYS_TIME_H && HAVE_TIME_H)
#  define TIME_WITH_SYS_TIME 1
#endif
#endif
#endif
#if !(LZO_CFG_AUTO_NO_FUNCTIONS)
#if (LZO_LIBC_NAKED)
#elif (LZO_LIBC_FREESTANDING)
#elif (LZO_LIBC_MOSTLY_FREESTANDING)
#  define HAVE_LONGJMP 1
#  define HAVE_MEMCMP 1
#  define HAVE_MEMCPY 1
#  define HAVE_MEMMOVE 1
#  define HAVE_MEMSET 1
#  define HAVE_SETJMP 1
#else
#define HAVE_ACCESS 1
#define HAVE_ALLOCA 1
#define HAVE_ATEXIT 1
#define HAVE_ATOI 1
#define HAVE_ATOL 1
#define HAVE_CHMOD 1
#define HAVE_CHOWN 1
#define HAVE_CTIME 1
#define HAVE_DIFFTIME 1
#define HAVE_FILENO 1
#define HAVE_FSTAT 1
#define HAVE_GETENV 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_GMTIME 1
#define HAVE_ISATTY 1
#define HAVE_LOCALTIME 1
#define HAVE_LONGJMP 1
#define HAVE_LSTAT 1
#define HAVE_MEMCMP 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_MKTIME 1
#define HAVE_QSORT 1
#define HAVE_RAISE 1
#define HAVE_RMDIR 1
#define HAVE_SETJMP 1
#define HAVE_SIGNAL 1
#define HAVE_SNPRINTF 1
#define HAVE_STAT 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
#define HAVE_TIME 1
#define HAVE_UMASK 1
#define HAVE_UTIME 1
#define HAVE_VSNPRINTF 1
#if (LZO_OS_BEOS || LZO_OS_CYGWIN || LZO_OS_POSIX || LZO_OS_QNX || LZO_OS_VMS)
#  define HAVE_STRCASECMP 1
#  define HAVE_STRNCASECMP 1
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
#  define HAVE_STRCASECMP 1
#  define HAVE_STRNCASECMP 1
#else
#  define HAVE_STRICMP 1
#  define HAVE_STRNICMP 1
#endif
#if (LZO_OS_POSIX)
#  if (LZO_OS_POSIX_AIX)
#    define HAVE_GETRUSAGE 1
#  elif (LZO_OS_POSIX_DARWIN && LZO_LIBC_MSL)
#    undef HAVE_CHOWN
#    undef HAVE_LSTAT
#  elif (LZO_OS_POSIX_UNICOS)
#    undef HAVE_ALLOCA
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#  if (LZO_CC_TINYC)
#    undef HAVE_ALLOCA
#  endif
#  if (LZO_LIBC_DIETLIBC || LZO_LIBC_GLIBC || LZO_LIBC_UCLIBC)
#    define HAVE_GETRUSAGE 1
#    define HAVE_GETPAGESIZE 1
#    define HAVE_MMAP 1
#    define HAVE_MPROTECT 1
#    define HAVE_MUNMAP 1
#  endif
#elif (LZO_OS_CYGWIN)
#  if (LZO_CC_GNUC < 0x025a00ul)
#    undef HAVE_GETTIMEOFDAY
#    undef HAVE_LSTAT
#  endif
#  if (LZO_CC_GNUC < 0x025f00ul)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (LZO_OS_EMX)
#  undef HAVE_CHOWN
#  undef HAVE_LSTAT
#elif (LZO_ARCH_M68K && LZO_OS_TOS && LZO_CC_GNUC)
#  if !defined(__MINT__)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (LZO_ARCH_M68K && LZO_OS_TOS && (LZO_CC_PUREC || LZO_CC_TURBOC))
#  undef HAVE_ALLOCA
#  undef HAVE_ACCESS
#  undef HAVE_CHMOD
#  undef HAVE_CHOWN
#  undef HAVE_FSTAT
#  undef HAVE_GETTIMEOFDAY
#  undef HAVE_LSTAT
#  undef HAVE_SNPRINTF
#  undef HAVE_UMASK
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#endif
#if (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_WIN16 || LZO_OS_WIN32 || LZO_OS_WIN64)
#undef HAVE_CHOWN
#undef HAVE_GETTIMEOFDAY
#undef HAVE_LSTAT
#undef HAVE_UMASK
#if (LZO_CC_AZTECC)
#  undef HAVE_ALLOCA
#  undef HAVE_DIFFTIME
#  undef HAVE_FSTAT
#  undef HAVE_STRDUP
#  undef HAVE_SNPRINTF
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#elif (LZO_CC_BORLANDC)
#  if (__BORLANDC__ < 0x0400)
#    undef HAVE_ALLOCA
#    undef HAVE_UTIME
#  endif
#  if ((__BORLANDC__ < 0x0410) && LZO_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  if (__BORLANDC__ < 0x0550)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  endif
#elif (LZO_CC_DMC)
#  if (LZO_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (LZO_OS_DOS32 && LZO_CC_HIGHC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (LZO_CC_GHS)
#  undef HAVE_ALLOCA
#  ifndef snprintf
#  define snprintf _snprintf
#  endif
#  ifndef vsnprintf
#  define vsnprintf _vsnprintf
#  endif
#elif (LZO_CC_IBMC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (LZO_CC_CLANG_MSC || LZO_CC_INTELC_MSC)
#  ifndef snprintf
#  define snprintf _snprintf
#  endif
#  ifndef vsnprintf
#  define vsnprintf _vsnprintf
#  endif
#elif (LZO_CC_LCCWIN32)
#  define utime _utime
#elif (LZO_CC_CLANG_C2 || LZO_CC_MSC)
#  if (_MSC_VER < 600)
#    undef HAVE_STRFTIME
#  endif
#  if (_MSC_VER < 700)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  elif (_MSC_VER < 1500)
#    ifndef snprintf
#    define snprintf _snprintf
#    endif
#    ifndef vsnprintf
#    define vsnprintf _vsnprintf
#    endif
#  elif (_MSC_VER < 1900)
#    ifndef snprintf
#    define snprintf _snprintf
#    endif
#  endif
#  if ((_MSC_VER < 800) && LZO_OS_WIN16)
#    undef HAVE_ALLOCA
#  endif
#  if (LZO_ARCH_I086) && defined(__cplusplus)
#    undef HAVE_LONGJMP
#    undef HAVE_SETJMP
#  endif
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__MINGW32__)
#  if (LZO_CC_GNUC < 0x025f00ul)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  else
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#elif (LZO_OS_WIN32 && LZO_LIBC_MSL)
#  if (__MSL__ < 0x8000ul)
#    undef HAVE_CHMOD
#  endif
#elif (LZO_CC_NDPC)
#  undef HAVE_ALLOCA
#  undef HAVE_SNPRINTF
#  undef HAVE_STRNICMP
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#  if defined(__cplusplus)
#    undef HAVE_STAT
#  endif
#elif (LZO_CC_PACIFICC)
#  undef HAVE_ACCESS
#  undef HAVE_ALLOCA
#  undef HAVE_CHMOD
#  undef HAVE_DIFFTIME
#  undef HAVE_FSTAT
#  undef HAVE_MKTIME
#  undef HAVE_RAISE
#  undef HAVE_SNPRINTF
#  undef HAVE_STRFTIME
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#elif (LZO_OS_WIN32 && LZO_CC_PELLESC)
#  if (__POCC__ < 280)
#    define alloca _alloca
#    undef HAVE_UTIME
#  endif
#elif (LZO_OS_WIN32 && LZO_CC_PGI) && defined(__MINGW32__)
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (LZO_CC_SYMANTECC)
#  if (LZO_OS_WIN16 && (LZO_MM_MEDIUM || LZO_MM_LARGE || LZO_MM_HUGE))
#    undef HAVE_ALLOCA
#  endif
#  if (__SC__ < 0x600)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  else
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#  if (__SC__ < 0x700)
#    undef HAVE_DIFFTIME
#    undef HAVE_UTIME
#  endif
#elif (LZO_CC_TOPSPEEDC)
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#elif (LZO_CC_TURBOC)
#  undef HAVE_ALLOCA
#  undef HAVE_SNPRINTF
#  undef HAVE_VSNPRINTF
#  if (__TURBOC__ < 0x0200)
#    undef HAVE_RAISE
#    undef HAVE_SIGNAL
#  endif
#  if (__TURBOC__ < 0x0295)
#    undef HAVE_MKTIME
#    undef HAVE_STRFTIME
#  endif
#  if (__TURBOC__ < 0x0400)
#    undef HAVE_UTIME
#  endif
#elif (LZO_CC_WATCOMC)
#  if (__WATCOMC__ < 1100)
#    undef HAVE_SNPRINTF
#    undef HAVE_VSNPRINTF
#  elif (__WATCOMC__ < 1200)
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#elif (LZO_CC_ZORTECHC)
#  if (LZO_OS_WIN16 && (LZO_MM_MEDIUM || LZO_MM_LARGE || LZO_MM_HUGE))
#    undef HAVE_ALLOCA
#  endif
#  undef HAVE_DIFFTIME
#  undef HAVE_SNPRINTF
#  undef HAVE_UTIME
#  undef HAVE_VSNPRINTF
#endif
#endif
#if (LZO_OS_CONSOLE)
#  undef HAVE_ACCESS
#  undef HAVE_CHMOD
#  undef HAVE_CHOWN
#  undef HAVE_GETTIMEOFDAY
#  undef HAVE_LSTAT
#  undef HAVE_TIME
#  undef HAVE_UMASK
#  undef HAVE_UTIME
#endif
#if (LZO_LIBC_ISOC90 || LZO_LIBC_ISOC99)
#  undef HAVE_ACCESS
#  undef HAVE_CHMOD
#  undef HAVE_CHOWN
#  undef HAVE_FILENO
#  undef HAVE_FSTAT
#  undef HAVE_GETTIMEOFDAY
#  undef HAVE_LSTAT
#  undef HAVE_STAT
#  undef HAVE_UMASK
#  undef HAVE_UTIME
# if 1
#  undef HAVE_ALLOCA
#  undef HAVE_ISATTY
#  undef HAVE_MKDIR
#  undef HAVE_RMDIR
#  undef HAVE_STRDUP
#  undef HAVE_STRICMP
#  undef HAVE_STRNICMP
# endif
#endif
#endif
#endif
#if !(LZO_CFG_AUTO_NO_SIZES)
#if !defined(SIZEOF_SHORT) && defined(LZO_SIZEOF_SHORT)
#  define SIZEOF_SHORT          LZO_SIZEOF_SHORT
#endif
#if !defined(SIZEOF_INT) && defined(LZO_SIZEOF_INT)
#  define SIZEOF_INT            LZO_SIZEOF_INT
#endif
#if !defined(SIZEOF_LONG) && defined(LZO_SIZEOF_LONG)
#  define SIZEOF_LONG           LZO_SIZEOF_LONG
#endif
#if !defined(SIZEOF_LONG_LONG) && defined(LZO_SIZEOF_LONG_LONG)
#  define SIZEOF_LONG_LONG      LZO_SIZEOF_LONG_LONG
#endif
#if !defined(SIZEOF___INT32) && defined(LZO_SIZEOF___INT32)
#  define SIZEOF___INT32        LZO_SIZEOF___INT32
#endif
#if !defined(SIZEOF___INT64) && defined(LZO_SIZEOF___INT64)
#  define SIZEOF___INT64        LZO_SIZEOF___INT64
#endif
#if !defined(SIZEOF_VOID_P) && defined(LZO_SIZEOF_VOID_P)
#  define SIZEOF_VOID_P         LZO_SIZEOF_VOID_P
#endif
#if !defined(SIZEOF_SIZE_T) && defined(LZO_SIZEOF_SIZE_T)
#  define SIZEOF_SIZE_T         LZO_SIZEOF_SIZE_T
#endif
#if !defined(SIZEOF_PTRDIFF_T) && defined(LZO_SIZEOF_PTRDIFF_T)
#  define SIZEOF_PTRDIFF_T      LZO_SIZEOF_PTRDIFF_T
#endif
#endif
#if (HAVE_SIGNAL) && !defined(RETSIGTYPE)
#  define RETSIGTYPE void
#endif
#endif
#if !(LZO_CFG_SKIP_LZO_TYPES)
#if 1 && !defined(lzo_signo_t) && defined(__linux__) && defined(__dietlibc__) && (LZO_SIZEOF_INT != 4)
#  define lzo_signo_t               lzo_int32e_t
#endif
#if !defined(lzo_signo_t)
#  define lzo_signo_t               int
#endif
#if defined(__cplusplus)
extern "C" {
#endif
#if (LZO_BROKEN_CDECL_ALT_SYNTAX)
typedef void __lzo_cdecl_sighandler (*lzo_sighandler_t)(lzo_signo_t);
#else
typedef void (__lzo_cdecl_sighandler *lzo_sighandler_t)(lzo_signo_t);
#endif
#if defined(__cplusplus)
}
#endif
#endif
#endif
#if defined(LZO_WANT_ACC_INCD_H)
#  undef LZO_WANT_ACC_INCD_H
#ifndef __LZO_INCD_H_INCLUDED
#define __LZO_INCD_H_INCLUDED 1
#if (LZO_LIBC_NAKED)
#ifndef __LZO_FALLBACK_STDDEF_H_INCLUDED
#define __LZO_FALLBACK_STDDEF_H_INCLUDED 1
#if defined(__PTRDIFF_TYPE__)
typedef __PTRDIFF_TYPE__ lzo_fallback_ptrdiff_t;
#elif defined(__MIPS_PSX2__)
typedef int lzo_fallback_ptrdiff_t;
#else
typedef long lzo_fallback_ptrdiff_t;
#endif
#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ lzo_fallback_size_t;
#elif defined(__MIPS_PSX2__)
typedef unsigned int lzo_fallback_size_t;
#else
typedef unsigned long lzo_fallback_size_t;
#endif
#if !defined(ptrdiff_t)
typedef lzo_fallback_ptrdiff_t ptrdiff_t;
#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED 1
#endif
#endif
#if !defined(size_t)
typedef lzo_fallback_size_t size_t;
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED 1
#endif
#endif
#if !defined(__cplusplus) && !defined(wchar_t)
typedef unsigned short wchar_t;
#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED 1
#endif
#endif
#ifndef NULL
#if defined(__cplusplus) && defined(__GNUC__) && (__GNUC__ >= 4)
#define NULL    __null
#elif defined(__cplusplus)
#define NULL    0
#else
#define NULL    ((void*)0)
#endif
#endif
#ifndef offsetof
#define offsetof(s,m)   ((size_t)((ptrdiff_t)&(((s*)0)->m)))
#endif
#endif
#elif (LZO_LIBC_FREESTANDING)
# if defined(HAVE_STDDEF_H) && (HAVE_STDDEF_H+0)
#  include <stddef.h>
# endif
# if defined(HAVE_STDINT_H) && (HAVE_STDINT_H+0)
#  include <stdint.h>
# endif
#elif (LZO_LIBC_MOSTLY_FREESTANDING)
# if defined(HAVE_STDIO_H) && (HAVE_STDIO_H+0)
#  include <stdio.h>
# endif
# if defined(HAVE_STDDEF_H) && (HAVE_STDDEF_H+0)
#  include <stddef.h>
# endif
# if defined(HAVE_STDINT_H) && (HAVE_STDINT_H+0)
#  include <stdint.h>
# endif
#else
#include <stdio.h>
#if defined(HAVE_TIME_H) && (HAVE_TIME_H+0) && defined(__MSL__) && defined(__cplusplus)
# include <time.h>
#endif
#if defined(HAVE_SYS_TYPES_H) && (HAVE_SYS_TYPES_H+0)
# include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H) && (HAVE_SYS_STAT_H+0)
# include <sys/stat.h>
#endif
#if defined(STDC_HEADERS) && (STDC_HEADERS+0)
# include <stdlib.h>
#elif defined(HAVE_STDLIB_H) && (HAVE_STDLIB_H+0)
# include <stdlib.h>
#endif
#include <stddef.h>
#if defined(HAVE_STRING_H) && (HAVE_STRING_H+0)
# if defined(STDC_HEADERS) && (STDC_HEADERS+0)
# elif defined(HAVE_MEMORY_H) && (HAVE_MEMORY_H+0)
#  include <memory.h>
# endif
# include <string.h>
#endif
#if defined(HAVE_STRINGS_H) && (HAVE_STRINGS_H+0)
# include <strings.h>
#endif
#if defined(HAVE_INTTYPES_H) && (HAVE_INTTYPES_H+0)
# include <inttypes.h>
#endif
#if defined(HAVE_STDINT_H) && (HAVE_STDINT_H+0)
# include <stdint.h>
#endif
#if defined(HAVE_UNISTD_H) && (HAVE_UNISTD_H+0)
# include <unistd.h>
#endif
#endif
#endif
#endif
#if defined(LZO_WANT_ACC_INCE_H)
#  undef LZO_WANT_ACC_INCE_H
#ifndef __LZO_INCE_H_INCLUDED
#define __LZO_INCE_H_INCLUDED 1
#if (LZO_LIBC_NAKED)
#elif (LZO_LIBC_FREESTANDING)
#elif (LZO_LIBC_MOSTLY_FREESTANDING)
#  if (HAVE_SETJMP_H)
#    include <setjmp.h>
#  endif
#else
#if (HAVE_STDARG_H)
#  include <stdarg.h>
#endif
#if (HAVE_CTYPE_H)
#  include <ctype.h>
#endif
#if (HAVE_ERRNO_H)
#  include <errno.h>
#endif
#if (HAVE_MALLOC_H)
#  include <malloc.h>
#endif
#if (HAVE_ALLOCA_H)
#  include <alloca.h>
#endif
#if (HAVE_FCNTL_H)
#  include <fcntl.h>
#endif
#if (HAVE_DIRENT_H)
#  include <dirent.h>
#endif
#if (HAVE_SETJMP_H)
#  include <setjmp.h>
#endif
#if (HAVE_SIGNAL_H)
#  include <signal.h>
#endif
#if (HAVE_SYS_TIME_H && HAVE_TIME_H)
#  include <sys/time.h>
#  include <time.h>
#elif (HAVE_TIME_H)
#  include <time.h>
#endif
#if (HAVE_UTIME_H)
#  include <utime.h>
#elif (HAVE_SYS_UTIME_H)
#  include <sys/utime.h>
#endif
#if (HAVE_IO_H)
#  include <io.h>
#endif
#if (HAVE_DOS_H)
#  include <dos.h>
#endif
#if (HAVE_DIRECT_H)
#  include <direct.h>
#endif
#if (HAVE_SHARE_H)
#  include <share.h>
#endif
#if (LZO_CC_NDPC)
#  include <os.h>
#endif
#if defined(__TOS__) && (defined(__PUREC__) || defined(__TURBOC__))
#  include <ext.h>
#endif
#endif
#endif
#endif
#if defined(LZO_WANT_ACC_INCI_H)
#  undef LZO_WANT_ACC_INCI_H
#ifndef __LZO_INCI_H_INCLUDED
#define __LZO_INCI_H_INCLUDED 1
#if (LZO_LIBC_NAKED)
#elif (LZO_LIBC_FREESTANDING)
#elif (LZO_LIBC_MOSTLY_FREESTANDING)
#else
#if (LZO_OS_TOS && (LZO_CC_PUREC || LZO_CC_TURBOC))
#  include <tos.h>
#elif (LZO_HAVE_WINDOWS_H)
#  if 1 && !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN 1
#  endif
#  if 1 && !defined(_WIN32_WINNT)
#    define _WIN32_WINNT 0x0400
#  endif
#  include <windows.h>
#  if (LZO_CC_BORLANDC || LZO_CC_TURBOC)
#    include <dir.h>
#  endif
#elif (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_WIN16)
#  if (LZO_CC_AZTECC)
#    include <model.h>
#    include <stat.h>
#  elif (LZO_CC_BORLANDC || LZO_CC_TURBOC)
#    include <alloc.h>
#    include <dir.h>
#  elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#    include <sys/exceptn.h>
#  elif (LZO_CC_PACIFICC)
#    include <unixio.h>
#    include <stat.h>
#    include <sys.h>
#  elif (LZO_CC_WATCOMC)
#    include <i86.h>
#  endif
#elif (LZO_OS_OS216)
#  if (LZO_CC_WATCOMC)
#    include <i86.h>
#  endif
#endif
#if (HAVE_SYS_MMAN_H)
#  include <sys/mman.h>
#endif
#if (HAVE_SYS_RESOURCE_H)
#  include <sys/resource.h>
#endif
#if (LZO_OS_DOS16 || LZO_OS_OS216 || LZO_OS_WIN16)
#  if defined(FP_OFF)
#    define LZO_PTR_FP_OFF(x)   FP_OFF(x)
#  elif defined(_FP_OFF)
#    define LZO_PTR_FP_OFF(x)   _FP_OFF(x)
#  else
#    define LZO_PTR_FP_OFF(x)   (((const unsigned __far*)&(x))[0])
#  endif
#  if defined(FP_SEG)
#    define LZO_PTR_FP_SEG(x)   FP_SEG(x)
#  elif defined(_FP_SEG)
#    define LZO_PTR_FP_SEG(x)   _FP_SEG(x)
#  else
#    define LZO_PTR_FP_SEG(x)   (((const unsigned __far*)&(x))[1])
#  endif
#  if defined(MK_FP)
#    define LZO_PTR_MK_FP(s,o)  MK_FP(s,o)
#  elif defined(_MK_FP)
#    define LZO_PTR_MK_FP(s,o)  _MK_FP(s,o)
#  else
#    define LZO_PTR_MK_FP(s,o)  ((void __far*)(((unsigned long)(s)<<16)+(unsigned)(o)))
#  endif
#  if 0
#    undef LZO_PTR_FP_OFF
#    undef LZO_PTR_FP_SEG
#    undef LZO_PTR_MK_FP
#    define LZO_PTR_FP_OFF(x)   (((const unsigned __far*)&(x))[0])
#    define LZO_PTR_FP_SEG(x)   (((const unsigned __far*)&(x))[1])
#    define LZO_PTR_MK_FP(s,o)  ((void __far*)(((unsigned long)(s)<<16)+(unsigned)(o)))
#  endif
#endif
#endif
#endif
#endif
#if defined(LZO_WANT_ACC_LIB_H)
#  undef LZO_WANT_ACC_LIB_H
#ifndef __LZO_LIB_H_INCLUDED
#define __LZO_LIB_H_INCLUDED 1
#if !defined(__LZOLIB_FUNCNAME)
#  define __LZOLIB_FUNCNAME(f)  f
#endif
#if !defined(LZOLIB_EXTERN)
#  define LZOLIB_EXTERN(r,f)                extern r __LZOLIB_FUNCNAME(f)
#endif
#if !defined(LZOLIB_EXTERN_NOINLINE)
#  if defined(__lzo_noinline)
#    define LZOLIB_EXTERN_NOINLINE(r,f)     extern __lzo_noinline r __LZOLIB_FUNCNAME(f)
#  else
#    define LZOLIB_EXTERN_NOINLINE(r,f)     extern r __LZOLIB_FUNCNAME(f)
#  endif
#endif
#if (LZO_SIZEOF_LONG > LZO_SIZEOF_VOID_P)
#  define lzolib_handle_t       long
#else
#  define lzolib_handle_t       lzo_intptr_t
#endif
#if 0
LZOLIB_EXTERN(int, lzo_ascii_digit)   (int);
LZOLIB_EXTERN(int, lzo_ascii_islower) (int);
LZOLIB_EXTERN(int, lzo_ascii_isupper) (int);
LZOLIB_EXTERN(int, lzo_ascii_tolower) (int);
LZOLIB_EXTERN(int, lzo_ascii_toupper) (int);
LZOLIB_EXTERN(int, lzo_ascii_utolower) (int);
LZOLIB_EXTERN(int, lzo_ascii_utoupper) (int);
#endif
#define lzo_ascii_isdigit(c)    ((LZO_ICAST(unsigned, c) - 48) < 10)
#define lzo_ascii_islower(c)    ((LZO_ICAST(unsigned, c) - 97) < 26)
#define lzo_ascii_isupper(c)    ((LZO_ICAST(unsigned, c) - 65) < 26)
#define lzo_ascii_tolower(c)    (LZO_ICAST(int, c) + (lzo_ascii_isupper(c) << 5))
#define lzo_ascii_toupper(c)    (LZO_ICAST(int, c) - (lzo_ascii_islower(c) << 5))
#define lzo_ascii_utolower(c)   lzo_ascii_tolower(LZO_ITRUNC(unsigned char, c))
#define lzo_ascii_utoupper(c)   lzo_ascii_toupper(LZO_ITRUNC(unsigned char, c))
#ifndef lzo_hsize_t
#if (LZO_HAVE_MM_HUGE_PTR)
#  define lzo_hsize_t   unsigned long
#  define lzo_hvoid_p   void __huge *
#  define lzo_hchar_p   char __huge *
#  define lzo_hchar_pp  char __huge * __huge *
#  define lzo_hbyte_p   unsigned char __huge *
#else
#  define lzo_hsize_t   size_t
#  define lzo_hvoid_p   void *
#  define lzo_hchar_p   char *
#  define lzo_hchar_pp  char **
#  define lzo_hbyte_p   unsigned char *
#endif
#endif
LZOLIB_EXTERN(lzo_hvoid_p, lzo_halloc) (lzo_hsize_t);
LZOLIB_EXTERN(void, lzo_hfree) (lzo_hvoid_p);
#if (LZO_OS_DOS16 || LZO_OS_OS216)
LZOLIB_EXTERN(void __far*, lzo_dos_alloc) (unsigned long);
LZOLIB_EXTERN(int, lzo_dos_free) (void __far*);
#endif
LZOLIB_EXTERN(int, lzo_hmemcmp) (const lzo_hvoid_p, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemcpy) (lzo_hvoid_p, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemmove) (lzo_hvoid_p, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemset) (lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrlen) (const lzo_hchar_p);
LZOLIB_EXTERN(int, lzo_hstrcmp) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(int, lzo_hstrncmp)(const lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(int, lzo_ascii_hstricmp) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(int, lzo_ascii_hstrnicmp)(const lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(int, lzo_ascii_hmemicmp) (const lzo_hvoid_p, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrstr) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_ascii_hstristr) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemmem) (const lzo_hvoid_p, lzo_hsize_t, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_ascii_hmemimem) (const lzo_hvoid_p, lzo_hsize_t, const lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrcpy) (lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrcat) (lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrlcpy) (lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrlcat) (lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(int, lzo_hstrscpy) (lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(int, lzo_hstrscat) (lzo_hchar_p, const lzo_hchar_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrccpy) (lzo_hchar_p, const lzo_hchar_p, int);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemccpy) (lzo_hvoid_p, const lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrchr)  (const lzo_hchar_p, int);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrrchr) (const lzo_hchar_p, int);
LZOLIB_EXTERN(lzo_hchar_p, lzo_ascii_hstrichr) (const lzo_hchar_p, int);
LZOLIB_EXTERN(lzo_hchar_p, lzo_ascii_hstrrichr) (const lzo_hchar_p, int);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemchr)  (const lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_hmemrchr) (const lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_ascii_hmemichr) (const lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_ascii_hmemrichr) (const lzo_hvoid_p, int, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrspn)  (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrrspn) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrcspn)  (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hstrrcspn) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrpbrk)  (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrrpbrk) (const lzo_hchar_p, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrsep)  (lzo_hchar_pp, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_hstrrsep) (lzo_hchar_pp, const lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_ascii_hstrlwr) (lzo_hchar_p);
LZOLIB_EXTERN(lzo_hchar_p, lzo_ascii_hstrupr) (lzo_hchar_p);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_ascii_hmemlwr) (lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hvoid_p, lzo_ascii_hmemupr) (lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hfread) (void *, lzo_hvoid_p, lzo_hsize_t);
LZOLIB_EXTERN(lzo_hsize_t, lzo_hfwrite) (void *, const lzo_hvoid_p, lzo_hsize_t);
#if (LZO_HAVE_MM_HUGE_PTR)
LZOLIB_EXTERN(long, lzo_hread) (int, lzo_hvoid_p, long);
LZOLIB_EXTERN(long, lzo_hwrite) (int, const lzo_hvoid_p, long);
#endif
LZOLIB_EXTERN(long, lzo_safe_hread) (int, lzo_hvoid_p, long);
LZOLIB_EXTERN(long, lzo_safe_hwrite) (int, const lzo_hvoid_p, long);
LZOLIB_EXTERN(unsigned, lzo_ua_get_be16) (const lzo_hvoid_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_ua_get_be24) (const lzo_hvoid_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_ua_get_be32) (const lzo_hvoid_p);
LZOLIB_EXTERN(void, lzo_ua_set_be16) (lzo_hvoid_p, unsigned);
LZOLIB_EXTERN(void, lzo_ua_set_be24) (lzo_hvoid_p, lzo_uint32l_t);
LZOLIB_EXTERN(void, lzo_ua_set_be32) (lzo_hvoid_p, lzo_uint32l_t);
LZOLIB_EXTERN(unsigned, lzo_ua_get_le16) (const lzo_hvoid_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_ua_get_le24) (const lzo_hvoid_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_ua_get_le32) (const lzo_hvoid_p);
LZOLIB_EXTERN(void, lzo_ua_set_le16) (lzo_hvoid_p, unsigned);
LZOLIB_EXTERN(void, lzo_ua_set_le24) (lzo_hvoid_p, lzo_uint32l_t);
LZOLIB_EXTERN(void, lzo_ua_set_le32) (lzo_hvoid_p, lzo_uint32l_t);
#if defined(lzo_int64l_t)
LZOLIB_EXTERN(lzo_uint64l_t, lzo_ua_get_be64) (const lzo_hvoid_p);
LZOLIB_EXTERN(void, lzo_ua_set_be64) (lzo_hvoid_p, lzo_uint64l_t);
LZOLIB_EXTERN(lzo_uint64l_t, lzo_ua_get_le64) (const lzo_hvoid_p);
LZOLIB_EXTERN(void, lzo_ua_set_le64) (lzo_hvoid_p, lzo_uint64l_t);
#endif
LZOLIB_EXTERN_NOINLINE(short, lzo_vget_short) (short, int);
LZOLIB_EXTERN_NOINLINE(int, lzo_vget_int) (int, int);
LZOLIB_EXTERN_NOINLINE(long, lzo_vget_long) (long, int);
#if defined(lzo_int64l_t)
LZOLIB_EXTERN_NOINLINE(lzo_int64l_t, lzo_vget_lzo_int64l_t) (lzo_int64l_t, int);
#endif
LZOLIB_EXTERN_NOINLINE(lzo_hsize_t, lzo_vget_lzo_hsize_t) (lzo_hsize_t, int);
#if !(LZO_CFG_NO_FLOAT)
LZOLIB_EXTERN_NOINLINE(float, lzo_vget_float) (float, int);
#endif
#if !(LZO_CFG_NO_DOUBLE)
LZOLIB_EXTERN_NOINLINE(double, lzo_vget_double) (double, int);
#endif
LZOLIB_EXTERN_NOINLINE(lzo_hvoid_p, lzo_vget_lzo_hvoid_p) (lzo_hvoid_p, int);
LZOLIB_EXTERN_NOINLINE(const lzo_hvoid_p, lzo_vget_lzo_hvoid_cp) (const lzo_hvoid_p, int);
#if !defined(LZO_FN_PATH_MAX)
#if (LZO_OS_DOS16 || LZO_OS_WIN16)
#  define LZO_FN_PATH_MAX   143
#elif (LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_WIN32 || LZO_OS_WIN64)
#  define LZO_FN_PATH_MAX   259
#elif (LZO_OS_TOS)
#  define LZO_FN_PATH_MAX   259
#endif
#endif
#if !defined(LZO_FN_PATH_MAX)
#  define LZO_FN_PATH_MAX   1023
#endif
#if !defined(LZO_FN_NAME_MAX)
#if (LZO_OS_DOS16 || LZO_OS_WIN16)
#  define LZO_FN_NAME_MAX   12
#elif (LZO_ARCH_M68K && LZO_OS_TOS && (LZO_CC_PUREC || LZO_CC_TURBOC))
#  define LZO_FN_NAME_MAX   12
#elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#elif (LZO_OS_DOS32)
#  define LZO_FN_NAME_MAX   12
#endif
#endif
#if !defined(LZO_FN_NAME_MAX)
#  define LZO_FN_NAME_MAX   LZO_FN_PATH_MAX
#endif
#define LZO_FNMATCH_NOESCAPE        1
#define LZO_FNMATCH_PATHNAME        2
#define LZO_FNMATCH_PATHSTAR        4
#define LZO_FNMATCH_PERIOD          8
#define LZO_FNMATCH_ASCII_CASEFOLD  16
LZOLIB_EXTERN(int, lzo_fnmatch) (const lzo_hchar_p, const lzo_hchar_p, int);
#undef __LZOLIB_USE_OPENDIR
#if (HAVE_DIRENT_H || LZO_CC_WATCOMC)
#  define __LZOLIB_USE_OPENDIR 1
#  if (LZO_OS_DOS32 && defined(__BORLANDC__))
#  elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#  elif (LZO_OS_OS2 || LZO_OS_OS216)
#  elif (LZO_ARCH_M68K && LZO_OS_TOS && LZO_CC_GNUC)
#  elif (LZO_OS_WIN32 && !(LZO_HAVE_WINDOWS_H))
#  elif (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_TOS || LZO_OS_WIN16 || LZO_OS_WIN32 || LZO_OS_WIN64)
#    undef __LZOLIB_USE_OPENDIR
#  endif
#endif
typedef struct
{
#if defined(__LZOLIB_USE_OPENDIR)
    void* u_dirp;
# if (LZO_CC_WATCOMC)
    unsigned short f_time;
    unsigned short f_date;
    unsigned long f_size;
# endif
    char f_name[LZO_FN_NAME_MAX+1];
#elif (LZO_OS_WIN32 || LZO_OS_WIN64)
    lzolib_handle_t u_handle;
    unsigned f_attr;
    unsigned f_size_low;
    unsigned f_size_high;
    char f_name[LZO_FN_NAME_MAX+1];
#elif (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_TOS || LZO_OS_WIN16)
    char u_dta[21];
    unsigned char f_attr;
    unsigned short f_time;
    unsigned short f_date;
    unsigned short f_size_low;
    unsigned short f_size_high;
    char f_name[LZO_FN_NAME_MAX+1];
    char u_dirp;
#else
    void* u_dirp;
    char f_name[LZO_FN_NAME_MAX+1];
#endif
} lzo_dir_t;
#ifndef lzo_dir_p
#define lzo_dir_p lzo_dir_t *
#endif
LZOLIB_EXTERN(int, lzo_opendir)  (lzo_dir_p, const char*);
LZOLIB_EXTERN(int, lzo_readdir)  (lzo_dir_p);
LZOLIB_EXTERN(int, lzo_closedir) (lzo_dir_p);
#if (LZO_CC_GNUC) && (defined(__CYGWIN__) || defined(__MINGW32__))
#  define lzo_alloca(x)     __builtin_alloca((x))
#elif (LZO_CC_GNUC) && (LZO_OS_CONSOLE_PS2)
#  define lzo_alloca(x)     __builtin_alloca((x))
#elif (LZO_CC_BORLANDC || LZO_CC_LCC) && defined(__linux__)
#elif (HAVE_ALLOCA)
#  define lzo_alloca(x)     LZO_STATIC_CAST(void *, alloca((x)))
#endif
#if (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
#  define lzo_stackavail()  stackavail()
#elif (LZO_ARCH_I086 && LZO_CC_BORLANDC && (__BORLANDC__ >= 0x0410))
#  define lzo_stackavail()  stackavail()
#elif (LZO_ARCH_I086 && LZO_CC_BORLANDC && (__BORLANDC__ >= 0x0400))
#  if (LZO_OS_WIN16) && (LZO_MM_TINY || LZO_MM_SMALL || LZO_MM_MEDIUM)
#  else
#    define lzo_stackavail()  stackavail()
#  endif
#elif ((LZO_ARCH_I086 || LZO_ARCH_I386) && (LZO_CC_DMC || LZO_CC_SYMANTECC))
#  define lzo_stackavail()  stackavail()
#elif ((LZO_ARCH_I086) && LZO_CC_MSC && (_MSC_VER >= 700))
#  define lzo_stackavail()  _stackavail()
#elif ((LZO_ARCH_I086) && LZO_CC_MSC)
#  define lzo_stackavail()  stackavail()
#elif ((LZO_ARCH_I086 || LZO_ARCH_I386) && LZO_CC_TURBOC && (__TURBOC__ >= 0x0450))
#  define lzo_stackavail()  stackavail()
#elif (LZO_ARCH_I086 && LZO_CC_TURBOC && (__TURBOC__ >= 0x0400))
   LZO_EXTERN_C size_t __cdecl stackavail(void);
#  define lzo_stackavail()  stackavail()
#elif ((LZO_ARCH_I086 || LZO_ARCH_I386) && (LZO_CC_WATCOMC))
#  define lzo_stackavail()  stackavail()
#elif (LZO_ARCH_I086 && LZO_CC_ZORTECHC)
#  define lzo_stackavail()  _chkstack()
#endif
LZOLIB_EXTERN(lzo_intptr_t, lzo_get_osfhandle) (int);
LZOLIB_EXTERN(const char *, lzo_getenv) (const char *);
LZOLIB_EXTERN(int, lzo_isatty) (int);
LZOLIB_EXTERN(int, lzo_mkdir) (const char*, unsigned);
LZOLIB_EXTERN(int, lzo_rmdir) (const char*);
LZOLIB_EXTERN(int, lzo_response) (int*, char***);
LZOLIB_EXTERN(int, lzo_set_binmode) (int, int);
#if defined(lzo_int32e_t)
LZOLIB_EXTERN(lzo_int32e_t, lzo_muldiv32s) (lzo_int32e_t, lzo_int32e_t, lzo_int32e_t);
LZOLIB_EXTERN(lzo_uint32e_t, lzo_muldiv32u) (lzo_uint32e_t, lzo_uint32e_t, lzo_uint32e_t);
#endif
LZOLIB_EXTERN(void, lzo_wildargv) (int*, char***);
LZOLIB_EXTERN_NOINLINE(void, lzo_debug_break) (void);
LZOLIB_EXTERN_NOINLINE(void, lzo_debug_nop) (void);
LZOLIB_EXTERN_NOINLINE(int, lzo_debug_align_check_query) (void);
LZOLIB_EXTERN_NOINLINE(int, lzo_debug_align_check_enable) (int);
LZOLIB_EXTERN_NOINLINE(unsigned, lzo_debug_running_on_qemu) (void);
LZOLIB_EXTERN_NOINLINE(unsigned, lzo_debug_running_on_valgrind) (void);
#if defined(lzo_int32e_t)
LZOLIB_EXTERN(int, lzo_tsc_read) (lzo_uint32e_t*);
#endif
struct lzo_pclock_handle_t;
struct lzo_pclock_t;
typedef struct lzo_pclock_handle_t lzo_pclock_handle_t;
typedef struct lzo_pclock_t lzo_pclock_t;
#ifndef lzo_pclock_handle_p
#define lzo_pclock_handle_p lzo_pclock_handle_t *
#endif
#ifndef lzo_pclock_p
#define lzo_pclock_p lzo_pclock_t *
#endif
#define LZO_PCLOCK_REALTIME             0
#define LZO_PCLOCK_MONOTONIC            1
#define LZO_PCLOCK_PROCESS_CPUTIME_ID   2
#define LZO_PCLOCK_THREAD_CPUTIME_ID    3
typedef int (*lzo_pclock_gettime_t) (lzo_pclock_handle_p, lzo_pclock_p);
struct lzo_pclock_handle_t {
    lzolib_handle_t h;
    int mode;
    int read_error;
    const char* name;
    lzo_pclock_gettime_t gettime;
#if defined(lzo_int64l_t)
    lzo_uint64l_t ticks_base;
#endif
};
struct lzo_pclock_t {
#if defined(lzo_int64l_t)
    lzo_int64l_t tv_sec;
#else
    lzo_int32l_t tv_sec_high;
    lzo_uint32l_t tv_sec_low;
#endif
    lzo_uint32l_t tv_nsec;
};
LZOLIB_EXTERN(int, lzo_pclock_open)  (lzo_pclock_handle_p, int);
LZOLIB_EXTERN(int, lzo_pclock_open_default) (lzo_pclock_handle_p);
LZOLIB_EXTERN(int, lzo_pclock_close) (lzo_pclock_handle_p);
LZOLIB_EXTERN(void, lzo_pclock_read) (lzo_pclock_handle_p, lzo_pclock_p);
#if !(LZO_CFG_NO_DOUBLE)
LZOLIB_EXTERN(double, lzo_pclock_get_elapsed) (lzo_pclock_handle_p, const lzo_pclock_p, const lzo_pclock_p);
#endif
LZOLIB_EXTERN(int, lzo_pclock_flush_cpu_cache) (lzo_pclock_handle_p, unsigned);
struct lzo_getopt_t;
typedef struct lzo_getopt_t lzo_getopt_t;
#ifndef lzo_getopt_p
#define lzo_getopt_p lzo_getopt_t *
#endif
struct lzo_getopt_longopt_t;
typedef struct lzo_getopt_longopt_t lzo_getopt_longopt_t;
#ifndef lzo_getopt_longopt_p
#define lzo_getopt_longopt_p lzo_getopt_longopt_t *
#endif
struct lzo_getopt_longopt_t {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};
typedef void (*lzo_getopt_opterr_t)(lzo_getopt_p, const char*, void *);
struct lzo_getopt_t {
    void *user;
    const char *progname;
    int bad_option;
    char *optarg;
    lzo_getopt_opterr_t opterr;
    int optind;
    int optopt;
    int errcount;
    int argc; char** argv;
    int eof; int shortpos;
    int pending_rotate_first, pending_rotate_middle;
};
enum { LZO_GETOPT_NO_ARG, LZO_GETOPT_REQUIRED_ARG, LZO_GETOPT_OPTIONAL_ARG, LZO_GETOPT_EXACT_ARG = 0x10 };
enum { LZO_GETOPT_PERMUTE, LZO_GETOPT_RETURN_IN_ORDER, LZO_GETOPT_REQUIRE_ORDER };
LZOLIB_EXTERN(void, lzo_getopt_init) (lzo_getopt_p g,
                                      int start_argc, int argc, char** argv);
LZOLIB_EXTERN(int, lzo_getopt) (lzo_getopt_p g,
                                const char* shortopts,
                                const lzo_getopt_longopt_p longopts,
                                int* longind);
typedef struct {
    lzo_uint32l_t seed;
} lzo_rand31_t;
#ifndef lzo_rand31_p
#define lzo_rand31_p lzo_rand31_t *
#endif
LZOLIB_EXTERN(void, lzo_srand31) (lzo_rand31_p, lzo_uint32l_t);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_rand31) (lzo_rand31_p);
#if defined(lzo_int64l_t)
typedef struct {
    lzo_uint64l_t seed;
} lzo_rand48_t;
#ifndef lzo_rand48_p
#define lzo_rand48_p lzo_rand48_t *
#endif
LZOLIB_EXTERN(void, lzo_srand48) (lzo_rand48_p, lzo_uint32l_t);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_rand48) (lzo_rand48_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_rand48_r32) (lzo_rand48_p);
#endif
#if defined(lzo_int64l_t)
typedef struct {
    lzo_uint64l_t seed;
} lzo_rand64_t;
#ifndef lzo_rand64_p
#define lzo_rand64_p lzo_rand64_t *
#endif
LZOLIB_EXTERN(void, lzo_srand64) (lzo_rand64_p, lzo_uint64l_t);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_rand64) (lzo_rand64_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_rand64_r32) (lzo_rand64_p);
#endif
typedef struct {
    unsigned n;
    lzo_uint32l_t s[624];
} lzo_randmt_t;
#ifndef lzo_randmt_p
#define lzo_randmt_p lzo_randmt_t *
#endif
LZOLIB_EXTERN(void, lzo_srandmt) (lzo_randmt_p, lzo_uint32l_t);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_randmt) (lzo_randmt_p);
LZOLIB_EXTERN(lzo_uint32l_t, lzo_randmt_r32) (lzo_randmt_p);
#if defined(lzo_int64l_t)
typedef struct {
    unsigned n;
    lzo_uint64l_t s[312];
} lzo_randmt64_t;
#ifndef lzo_randmt64_p
#define lzo_randmt64_p lzo_randmt64_t *
#endif
LZOLIB_EXTERN(void, lzo_srandmt64) (lzo_randmt64_p, lzo_uint64l_t);
LZOLIB_EXTERN(lzo_uint64l_t, lzo_randmt64_r64) (lzo_randmt64_p);
#endif
#define LZO_SPAWN_P_WAIT    0
#define LZO_SPAWN_P_NOWAIT  1
LZOLIB_EXTERN(int, lzo_spawnv)  (int mode, const char* fn, const char* const * argv);
LZOLIB_EXTERN(int, lzo_spawnvp) (int mode, const char* fn, const char* const * argv);
LZOLIB_EXTERN(int, lzo_spawnve) (int mode, const char* fn, const char* const * argv, const char * const envp);
#endif
#endif
#if defined(LZO_WANT_ACC_CXX_H)
#  undef LZO_WANT_ACC_CXX_H
#ifndef __LZO_CXX_H_INCLUDED
#define __LZO_CXX_H_INCLUDED 1
#if defined(__cplusplus)
#if defined(LZO_CXX_NOTHROW)
#elif (LZO_CC_GNUC && (LZO_CC_GNUC < 0x020800ul))
#elif (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0450))
#elif (LZO_CC_GHS && !defined(__EXCEPTIONS))
#elif (LZO_CC_HIGHC)
#elif (LZO_CC_MSC && (_MSC_VER < 1100))
#elif (LZO_CC_NDPC)
#elif (LZO_CC_TURBOC)
#elif (LZO_CC_WATCOMC && !defined(_CPPUNWIND))
#elif (LZO_CC_ZORTECHC)
#else
#  define LZO_CXX_NOTHROW           throw()
#endif
#if !defined(LZO_CXX_NOTHROW)
#  define LZO_CXX_NOTHROW           /*empty*/
#endif
#if defined(__LZO_CXX_DO_NEW)
#elif (LZO_CC_GHS || LZO_CC_NDPC || LZO_CC_PGI)
#  define __LZO_CXX_DO_NEW          { return 0; }
#elif ((LZO_CC_BORLANDC || LZO_CC_TURBOC) && LZO_ARCH_I086)
#  define __LZO_CXX_DO_NEW          { return 0; }
#else
#  define __LZO_CXX_DO_NEW          ;
#endif
#if defined(__LZO_CXX_DO_DELETE)
#elif (LZO_CC_BORLANDC || LZO_CC_TURBOC)
#  define __LZO_CXX_DO_DELETE       { }
#else
#  define __LZO_CXX_DO_DELETE       LZO_CXX_NOTHROW { }
#endif
#if (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0450))
#elif (LZO_CC_MSC && LZO_MM_HUGE)
#  define LZO_CXX_DISABLE_NEW_DELETE private:
#elif (LZO_CC_MSC && (_MSC_VER < 1100))
#elif (LZO_CC_NDPC)
#elif (LZO_CC_SYMANTECC || LZO_CC_ZORTECHC)
#elif (LZO_CC_TURBOC)
#elif (LZO_CC_WATCOMC && (__WATCOMC__ < 1100))
#else
#  define __LZO_CXX_HAVE_ARRAY_NEW 1
#endif
#if (__LZO_CXX_HAVE_ARRAY_NEW)
#  define __LZO_CXX_HAVE_PLACEMENT_NEW 1
#endif
#if (__LZO_CXX_HAVE_PLACEMENT_NEW)
#  if (LZO_CC_GNUC >= 0x030000ul)
#    define __LZO_CXX_HAVE_PLACEMENT_DELETE 1
#  elif (LZO_CC_INTELC)
#    define __LZO_CXX_HAVE_PLACEMENT_DELETE 1
#  elif (LZO_CC_MSC && (_MSC_VER >= 1200))
#    define __LZO_CXX_HAVE_PLACEMENT_DELETE 1
#  elif (LZO_CC_CLANG || LZO_CC_LLVM || LZO_CC_PATHSCALE)
#    define __LZO_CXX_HAVE_PLACEMENT_DELETE 1
#  elif (LZO_CC_PGI)
#    define __LZO_CXX_HAVE_PLACEMENT_DELETE 1
#  endif
#endif
#if defined(LZO_CXX_DISABLE_NEW_DELETE)
#elif defined(new) || defined(delete)
#  define LZO_CXX_DISABLE_NEW_DELETE private:
#elif (LZO_CC_GNUC && (LZO_CC_GNUC < 0x025b00ul))
#  define LZO_CXX_DISABLE_NEW_DELETE private:
#elif  (LZO_CC_HIGHC)
#  define LZO_CXX_DISABLE_NEW_DELETE private:
#elif !(__LZO_CXX_HAVE_ARRAY_NEW)
#  define LZO_CXX_DISABLE_NEW_DELETE \
        protected: static void operator delete(void*) __LZO_CXX_DO_DELETE \
        protected: static void* operator new(size_t) __LZO_CXX_DO_NEW \
        private:
#else
#  define LZO_CXX_DISABLE_NEW_DELETE \
        protected: static void operator delete(void*) __LZO_CXX_DO_DELETE \
                   static void operator delete[](void*) __LZO_CXX_DO_DELETE \
        private:   static void* operator new(size_t)  __LZO_CXX_DO_NEW \
                   static void* operator new[](size_t) __LZO_CXX_DO_NEW
#endif
#if defined(LZO_CXX_TRIGGER_FUNCTION)
#else
#  define LZO_CXX_TRIGGER_FUNCTION \
        protected: virtual const void* lzo_cxx_trigger_function() const; \
        private:
#endif
#if defined(LZO_CXX_TRIGGER_FUNCTION_IMPL)
#else
#  define LZO_CXX_TRIGGER_FUNCTION_IMPL(klass) \
        const void* klass::lzo_cxx_trigger_function() const { return LZO_STATIC_CAST(const void *, 0); }
#endif
#endif
#endif
#endif
#if defined(LZO_WANT_ACC_CHK_CH)
#  undef LZO_WANT_ACC_CHK_CH
#if !defined(LZOCHK_ASSERT)
#  define LZOCHK_ASSERT(expr)   LZO_COMPILE_TIME_ASSERT_HEADER(expr)
#endif
#if !defined(LZOCHK_ASSERT_SIGN_T)
#  define LZOCHK_ASSERT_SIGN_T(type,relop) \
        LZOCHK_ASSERT( LZO_STATIC_CAST(type, -1)  relop  LZO_STATIC_CAST(type, 0)) \
        LZOCHK_ASSERT( LZO_STATIC_CAST(type, ~LZO_STATIC_CAST(type, 0)) relop  LZO_STATIC_CAST(type, 0)) \
        LZOCHK_ASSERT( LZO_STATIC_CAST(type, ~LZO_STATIC_CAST(type, 0)) ==     LZO_STATIC_CAST(type, -1))
#endif
#if !defined(LZOCHK_ASSERT_IS_SIGNED_T)
#  define LZOCHK_ASSERT_IS_SIGNED_T(type)       LZOCHK_ASSERT_SIGN_T(type,<)
#endif
#if !defined(LZOCHK_ASSERT_IS_UNSIGNED_T)
#  if (LZO_BROKEN_INTEGRAL_PROMOTION)
#    define LZOCHK_ASSERT_IS_UNSIGNED_T(type) \
        LZOCHK_ASSERT( LZO_STATIC_CAST(type, -1) > LZO_STATIC_CAST(type, 0) )
#  else
#    define LZOCHK_ASSERT_IS_UNSIGNED_T(type)   LZOCHK_ASSERT_SIGN_T(type,>)
#  endif
#endif
#if defined(LZOCHK_CFG_PEDANTIC)
#if (LZO_CC_BORLANDC && (__BORLANDC__ >= 0x0550) && (__BORLANDC__ < 0x0560))
#  pragma option push -w-8055
#elif (LZO_CC_BORLANDC && (__BORLANDC__ >= 0x0530) && (__BORLANDC__ < 0x0550))
#  pragma option push -w-osh
#endif
#endif
#if (LZO_0xffffffffL - LZO_UINT32_C(4294967294) != 1)
#  error "preprocessor error"
#endif
#if (LZO_0xffffffffL - LZO_UINT32_C(0xfffffffd) != 2)
#  error "preprocessor error"
#endif
#if +0
#  error "preprocessor error"
#endif
#if -0
#  error "preprocessor error"
#endif
#if +0 != 0
#  error "preprocessor error"
#endif
#if -0 != 0
#  error "preprocessor error"
#endif
#define LZOCHK_VAL  1
#define LZOCHK_TMP1 LZOCHK_VAL
#undef LZOCHK_VAL
#define LZOCHK_VAL  2
#define LZOCHK_TMP2 LZOCHK_VAL
#if (LZOCHK_TMP1 != 2)
#  error "preprocessor error 3a"
#endif
#if (LZOCHK_TMP2 != 2)
#  error "preprocessor error 3b"
#endif
#undef LZOCHK_VAL
#if (LZOCHK_TMP2)
#  error "preprocessor error 3c"
#endif
#if (LZOCHK_TMP2 + 0 != 0)
#  error "preprocessor error 3d"
#endif
#undef LZOCHK_TMP1
#undef LZOCHK_TMP2
#if 0 || defined(LZOCHK_CFG_PEDANTIC)
#  if (LZO_ARCH_MIPS) && defined(_MIPS_SZINT)
    LZOCHK_ASSERT((_MIPS_SZINT) == 8 * sizeof(int))
#  endif
#  if (LZO_ARCH_MIPS) && defined(_MIPS_SZLONG)
    LZOCHK_ASSERT((_MIPS_SZLONG) == 8 * sizeof(long))
#  endif
#  if (LZO_ARCH_MIPS) && defined(_MIPS_SZPTR)
    LZOCHK_ASSERT((_MIPS_SZPTR) == 8 * sizeof(void *))
#  endif
#endif
    LZOCHK_ASSERT(1 == 1)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,1) == 1u)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,2) == 3u)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,3) == 7u)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,8) == 255u)
#if (LZO_SIZEOF_INT >= 2)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1,15) == 32767)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,16) == 0xffffU)
    LZOCHK_ASSERT(__LZO_MASK_GEN(0u,16) == 0u)
#endif
    LZOCHK_ASSERT(__LZO_MASK_GEN(1ul,16) == 0xffffUL)
    LZOCHK_ASSERT(__LZO_MASK_GEN(0ul,16) == 0ul)
#if (LZO_SIZEOF_INT >= 4)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1,31) == 2147483647)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,32) == 0xffffffffU)
    LZOCHK_ASSERT(__LZO_MASK_GEN(0u,32) == 0u)
#endif
#if (LZO_SIZEOF_LONG >= 4)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1ul,32) == 0xffffffffUL)
    LZOCHK_ASSERT(__LZO_MASK_GEN(0ul,32) == 0ul)
#endif
#if (LZO_SIZEOF_LONG >= 8)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1ul,64) == 0xffffffffffffffffUL)
    LZOCHK_ASSERT(__LZO_MASK_GEN(0ul,64) == 0ul)
#endif
#if !(LZO_BROKEN_INTEGRAL_PROMOTION)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1u,LZO_SIZEOF_INT*8) == ~0u)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1ul,LZO_SIZEOF_LONG*8) == ~0ul)
#endif
#if 1
    LZOCHK_ASSERT(__LZO_MASK_GEN(0,0) == 0)
    LZOCHK_ASSERT(__LZO_MASK_GEN(1,0) == 0)
    LZOCHK_ASSERT(__LZO_MASK_GEN(2,0) == 0)
    LZOCHK_ASSERT(__LZO_MASK_GEN(4,0) == 0)
#endif
#if 1
    LZOCHK_ASSERT(__LZO_MASK_GEN(2,1) == 2)
    LZOCHK_ASSERT(__LZO_MASK_GEN(4,1) == 4)
    LZOCHK_ASSERT(__LZO_MASK_GEN(8,1) == 8)
    LZOCHK_ASSERT(__LZO_MASK_GEN(2,2) == 2+4)
    LZOCHK_ASSERT(__LZO_MASK_GEN(4,2) == 4+8)
    LZOCHK_ASSERT(__LZO_MASK_GEN(8,2) == 8+16)
    LZOCHK_ASSERT(__LZO_MASK_GEN(2,3) == 2+4+8)
    LZOCHK_ASSERT(__LZO_MASK_GEN(4,3) == 4+8+16)
    LZOCHK_ASSERT(__LZO_MASK_GEN(8,3) == 8+16+32)
    LZOCHK_ASSERT(__LZO_MASK_GEN(7,1) == 7)
    LZOCHK_ASSERT(__LZO_MASK_GEN(7,2) == 7+14)
    LZOCHK_ASSERT(__LZO_MASK_GEN(7,3) == 7+14+28)
#endif
#if !(LZO_BROKEN_SIGNED_RIGHT_SHIFT)
    LZOCHK_ASSERT(((-1) >> 7) == -1)
#endif
    LZOCHK_ASSERT(((1)  >> 7) == 0)
#if (LZO_CC_INTELC && (__INTEL_COMPILER >= 900))
#  pragma warning(push)
#  pragma warning(disable: 1025)
#endif
    LZOCHK_ASSERT((~0l  & ~0)  == ~0l)
    LZOCHK_ASSERT((~0l  & ~0u) == ~0u)
    LZOCHK_ASSERT((~0ul & ~0)  == ~0ul)
    LZOCHK_ASSERT((~0ul & ~0u) == ~0u)
#if defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0150)
#elif (LZO_SIZEOF_INT == 2)
    LZOCHK_ASSERT((~0l  & ~0u) == 0xffffU)
    LZOCHK_ASSERT((~0ul & ~0u) == 0xffffU)
#elif (LZO_SIZEOF_INT == 4)
    LZOCHK_ASSERT((~0l  & ~0u) == 0xffffffffU)
    LZOCHK_ASSERT((~0ul & ~0u) == 0xffffffffU)
#endif
#if (LZO_CC_INTELC && (__INTEL_COMPILER >= 900))
#  pragma warning(pop)
#endif
    LZOCHK_ASSERT_IS_SIGNED_T(signed char)
    LZOCHK_ASSERT_IS_UNSIGNED_T(unsigned char)
    LZOCHK_ASSERT(sizeof(signed char) == sizeof(char))
    LZOCHK_ASSERT(sizeof(unsigned char) == sizeof(char))
    LZOCHK_ASSERT(sizeof(char) == 1)
#if (LZO_CC_CILLY) && (!defined(__CILLY__) || (__CILLY__ < 0x010302L))
#else
    LZOCHK_ASSERT(sizeof(char) == sizeof(LZO_STATIC_CAST(char, 0)))
#endif
#if defined(__cplusplus)
    LZOCHK_ASSERT(sizeof('\0') == sizeof(char))
#else
#  if (LZO_CC_DMC)
#  else
    LZOCHK_ASSERT(sizeof('\0') == sizeof(int))
#  endif
#endif
#if defined(__lzo_alignof)
    LZOCHK_ASSERT(__lzo_alignof(char) == 1)
    LZOCHK_ASSERT(__lzo_alignof(signed char) == 1)
    LZOCHK_ASSERT(__lzo_alignof(unsigned char) == 1)
#if defined(lzo_int16e_t)
    LZOCHK_ASSERT(__lzo_alignof(lzo_int16e_t) >= 1)
    LZOCHK_ASSERT(__lzo_alignof(lzo_int16e_t) <= 2)
#endif
#if defined(lzo_int32e_t)
    LZOCHK_ASSERT(__lzo_alignof(lzo_int32e_t) >= 1)
    LZOCHK_ASSERT(__lzo_alignof(lzo_int32e_t) <= 4)
#endif
#endif
    LZOCHK_ASSERT_IS_SIGNED_T(short)
    LZOCHK_ASSERT_IS_UNSIGNED_T(unsigned short)
    LZOCHK_ASSERT(sizeof(short) == sizeof(unsigned short))
#if !(LZO_ABI_I8LP16)
    LZOCHK_ASSERT(sizeof(short) >= 2)
#endif
    LZOCHK_ASSERT(sizeof(short) >= sizeof(char))
#if (LZO_CC_CILLY) && (!defined(__CILLY__) || (__CILLY__ < 0x010302L))
#else
    LZOCHK_ASSERT(sizeof(short) == sizeof(LZO_STATIC_CAST(short, 0)))
#endif
#if (LZO_SIZEOF_SHORT > 0)
    LZOCHK_ASSERT(sizeof(short) == LZO_SIZEOF_SHORT)
#endif
    LZOCHK_ASSERT_IS_SIGNED_T(int)
    LZOCHK_ASSERT_IS_UNSIGNED_T(unsigned int)
    LZOCHK_ASSERT(sizeof(int) == sizeof(unsigned int))
#if !(LZO_ABI_I8LP16)
    LZOCHK_ASSERT(sizeof(int) >= 2)
#endif
    LZOCHK_ASSERT(sizeof(int) >= sizeof(short))
    LZOCHK_ASSERT(sizeof(int) == sizeof(0))
    LZOCHK_ASSERT(sizeof(int) == sizeof(LZO_STATIC_CAST(int, 0)))
#if (LZO_SIZEOF_INT > 0)
    LZOCHK_ASSERT(sizeof(int) == LZO_SIZEOF_INT)
#endif
    LZOCHK_ASSERT(sizeof(0) == sizeof(int))
    LZOCHK_ASSERT_IS_SIGNED_T(long)
    LZOCHK_ASSERT_IS_UNSIGNED_T(unsigned long)
    LZOCHK_ASSERT(sizeof(long) == sizeof(unsigned long))
#if !(LZO_ABI_I8LP16)
    LZOCHK_ASSERT(sizeof(long) >= 4)
#endif
    LZOCHK_ASSERT(sizeof(long) >= sizeof(int))
    LZOCHK_ASSERT(sizeof(long) == sizeof(0L))
    LZOCHK_ASSERT(sizeof(long) == sizeof(LZO_STATIC_CAST(long, 0)))
#if (LZO_SIZEOF_LONG > 0)
    LZOCHK_ASSERT(sizeof(long) == LZO_SIZEOF_LONG)
#endif
    LZOCHK_ASSERT(sizeof(0L) == sizeof(long))
    LZOCHK_ASSERT_IS_UNSIGNED_T(size_t)
    LZOCHK_ASSERT(sizeof(size_t) >= sizeof(int))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(sizeof(0)))
#if (LZO_SIZEOF_SIZE_T > 0)
    LZOCHK_ASSERT(sizeof(size_t) == LZO_SIZEOF_SIZE_T)
#endif
    LZOCHK_ASSERT_IS_SIGNED_T(ptrdiff_t)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(int))
    LZOCHK_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t))
#if !(LZO_BROKEN_SIZEOF)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(LZO_STATIC_CAST(char*, 0) - LZO_STATIC_CAST(char*, 0)))
# if (LZO_HAVE_MM_HUGE_PTR)
    LZOCHK_ASSERT(4 == sizeof(LZO_STATIC_CAST(char __huge*, 0) - LZO_STATIC_CAST(char __huge*, 0)))
# endif
#endif
#if (LZO_SIZEOF_PTRDIFF_T > 0)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == LZO_SIZEOF_PTRDIFF_T)
#endif
    LZOCHK_ASSERT(sizeof(void*) >= sizeof(char*))
#if (LZO_SIZEOF_VOID_P > 0)
    LZOCHK_ASSERT(sizeof(void*) == LZO_SIZEOF_VOID_P)
    LZOCHK_ASSERT(sizeof(char*) == LZO_SIZEOF_VOID_P)
#endif
#if (LZO_HAVE_MM_HUGE_PTR)
    LZOCHK_ASSERT(4 == sizeof(void __huge*))
    LZOCHK_ASSERT(4 == sizeof(char __huge*))
#endif
#if (LZO_ABI_I8LP16)
    LZOCHK_ASSERT((((1u  <<  7) + 1) >>  7) == 1)
    LZOCHK_ASSERT((((1ul << 15) + 1) >> 15) == 1)
#else
    LZOCHK_ASSERT((((1u  << 15) + 1) >> 15) == 1)
    LZOCHK_ASSERT((((1ul << 31) + 1) >> 31) == 1)
#endif
#if defined(LZOCHK_CFG_PEDANTIC)
#if defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0150)
#else
    LZOCHK_ASSERT((1   << (8*LZO_SIZEOF_INT-1)) < 0)
#endif
#endif
    LZOCHK_ASSERT((1u  << (8*LZO_SIZEOF_INT-1)) > 0)
#if defined(LZOCHK_CFG_PEDANTIC)
    LZOCHK_ASSERT((1l  << (8*LZO_SIZEOF_LONG-1)) < 0)
#endif
    LZOCHK_ASSERT((1ul << (8*LZO_SIZEOF_LONG-1)) > 0)
#if defined(lzo_int16e_t)
    LZOCHK_ASSERT(sizeof(lzo_int16e_t) == 2)
    LZOCHK_ASSERT(sizeof(lzo_int16e_t) == LZO_SIZEOF_LZO_INT16E_T)
    LZOCHK_ASSERT(sizeof(lzo_uint16e_t) == 2)
    LZOCHK_ASSERT(sizeof(lzo_int16e_t) == sizeof(lzo_uint16e_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int16e_t)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint16e_t)
#if defined(__MSDOS__) && defined(__TURBOC__) && (__TURBOC__ < 0x0150)
#else
    LZOCHK_ASSERT((LZO_STATIC_CAST(lzo_uint16e_t, (~LZO_STATIC_CAST(lzo_uint16e_t,0ul))) >> 15) == 1)
#endif
    LZOCHK_ASSERT( LZO_STATIC_CAST(lzo_int16e_t, (1 + ~LZO_STATIC_CAST(lzo_int16e_t, 0))) == 0)
#if defined(LZOCHK_CFG_PEDANTIC)
    LZOCHK_ASSERT( LZO_STATIC_CAST(lzo_uint16e_t, (1 + ~LZO_STATIC_CAST(lzo_uint16e_t, 0))) == 0)
#endif
#endif
#if defined(lzo_int32e_t)
    LZOCHK_ASSERT(sizeof(lzo_int32e_t) == 4)
    LZOCHK_ASSERT(sizeof(lzo_int32e_t) == LZO_SIZEOF_LZO_INT32E_T)
    LZOCHK_ASSERT(sizeof(lzo_uint32e_t) == 4)
    LZOCHK_ASSERT(sizeof(lzo_int32e_t) == sizeof(lzo_uint32e_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int32e_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_int32e_t, 1) << 30) + 1) >> 30) == 1)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint32e_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_uint32e_t, 1) << 31) + 1) >> 31) == 1)
    LZOCHK_ASSERT((LZO_STATIC_CAST(lzo_uint32e_t, (~LZO_STATIC_CAST(lzo_uint32e_t, 0ul))) >> 31) == 1)
    LZOCHK_ASSERT( LZO_STATIC_CAST(lzo_int32e_t, (1 + ~LZO_STATIC_CAST(lzo_int32e_t, 0))) == 0)
#if defined(LZOCHK_CFG_PEDANTIC)
    LZOCHK_ASSERT( LZO_STATIC_CAST(lzo_uint32e_t, (1 + ~LZO_STATIC_CAST(lzo_uint32e_t, 0))) == 0)
#endif
#endif
#if defined(lzo_int32e_t)
    LZOCHK_ASSERT(sizeof(lzo_int32l_t) >= sizeof(lzo_int32e_t))
#endif
    LZOCHK_ASSERT(sizeof(lzo_int32l_t) >= 4)
    LZOCHK_ASSERT(sizeof(lzo_int32l_t) == LZO_SIZEOF_LZO_INT32L_T)
    LZOCHK_ASSERT(sizeof(lzo_uint32l_t) >= 4)
    LZOCHK_ASSERT(sizeof(lzo_int32l_t) == sizeof(lzo_uint32l_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int32l_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_int32l_t, 1) << 30) + 1) >> 30) == 1)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint32l_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_uint32l_t, 1) << 31) + 1) >> 31) == 1)
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) >= sizeof(int))
#if defined(lzo_int32e_t)
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) >= sizeof(lzo_int32e_t))
#endif
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) >= sizeof(lzo_int32l_t))
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) >= 4)
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) >= sizeof(lzo_int32l_t))
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) == LZO_SIZEOF_LZO_INT32F_T)
    LZOCHK_ASSERT(sizeof(lzo_uint32f_t) >= 4)
    LZOCHK_ASSERT(sizeof(lzo_uint32f_t) >= sizeof(lzo_uint32l_t))
    LZOCHK_ASSERT(sizeof(lzo_int32f_t) == sizeof(lzo_uint32f_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int32f_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_int32f_t, 1) << 30) + 1) >> 30) == 1)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint32f_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_uint32f_t, 1) << 31) + 1) >> 31) == 1)
#if defined(lzo_int64e_t)
    LZOCHK_ASSERT(sizeof(lzo_int64e_t) == 8)
    LZOCHK_ASSERT(sizeof(lzo_int64e_t) == LZO_SIZEOF_LZO_INT64E_T)
    LZOCHK_ASSERT(sizeof(lzo_uint64e_t) == 8)
    LZOCHK_ASSERT(sizeof(lzo_int64e_t) == sizeof(lzo_uint64e_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int64e_t)
#if (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0530))
#else
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint64e_t)
#endif
#endif
#if defined(lzo_int64l_t)
#if defined(lzo_int64e_t)
    LZOCHK_ASSERT(sizeof(lzo_int64l_t) >= sizeof(lzo_int64e_t))
#endif
    LZOCHK_ASSERT(sizeof(lzo_int64l_t) >= 8)
    LZOCHK_ASSERT(sizeof(lzo_int64l_t) == LZO_SIZEOF_LZO_INT64L_T)
    LZOCHK_ASSERT(sizeof(lzo_uint64l_t) >= 8)
    LZOCHK_ASSERT(sizeof(lzo_int64l_t) == sizeof(lzo_uint64l_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int64l_t)
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_int64l_t, 1) << 62) + 1) >> 62) == 1)
    LZOCHK_ASSERT(((( LZO_INT64_C(1) << 62) + 1) >> 62) == 1)
#if (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0530))
#else
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint64l_t)
    LZOCHK_ASSERT(LZO_UINT64_C(18446744073709551615)     > 0)
#endif
    LZOCHK_ASSERT(((( LZO_STATIC_CAST(lzo_uint64l_t, 1) << 63) + 1) >> 63) == 1)
    LZOCHK_ASSERT(((( LZO_UINT64_C(1) << 63) + 1) >> 63) == 1)
#if (LZO_CC_GNUC && (LZO_CC_GNUC < 0x020600ul))
    LZOCHK_ASSERT(LZO_INT64_C(9223372036854775807)       > LZO_INT64_C(0))
#else
    LZOCHK_ASSERT(LZO_INT64_C(9223372036854775807)       > 0)
#endif
    LZOCHK_ASSERT(LZO_INT64_C(-9223372036854775807) - 1  < 0)
    LZOCHK_ASSERT( LZO_INT64_C(9223372036854775807) % LZO_INT32_C(2147483629)  == 721)
    LZOCHK_ASSERT( LZO_INT64_C(9223372036854775807) % LZO_INT32_C(2147483647)  == 1)
    LZOCHK_ASSERT(LZO_UINT64_C(9223372036854775807) % LZO_UINT32_C(2147483629) == 721)
    LZOCHK_ASSERT(LZO_UINT64_C(9223372036854775807) % LZO_UINT32_C(2147483647) == 1)
#endif
#if defined(lzo_int64f_t)
#if defined(lzo_int64e_t)
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) >= sizeof(lzo_int64e_t))
#endif
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) >= sizeof(lzo_int64l_t))
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) >= 8)
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) >= sizeof(lzo_int64l_t))
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) == LZO_SIZEOF_LZO_INT64F_T)
    LZOCHK_ASSERT(sizeof(lzo_uint64f_t) >= 8)
    LZOCHK_ASSERT(sizeof(lzo_uint64f_t) >= sizeof(lzo_uint64l_t))
    LZOCHK_ASSERT(sizeof(lzo_int64f_t) == sizeof(lzo_uint64f_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int64f_t)
#if (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0530))
#else
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint64f_t)
#endif
#endif
#if !defined(__LZO_INTPTR_T_IS_POINTER)
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_intptr_t)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uintptr_t)
#endif
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) >= sizeof(void *))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == LZO_SIZEOF_LZO_INTPTR_T)
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(lzo_uintptr_t))
#if defined(lzo_word_t)
    LZOCHK_ASSERT(LZO_WORDSIZE == LZO_SIZEOF_LZO_WORD_T)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_word_t)
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_sword_t)
    LZOCHK_ASSERT(sizeof(lzo_word_t) == LZO_SIZEOF_LZO_WORD_T)
    LZOCHK_ASSERT(sizeof(lzo_word_t) == sizeof(lzo_sword_t))
#endif
    LZOCHK_ASSERT(sizeof(lzo_int8_t) == 1)
    LZOCHK_ASSERT(sizeof(lzo_uint8_t) == 1)
    LZOCHK_ASSERT(sizeof(lzo_int8_t) == sizeof(lzo_uint8_t))
    LZOCHK_ASSERT_IS_SIGNED_T(lzo_int8_t)
    LZOCHK_ASSERT_IS_UNSIGNED_T(lzo_uint8_t)
#if defined(LZO_INT16_C)
    LZOCHK_ASSERT(sizeof(LZO_INT16_C(0)) >= 2)
    LZOCHK_ASSERT(sizeof(LZO_UINT16_C(0)) >= 2)
    LZOCHK_ASSERT((LZO_UINT16_C(0xffff) >> 15) == 1)
#endif
#if defined(LZO_INT32_C)
    LZOCHK_ASSERT(sizeof(LZO_INT32_C(0)) >= 4)
    LZOCHK_ASSERT(sizeof(LZO_UINT32_C(0)) >= 4)
    LZOCHK_ASSERT((LZO_UINT32_C(0xffffffff) >> 31) == 1)
#endif
#if defined(LZO_INT64_C)
#if (LZO_CC_BORLANDC && (__BORLANDC__ < 0x0560))
#else
    LZOCHK_ASSERT(sizeof(LZO_INT64_C(0)) >= 8)
    LZOCHK_ASSERT(sizeof(LZO_UINT64_C(0)) >= 8)
#endif
    LZOCHK_ASSERT((LZO_UINT64_C(0xffffffffffffffff) >> 63) == 1)
    LZOCHK_ASSERT((LZO_UINT64_C(0xffffffffffffffff) & ~0)  == LZO_UINT64_C(0xffffffffffffffff))
    LZOCHK_ASSERT((LZO_UINT64_C(0xffffffffffffffff) & ~0l) == LZO_UINT64_C(0xffffffffffffffff))
#if (LZO_SIZEOF_INT == 4)
# if (LZO_CC_GNUC && (LZO_CC_GNUC < 0x020000ul))
# else
    LZOCHK_ASSERT((LZO_UINT64_C(0xffffffffffffffff) & (~0u+0u)) == 0xffffffffu)
# endif
#endif
#if (LZO_SIZEOF_LONG == 4)
# if (LZO_CC_GNUC && (LZO_CC_GNUC < 0x020000ul))
# else
    LZOCHK_ASSERT((LZO_UINT64_C(0xffffffffffffffff) & (~0ul+0ul)) == 0xfffffffful)
# endif
#endif
#endif
#if (LZO_MM_TINY || LZO_MM_SMALL || LZO_MM_MEDIUM)
    LZOCHK_ASSERT(sizeof(void*) == 2)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == 2)
#elif (LZO_MM_COMPACT || LZO_MM_LARGE || LZO_MM_HUGE)
    LZOCHK_ASSERT(sizeof(void*) == 4)
#endif
#if (LZO_MM_TINY || LZO_MM_SMALL || LZO_MM_COMPACT)
    LZOCHK_ASSERT(sizeof(void (*)(void)) == 2)
#elif (LZO_MM_MEDIUM || LZO_MM_LARGE || LZO_MM_HUGE)
    LZOCHK_ASSERT(sizeof(void (*)(void)) == 4)
#endif
#if (LZO_ABI_ILP32)
    LZOCHK_ASSERT(sizeof(int) == 4)
    LZOCHK_ASSERT(sizeof(long) == 4)
    LZOCHK_ASSERT(sizeof(void*) == 4)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_ILP64)
    LZOCHK_ASSERT(sizeof(int) == 8)
    LZOCHK_ASSERT(sizeof(long) == 8)
    LZOCHK_ASSERT(sizeof(void*) == 8)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_IP32L64)
    LZOCHK_ASSERT(sizeof(int) == 4)
    LZOCHK_ASSERT(sizeof(long) == 8)
    LZOCHK_ASSERT(sizeof(void*) == 4)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_LLP64)
    LZOCHK_ASSERT(sizeof(int) == 4)
    LZOCHK_ASSERT(sizeof(long) == 4)
    LZOCHK_ASSERT(sizeof(void*) == 8)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_LP32)
    LZOCHK_ASSERT(sizeof(int) == 2)
    LZOCHK_ASSERT(sizeof(long) == 4)
    LZOCHK_ASSERT(sizeof(void*) == 4)
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_LP64)
    LZOCHK_ASSERT(sizeof(int) == 4)
    LZOCHK_ASSERT(sizeof(long) == 8)
    LZOCHK_ASSERT(sizeof(void*) == 8)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_ABI_IP32W64)
    LZOCHK_ASSERT(sizeof(int) == 4)
    LZOCHK_ASSERT(sizeof(void*) == 4)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(size_t) == sizeof(void*))
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
    LZOCHK_ASSERT(LZO_WORDSIZE == 8)
#endif
#if (LZO_ARCH_I086)
    LZOCHK_ASSERT(sizeof(size_t) == 2)
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#elif (LZO_ARCH_I386 || LZO_ARCH_M68K)
    LZOCHK_ASSERT(sizeof(size_t) == 4)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == 4)
    LZOCHK_ASSERT(sizeof(lzo_intptr_t) == sizeof(void *))
#endif
#if (LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_WIN32)
    LZOCHK_ASSERT(sizeof(size_t) == 4)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == 4)
    LZOCHK_ASSERT(sizeof(void (*)(void)) == 4)
#elif (LZO_OS_WIN64)
    LZOCHK_ASSERT(sizeof(size_t) == 8)
    LZOCHK_ASSERT(sizeof(ptrdiff_t) == 8)
    LZOCHK_ASSERT(sizeof(void (*)(void)) == 8)
#endif
#if (LZO_CC_NDPC)
#elif (LZO_SIZEOF_INT > 1)
    LZOCHK_ASSERT( LZO_STATIC_CAST(int, LZO_STATIC_CAST(unsigned char, LZO_STATIC_CAST(signed char, -1))) == 255)
#endif
#if defined(LZOCHK_CFG_PEDANTIC)
#if (LZO_CC_KEILC)
#elif (LZO_CC_NDPC)
#elif !(LZO_BROKEN_INTEGRAL_PROMOTION) && (LZO_SIZEOF_INT > 1)
    LZOCHK_ASSERT( ((LZO_STATIC_CAST(unsigned char, 128)) << LZO_STATIC_CAST(int, (8*sizeof(int)-8))) < 0)
#endif
#endif
#if defined(LZOCHK_CFG_PEDANTIC)
#if (LZO_CC_BORLANDC && (__BORLANDC__ >= 0x0530) && (__BORLANDC__ < 0x0560))
#  pragma option pop
#endif
#endif
#endif
#if defined(LZO_WANT_ACCLIB_VGET)
#  undef LZO_WANT_ACCLIB_VGET
#define __LZOLIB_VGET_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)                r __LZOLIB_FUNCNAME(f)
#endif
#if !defined(LZOLIB_PUBLIC_NOINLINE)
#  if !defined(__lzo_noinline)
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     r __LZOLIB_FUNCNAME(f)
#  elif (LZO_CC_CLANG || (LZO_CC_GNUC >= 0x030400ul) || LZO_CC_LLVM)
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     __lzo_noinline __attribute__((__used__)) r __LZOLIB_FUNCNAME(f)
#  else
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     __lzo_noinline r __LZOLIB_FUNCNAME(f)
#  endif
#endif
extern void* volatile lzo_vget_ptr__;
#if (LZO_CC_CLANG || (LZO_CC_GNUC >= 0x030400ul) || LZO_CC_LLVM)
void* volatile __attribute__((__used__)) lzo_vget_ptr__ = LZO_STATIC_CAST(void *, 0);
#else
void* volatile lzo_vget_ptr__ = LZO_STATIC_CAST(void *, 0);
#endif
#ifndef __LZOLIB_VGET_BODY
#define __LZOLIB_VGET_BODY(T) \
    if __lzo_unlikely(lzo_vget_ptr__) { \
        typedef T __lzo_may_alias TT; \
        unsigned char e; expr &= 255; e = LZO_STATIC_CAST(unsigned char, expr); \
        * LZO_STATIC_CAST(TT *, lzo_vget_ptr__) = v; \
        * LZO_STATIC_CAST(unsigned char *, lzo_vget_ptr__) = e; \
        v = * LZO_STATIC_CAST(TT *, lzo_vget_ptr__); \
    } \
    return v;
#endif
LZOLIB_PUBLIC_NOINLINE(short, lzo_vget_short) (short v, int expr)
{
    __LZOLIB_VGET_BODY(short)
}
LZOLIB_PUBLIC_NOINLINE(int, lzo_vget_int) (int v, int expr)
{
    __LZOLIB_VGET_BODY(int)
}
LZOLIB_PUBLIC_NOINLINE(long, lzo_vget_long) (long v, int expr)
{
    __LZOLIB_VGET_BODY(long)
}
#if defined(lzo_int64l_t)
LZOLIB_PUBLIC_NOINLINE(lzo_int64l_t, lzo_vget_lzo_int64l_t) (lzo_int64l_t v, int expr)
{
    __LZOLIB_VGET_BODY(lzo_int64l_t)
}
#endif
LZOLIB_PUBLIC_NOINLINE(lzo_hsize_t, lzo_vget_lzo_hsize_t) (lzo_hsize_t v, int expr)
{
    __LZOLIB_VGET_BODY(lzo_hsize_t)
}
#if !(LZO_CFG_NO_DOUBLE)
LZOLIB_PUBLIC_NOINLINE(double, lzo_vget_double) (double v, int expr)
{
    __LZOLIB_VGET_BODY(double)
}
#endif
LZOLIB_PUBLIC_NOINLINE(lzo_hvoid_p, lzo_vget_lzo_hvoid_p) (lzo_hvoid_p v, int expr)
{
    __LZOLIB_VGET_BODY(lzo_hvoid_p)
}
#if (LZO_ARCH_I086 && LZO_CC_TURBOC && (__TURBOC__ == 0x0295)) && !defined(__cplusplus)
LZOLIB_PUBLIC_NOINLINE(lzo_hvoid_p, lzo_vget_lzo_hvoid_cp) (const lzo_hvoid_p vv, int expr)
{
    lzo_hvoid_p v = (lzo_hvoid_p) vv;
    __LZOLIB_VGET_BODY(lzo_hvoid_p)
}
#else
LZOLIB_PUBLIC_NOINLINE(const lzo_hvoid_p, lzo_vget_lzo_hvoid_cp) (const lzo_hvoid_p v, int expr)
{
    __LZOLIB_VGET_BODY(const lzo_hvoid_p)
}
#endif
#endif
#if defined(LZO_WANT_ACCLIB_HMEMCPY)
#  undef LZO_WANT_ACCLIB_HMEMCPY
#define __LZOLIB_HMEMCPY_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(int, lzo_hmemcmp) (const lzo_hvoid_p s1, const lzo_hvoid_p s2, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !(HAVE_MEMCMP)
    const lzo_hbyte_p p1 = LZO_STATIC_CAST(const lzo_hbyte_p, s1);
    const lzo_hbyte_p p2 = LZO_STATIC_CAST(const lzo_hbyte_p, s2);
    if __lzo_likely(len > 0) do
    {
        int d = *p1 - *p2;
        if (d != 0)
            return d;
        p1++; p2++;
    } while __lzo_likely(--len > 0);
    return 0;
#else
    return memcmp(s1, s2, len);
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemcpy) (lzo_hvoid_p dest, const lzo_hvoid_p src, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !(HAVE_MEMCPY)
    lzo_hbyte_p p1 = LZO_STATIC_CAST(lzo_hbyte_p, dest);
    const lzo_hbyte_p p2 = LZO_STATIC_CAST(const lzo_hbyte_p, src);
    if (!(len > 0) || p1 == p2)
        return dest;
    do
        *p1++ = *p2++;
    while __lzo_likely(--len > 0);
    return dest;
#else
    return memcpy(dest, src, len);
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemmove) (lzo_hvoid_p dest, const lzo_hvoid_p src, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !(HAVE_MEMMOVE)
    lzo_hbyte_p p1 = LZO_STATIC_CAST(lzo_hbyte_p, dest);
    const lzo_hbyte_p p2 = LZO_STATIC_CAST(const lzo_hbyte_p, src);
    if (!(len > 0) || p1 == p2)
        return dest;
    if (p1 < p2)
    {
        do
            *p1++ = *p2++;
        while __lzo_likely(--len > 0);
    }
    else
    {
        p1 += len;
        p2 += len;
        do
            *--p1 = *--p2;
        while __lzo_likely(--len > 0);
    }
    return dest;
#else
    return memmove(dest, src, len);
#endif
}
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_hmemset) (lzo_hvoid_p s, int cc, lzo_hsize_t len)
{
#if (LZO_HAVE_MM_HUGE_PTR) || !(HAVE_MEMSET)
    lzo_hbyte_p p = LZO_STATIC_CAST(lzo_hbyte_p, s);
    unsigned char c = LZO_ITRUNC(unsigned char, cc);
    if __lzo_likely(len > 0) do
        *p++ = c;
    while __lzo_likely(--len > 0);
    return s;
#else
    return memset(s, cc, len);
#endif
}
#endif
#if defined(LZO_WANT_ACCLIB_RAND)
#  undef LZO_WANT_ACCLIB_RAND
#define __LZOLIB_RAND_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(void, lzo_srand31) (lzo_rand31_p r, lzo_uint32l_t seed)
{
    r->seed = seed & LZO_UINT32_C(0xffffffff);
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_rand31) (lzo_rand31_p r)
{
    r->seed = r->seed * LZO_UINT32_C(1103515245) + 12345;
    r->seed &= LZO_UINT32_C(0x7fffffff);
    return r->seed;
}
#if defined(lzo_int64l_t)
LZOLIB_PUBLIC(void, lzo_srand48) (lzo_rand48_p r, lzo_uint32l_t seed)
{
    r->seed = seed & LZO_UINT32_C(0xffffffff);
    r->seed <<= 16; r->seed |= 0x330e;
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_rand48) (lzo_rand48_p r)
{
    lzo_uint64l_t a;
    r->seed = r->seed * LZO_UINT64_C(25214903917) + 11;
    r->seed &= LZO_UINT64_C(0xffffffffffff);
    a = r->seed >> 17;
    return LZO_STATIC_CAST(lzo_uint32l_t, a);
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_rand48_r32) (lzo_rand48_p r)
{
    lzo_uint64l_t a;
    r->seed = r->seed * LZO_UINT64_C(25214903917) + 11;
    r->seed &= LZO_UINT64_C(0xffffffffffff);
    a = r->seed >> 16;
    return LZO_STATIC_CAST(lzo_uint32l_t, a);
}
#endif
#if defined(lzo_int64l_t)
LZOLIB_PUBLIC(void, lzo_srand64) (lzo_rand64_p r, lzo_uint64l_t seed)
{
    r->seed = seed & LZO_UINT64_C(0xffffffffffffffff);
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_rand64) (lzo_rand64_p r)
{
    lzo_uint64l_t a;
    r->seed = r->seed * LZO_UINT64_C(6364136223846793005) + 1;
#if (LZO_SIZEOF_LZO_INT64L_T > 8)
    r->seed &= LZO_UINT64_C(0xffffffffffffffff);
#endif
    a = r->seed >> 33;
    return LZO_STATIC_CAST(lzo_uint32l_t, a);
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_rand64_r32) (lzo_rand64_p r)
{
    lzo_uint64l_t a;
    r->seed = r->seed * LZO_UINT64_C(6364136223846793005) + 1;
#if (LZO_SIZEOF_LZO_INT64L_T > 8)
    r->seed &= LZO_UINT64_C(0xffffffffffffffff);
#endif
    a = r->seed >> 32;
    return LZO_STATIC_CAST(lzo_uint32l_t, a);
}
#endif
LZOLIB_PUBLIC(void, lzo_srandmt) (lzo_randmt_p r, lzo_uint32l_t seed)
{
    unsigned i = 0;
    do {
        r->s[i++] = (seed &= LZO_UINT32_C(0xffffffff));
        seed ^= seed >> 30;
        seed = seed * LZO_UINT32_C(0x6c078965) + i;
    } while (i != 624);
    r->n = i;
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_randmt) (lzo_randmt_p r)
{
    return (__LZOLIB_FUNCNAME(lzo_randmt_r32)(r)) >> 1;
}
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_randmt_r32) (lzo_randmt_p r)
{
    lzo_uint32l_t v;
    if __lzo_unlikely(r->n == 624) {
        unsigned i = 0, j;
        r->n = 0;
        do {
            j = i - 623; if (LZO_STATIC_CAST(int, j) < 0) j += 624;
            v = (r->s[i] & LZO_UINT32_C(0x80000000)) ^ (r->s[j] & LZO_UINT32_C(0x7fffffff));
            j = i - 227; if (LZO_STATIC_CAST(int, j) < 0) j += 624;
            r->s[i] = r->s[j] ^ (v >> 1);
            if (v & 1) r->s[i] ^= LZO_UINT32_C(0x9908b0df);
        } while (++i != 624);
    }
    { unsigned i = r->n++; v = r->s[i]; }
    v ^= v >> 11; v ^= (v & LZO_UINT32_C(0x013a58ad)) << 7;
    v ^= (v & LZO_UINT32_C(0x0001df8c)) << 15; v ^= v >> 18;
    return v;
}
#if defined(lzo_int64l_t)
LZOLIB_PUBLIC(void, lzo_srandmt64) (lzo_randmt64_p r, lzo_uint64l_t seed)
{
    unsigned i = 0;
    do {
        r->s[i++] = (seed &= LZO_UINT64_C(0xffffffffffffffff));
        seed ^= seed >> 62;
        seed = seed * LZO_UINT64_C(0x5851f42d4c957f2d) + i;
    } while (i != 312);
    r->n = i;
}
#if 0
LZOLIB_PUBLIC(lzo_uint32l_t, lzo_randmt64) (lzo_randmt64_p r)
{
    lzo_uint64l_t v;
    v = (__LZOLIB_FUNCNAME(lzo_randmt64_r64)(r)) >> 33;
    return LZO_STATIC_CAST(lzo_uint32l_t, v);
}
#endif
LZOLIB_PUBLIC(lzo_uint64l_t, lzo_randmt64_r64) (lzo_randmt64_p r)
{
    lzo_uint64l_t v;
    if __lzo_unlikely(r->n == 312) {
        unsigned i = 0, j;
        r->n = 0;
        do {
            j = i - 311; if (LZO_STATIC_CAST(int, j) < 0) j += 312;
            v = (r->s[i] & LZO_UINT64_C(0xffffffff80000000)) ^ (r->s[j] & LZO_UINT64_C(0x7fffffff));
            j = i - 156; if (LZO_STATIC_CAST(int, j) < 0) j += 312;
            r->s[i] = r->s[j] ^ (v >> 1);
            if (v & 1) r->s[i] ^= LZO_UINT64_C(0xb5026f5aa96619e9);
        } while (++i != 312);
    }
    { unsigned i = r->n++; v = r->s[i]; }
    v ^= (v & LZO_UINT64_C(0xaaaaaaaaa0000000)) >> 29;
    v ^= (v & LZO_UINT64_C(0x38eb3ffff6d3)) << 17;
    v ^= (v & LZO_UINT64_C(0x7ffbf77)) << 37;
    return v ^ (v >> 43);
}
#endif
#endif
#if defined(LZO_WANT_ACCLIB_RDTSC)
#  undef LZO_WANT_ACCLIB_RDTSC
#define __LZOLIB_RDTSC_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
#if defined(lzo_int32e_t)
#if (LZO_OS_WIN32 && LZO_CC_PELLESC && (__POCC__ >= 290))
#  pragma warn(push)
#  pragma warn(disable:2007)
#endif
#if (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
#if (LZO_ARCH_AMD64 && LZO_CC_INTELC)
#  define __LZOLIB_RDTSC_REGS   : : "c" (t) : "memory", "rax", "rdx"
#elif (LZO_ARCH_AMD64)
#  define __LZOLIB_RDTSC_REGS   : : "c" (t) : "cc", "memory", "rax", "rdx"
#elif (LZO_ARCH_I386 && LZO_CC_GNUC && (LZO_CC_GNUC < 0x020000ul))
#  define __LZOLIB_RDTSC_REGS   : : "c" (t) : "ax", "dx"
#elif (LZO_ARCH_I386 && LZO_CC_INTELC)
#  define __LZOLIB_RDTSC_REGS   : : "c" (t) : "memory", "eax", "edx"
#else
#  define __LZOLIB_RDTSC_REGS   : : "c" (t) : "cc", "memory", "eax", "edx"
#endif
#endif
LZOLIB_PUBLIC(int, lzo_tsc_read) (lzo_uint32e_t* t)
{
#if (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    __asm__ __volatile__(
        "clc \n" ".byte 0x0f,0x31\n"
        "movl %%eax,(%0)\n" "movl %%edx,4(%0)\n"
        __LZOLIB_RDTSC_REGS
    );
    return 0;
#elif (LZO_ARCH_I386) && (LZO_ASM_SYNTAX_MSC)
    LZO_UNUSED(t);
    __asm {
        mov ecx, t
        clc
#  if (LZO_CC_MSC && (_MSC_VER < 1200))
        _emit 0x0f
        _emit 0x31
#  else
        rdtsc
#  endif
        mov [ecx], eax
        mov [ecx+4], edx
    }
    return 0;
#else
    t[0] = t[1] = 0; return -1;
#endif
}
#if (LZO_OS_WIN32 && LZO_CC_PELLESC && (__POCC__ >= 290))
#  pragma warn(pop)
#endif
#endif
#endif
#if defined(LZO_WANT_ACCLIB_DOSALLOC)
#  undef LZO_WANT_ACCLIB_DOSALLOC
#define __LZOLIB_DOSALLOC_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
#if (LZO_OS_OS216)
LZO_EXTERN_C unsigned short __far __pascal DosAllocHuge(unsigned short, unsigned short, unsigned short __far *, unsigned short, unsigned short);
LZO_EXTERN_C unsigned short __far __pascal DosFreeSeg(unsigned short);
#endif
#if (LZO_OS_DOS16 || LZO_OS_WIN16)
#if !(LZO_CC_AZTECC)
LZOLIB_PUBLIC(void __far*, lzo_dos_alloc) (unsigned long size)
{
    void __far* p = 0;
    union REGS ri, ro;
    if ((long)size <= 0)
        return p;
    size = (size + 15) >> 4;
    if (size > 0xffffu)
        return p;
    ri.x.ax = 0x4800;
    ri.x.bx = (unsigned short) size;
    int86(0x21, &ri, &ro);
    if ((ro.x.cflag & 1) == 0)
        p = (void __far*) LZO_PTR_MK_FP(ro.x.ax, 0);
    return p;
}
LZOLIB_PUBLIC(int, lzo_dos_free) (void __far* p)
{
    union REGS ri, ro;
    struct SREGS rs;
    if (!p)
        return 0;
    if (LZO_PTR_FP_OFF(p) != 0)
        return -1;
    segread(&rs);
    ri.x.ax = 0x4900;
    rs.es = LZO_PTR_FP_SEG(p);
    int86x(0x21, &ri, &ro, &rs);
    if (ro.x.cflag & 1)
        return -1;
    return 0;
}
#endif
#endif
#if (LZO_OS_OS216)
LZOLIB_PUBLIC(void __far*, lzo_dos_alloc) (unsigned long size)
{
    void __far* p = 0;
    unsigned short sel = 0;
    if ((long)size <= 0)
        return p;
    if (DosAllocHuge((unsigned short)(size >> 16), (unsigned short)size, &sel, 0, 0) == 0)
        p = (void __far*) LZO_PTR_MK_FP(sel, 0);
    return p;
}
LZOLIB_PUBLIC(int, lzo_dos_free) (void __far* p)
{
    if (!p)
        return 0;
    if (LZO_PTR_FP_OFF(p) != 0)
        return -1;
    if (DosFreeSeg(LZO_PTR_FP_SEG(p)) != 0)
        return -1;
    return 0;
}
#endif
#endif
#if defined(LZO_WANT_ACCLIB_GETOPT)
#  undef LZO_WANT_ACCLIB_GETOPT
#define __LZOLIB_GETOPT_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(void, lzo_getopt_init) (lzo_getopt_p g,
                                      int start_argc, int argc, char** argv)
{
    memset(g, 0, sizeof(*g));
    g->optind = start_argc;
    g->argc = argc; g->argv = argv;
    g->optopt = -1;
}
static int __LZOLIB_FUNCNAME(lzo_getopt_rotate) (char** p, int first, int middle, int last)
{
    int i = middle, n = middle - first;
    if (first >= middle || middle >= last) return 0;
    for (;;)
    {
        char* t = p[first]; p[first] = p[i]; p[i] = t;
        if (++first == middle)
        {
            if (++i == last) break;
            middle = i;
        }
        else if (++i == last)
            i = middle;
    }
    return n;
}
static int __LZOLIB_FUNCNAME(lzo_getopt_perror) (lzo_getopt_p g, int ret, const char* f, ...)
{
    if (g->opterr)
    {
#if (HAVE_STDARG_H)
        struct { va_list ap; } s;
        va_start(s.ap, f);
        g->opterr(g, f, &s);
        va_end(s.ap);
#else
        g->opterr(g, f, NULL);
#endif
    }
    ++g->errcount;
    return ret;
}
LZOLIB_PUBLIC(int, lzo_getopt) (lzo_getopt_p g,
                                const char* shortopts,
                                const lzo_getopt_longopt_p longopts,
                                int* longind)
{
#define pe  __LZOLIB_FUNCNAME(lzo_getopt_perror)
    int ordering = LZO_GETOPT_PERMUTE;
    int missing_arg_ret = g->bad_option;
    char* a;
    if (shortopts)
    {
        if (*shortopts == '-' || *shortopts == '+')
            ordering = *shortopts++ == '-' ? LZO_GETOPT_RETURN_IN_ORDER : LZO_GETOPT_REQUIRE_ORDER;
        if (*shortopts == ':')
            missing_arg_ret = *shortopts++;
    }
    g->optarg = NULL;
    if (g->optopt == -1)
        g->optopt = g->bad_option;
    if (longind)
        *longind = -1;
    if (g->eof)
        return -1;
    if (g->shortpos)
        goto lzo_label_next_shortopt;
    g->optind -= __LZOLIB_FUNCNAME(lzo_getopt_rotate)(g->argv, g->pending_rotate_first, g->pending_rotate_middle, g->optind);
    g->pending_rotate_first = g->pending_rotate_middle = g->optind;
    if (ordering == LZO_GETOPT_PERMUTE)
    {
        while (g->optind < g->argc && !(g->argv[g->optind][0] == '-' && g->argv[g->optind][1]))
            ++g->optind;
        g->pending_rotate_middle = g->optind;
    }
    if (g->optind >= g->argc)
    {
        g->optind = g->pending_rotate_first;
        goto lzo_label_eof;
    }
    a = g->argv[g->optind];
    if (a[0] == '-' && a[1] == '-')
    {
        size_t l = 0;
        const lzo_getopt_longopt_p o;
        const lzo_getopt_longopt_p o1 = NULL;
        const lzo_getopt_longopt_p o2 = NULL;
        int need_exact = 0;
        ++g->optind;
        if (!a[2])
            goto lzo_label_eof;
        for (a += 2; a[l] && a[l] != '=' && a[l] != '#'; )
            ++l;
        for (o = longopts; l && o && o->name; ++o)
        {
            if (strncmp(a, o->name, l) != 0)
                continue;
            if (!o->name[l])
                goto lzo_label_found_o;
            need_exact |= o->has_arg & LZO_GETOPT_EXACT_ARG;
            if (o1) o2 = o;
            else    o1 = o;
        }
        if (!o1 || need_exact)
            return pe(g, g->bad_option, "unrecognized option '--%s'", a);
        if (o2)
            return pe(g, g->bad_option, "option '--%s' is ambiguous (could be '--%s' or '--%s')", a, o1->name, o2->name);
        o = o1;
    lzo_label_found_o:
        a += l;
        switch (o->has_arg & 0x2f)
        {
        case LZO_GETOPT_OPTIONAL_ARG:
            if (a[0])
                g->optarg = a + 1;
            break;
        case LZO_GETOPT_REQUIRED_ARG:
            if (a[0])
                g->optarg = a + 1;
            else if (g->optind < g->argc)
                g->optarg = g->argv[g->optind++];
            if (!g->optarg)
                return pe(g, missing_arg_ret, "option '--%s' requires an argument", o->name);
            break;
        case LZO_GETOPT_REQUIRED_ARG | 0x20:
            if (a[0] && a[1])
                g->optarg = a + 1;
            if (!g->optarg)
                return pe(g, missing_arg_ret, "option '--%s=' requires an argument", o->name);
            break;
        default:
            if (a[0])
                return pe(g, g->bad_option, "option '--%s' doesn't allow an argument", o->name);
            break;
        }
        if (longind)
            *longind = (int) (o - longopts);
        if (o->flag)
        {
            *o->flag = o->val;
            return 0;
        }
        return o->val;
    }
    if (a[0] == '-' && a[1])
    {
        unsigned char c;
        const char* s;
    lzo_label_next_shortopt:
        a = g->argv[g->optind] + ++g->shortpos;
        c = (unsigned char) *a++; s = NULL;
        if (c != ':' && shortopts)
            s = strchr(shortopts, c);
        if (!s || s[1] != ':')
        {
            if (!a[0])
                { ++g->optind; g->shortpos = 0; }
            if (!s)
            {
                g->optopt = c;
                return pe(g, g->bad_option, "invalid option '-%c'", c);
            }
        }
        else
        {
            ++g->optind; g->shortpos = 0;
            if (a[0])
                g->optarg = a;
            else if (s[2] != ':')
            {
                if (g->optind < g->argc)
                    g->optarg = g->argv[g->optind++];
                else
                {
                    g->optopt = c;
                    return pe(g, missing_arg_ret, "option '-%c' requires an argument", c);
                }
            }
        }
        return c;
    }
    if (ordering == LZO_GETOPT_RETURN_IN_ORDER)
    {
        ++g->optind;
        g->optarg = a;
        return 1;
    }
lzo_label_eof:
    g->optind -= __LZOLIB_FUNCNAME(lzo_getopt_rotate)(g->argv, g->pending_rotate_first, g->pending_rotate_middle, g->optind);
    g->pending_rotate_first = g->pending_rotate_middle = g->optind;
    g->eof = 1;
    return -1;
#undef pe
}
#endif
#if defined(LZO_WANT_ACCLIB_HALLOC)
#  undef LZO_WANT_ACCLIB_HALLOC
#define __LZOLIB_HALLOC_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
#if (LZO_HAVE_MM_HUGE_PTR)
#if 1 && (LZO_OS_DOS16 && defined(BLX286))
#  define __LZOLIB_HALLOC_USE_DAH 1
#elif 1 && (LZO_OS_DOS16 && defined(DOSX286))
#  define __LZOLIB_HALLOC_USE_DAH 1
#elif 1 && (LZO_OS_OS216)
#  define __LZOLIB_HALLOC_USE_DAH 1
#elif 1 && (LZO_OS_WIN16)
#  define __LZOLIB_HALLOC_USE_GA 1
#elif 1 && (LZO_OS_DOS16) && (LZO_CC_BORLANDC) && defined(__DPMI16__)
#  define __LZOLIB_HALLOC_USE_GA 1
#endif
#endif
#if (__LZOLIB_HALLOC_USE_DAH)
#if 0 && (LZO_OS_OS216)
#include <os2.h>
#else
LZO_EXTERN_C unsigned short __far __pascal DosAllocHuge(unsigned short, unsigned short, unsigned short __far *, unsigned short, unsigned short);
LZO_EXTERN_C unsigned short __far __pascal DosFreeSeg(unsigned short);
#endif
#endif
#if (__LZOLIB_HALLOC_USE_GA)
#if 0
#define STRICT 1
#include <windows.h>
#else
LZO_EXTERN_C const void __near* __far __pascal GlobalAlloc(unsigned, unsigned long);
LZO_EXTERN_C const void __near* __far __pascal GlobalFree(const void __near*);
LZO_EXTERN_C unsigned long __far __pascal GlobalHandle(unsigned);
LZO_EXTERN_C void __far* __far __pascal GlobalLock(const void __near*);
LZO_EXTERN_C int __far __pascal GlobalUnlock(const void __near*);
#endif
#endif
LZOLIB_PUBLIC(lzo_hvoid_p, lzo_halloc) (lzo_hsize_t size)
{
    lzo_hvoid_p p = LZO_STATIC_CAST(lzo_hvoid_p, 0);
    if (!(size > 0))
        return p;
#if 0 && defined(__palmos__)
    p = MemPtrNew(size);
#elif !(LZO_HAVE_MM_HUGE_PTR)
    if (size < LZO_STATIC_CAST(size_t, -1))
        p = malloc(LZO_STATIC_CAST(size_t, size));
#else
    if (LZO_STATIC_CAST(long, size) <= 0)
        return p;
{
#if (__LZOLIB_HALLOC_USE_DAH)
    unsigned short sel = 0;
    if (DosAllocHuge((unsigned short)(size >> 16), (unsigned short)size, &sel, 0, 0) == 0)
        p = (lzo_hvoid_p) LZO_PTR_MK_FP(sel, 0);
#elif (__LZOLIB_HALLOC_USE_GA)
    const void __near* h = GlobalAlloc(2, size);
    if (h) {
        p = GlobalLock(h);
        if (p && LZO_PTR_FP_OFF(p) != 0) {
            GlobalUnlock(h);
            p = 0;
        }
        if (!p)
            GlobalFree(h);
    }
#elif (LZO_CC_MSC && (_MSC_VER >= 700))
    p = _halloc(size, 1);
#elif (LZO_CC_MSC || LZO_CC_WATCOMC)
    p = halloc(size, 1);
#elif (LZO_CC_DMC || LZO_CC_SYMANTECC || LZO_CC_ZORTECHC)
    p = farmalloc(size);
#elif (LZO_CC_BORLANDC || LZO_CC_TURBOC)
    p = farmalloc(size);
#elif (LZO_CC_AZTECC)
    p = lmalloc(size);
#else
    if (size < LZO_STATIC_CAST(size_t, -1))
        p = malloc(LZO_STATIC_CAST(size_t, size));
#endif
}
#endif
    return p;
}
LZOLIB_PUBLIC(void, lzo_hfree) (lzo_hvoid_p p)
{
    if (!p)
        return;
#if 0 && defined(__palmos__)
    MemPtrFree(p);
#elif !(LZO_HAVE_MM_HUGE_PTR)
    free(p);
#else
#if (__LZOLIB_HALLOC_USE_DAH)
    if (LZO_PTR_FP_OFF(p) == 0)
        DosFreeSeg((unsigned short) LZO_PTR_FP_SEG(p));
#elif (__LZOLIB_HALLOC_USE_GA)
    if (LZO_PTR_FP_OFF(p) == 0) {
        const void __near* h = (const void __near*) (unsigned) GlobalHandle(LZO_PTR_FP_SEG(p));
        if (h) {
            GlobalUnlock(h);
            GlobalFree(h);
        }
    }
#elif (LZO_CC_MSC && (_MSC_VER >= 700))
    _hfree(p);
#elif (LZO_CC_MSC || LZO_CC_WATCOMC)
    hfree(p);
#elif (LZO_CC_DMC || LZO_CC_SYMANTECC || LZO_CC_ZORTECHC)
    farfree((void __far*) p);
#elif (LZO_CC_BORLANDC || LZO_CC_TURBOC)
    farfree((void __far*) p);
#elif (LZO_CC_AZTECC)
    lfree(p);
#else
    free(p);
#endif
#endif
}
#endif
#if defined(LZO_WANT_ACCLIB_HFREAD)
#  undef LZO_WANT_ACCLIB_HFREAD
#define __LZOLIB_HFREAD_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(lzo_hsize_t, lzo_hfread) (void* vfp, lzo_hvoid_p buf, lzo_hsize_t size)
{
    FILE* fp = LZO_STATIC_CAST(FILE *, vfp);
#if (LZO_HAVE_MM_HUGE_PTR)
#if (LZO_MM_TINY || LZO_MM_SMALL || LZO_MM_MEDIUM)
#define __LZOLIB_REQUIRE_HMEMCPY_CH 1
    unsigned char tmp[512];
    lzo_hsize_t l = 0;
    while (l < size)
    {
        size_t n = size - l > sizeof(tmp) ? sizeof(tmp) : (size_t) (size - l);
        n = fread(tmp, 1, n, fp);
        if (n == 0)
            break;
        __LZOLIB_FUNCNAME(lzo_hmemcpy)((lzo_hbyte_p)buf + l, tmp, (lzo_hsize_t)n);
        l += n;
    }
    return l;
#elif (LZO_MM_COMPACT || LZO_MM_LARGE || LZO_MM_HUGE)
    lzo_hbyte_p b = (lzo_hbyte_p) buf;
    lzo_hsize_t l = 0;
    while (l < size)
    {
        size_t n;
        n = LZO_PTR_FP_OFF(b); n = (n <= 1) ? 0x8000u : (0u - n);
        if ((lzo_hsize_t) n > size - l)
            n = (size_t) (size - l);
        n = fread((void __far*)b, 1, n, fp);
        if (n == 0)
            break;
        b += n; l += n;
    }
    return l;
#else
#  error "unknown memory model"
#endif
#else
    return fread(buf, 1, size, fp);
#endif
}
LZOLIB_PUBLIC(lzo_hsize_t, lzo_hfwrite) (void* vfp, const lzo_hvoid_p buf, lzo_hsize_t size)
{
    FILE* fp = LZO_STATIC_CAST(FILE *, vfp);
#if (LZO_HAVE_MM_HUGE_PTR)
#if (LZO_MM_TINY || LZO_MM_SMALL || LZO_MM_MEDIUM)
#define __LZOLIB_REQUIRE_HMEMCPY_CH 1
    unsigned char tmp[512];
    lzo_hsize_t l = 0;
    while (l < size)
    {
        size_t n = size - l > sizeof(tmp) ? sizeof(tmp) : (size_t) (size - l);
        __LZOLIB_FUNCNAME(lzo_hmemcpy)(tmp, (const lzo_hbyte_p)buf + l, (lzo_hsize_t)n);
        n = fwrite(tmp, 1, n, fp);
        if (n == 0)
            break;
        l += n;
    }
    return l;
#elif (LZO_MM_COMPACT || LZO_MM_LARGE || LZO_MM_HUGE)
    const lzo_hbyte_p b = (const lzo_hbyte_p) buf;
    lzo_hsize_t l = 0;
    while (l < size)
    {
        size_t n;
        n = LZO_PTR_FP_OFF(b); n = (n <= 1) ? 0x8000u : (0u - n);
        if ((lzo_hsize_t) n > size - l)
            n = (size_t) (size - l);
        n = fwrite((void __far*)b, 1, n, fp);
        if (n == 0)
            break;
        b += n; l += n;
    }
    return l;
#else
#  error "unknown memory model"
#endif
#else
    return fwrite(buf, 1, size, fp);
#endif
}
#endif
#if defined(LZO_WANT_ACCLIB_HSREAD)
#  undef LZO_WANT_ACCLIB_HSREAD
#define __LZOLIB_HSREAD_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
LZOLIB_PUBLIC(long, lzo_safe_hread) (int fd, lzo_hvoid_p buf, long size)
{
    lzo_hbyte_p b = (lzo_hbyte_p) buf;
    long l = 0;
    int saved_errno;
    saved_errno = errno;
    while (l < size)
    {
        long n = size - l;
#if (LZO_HAVE_MM_HUGE_PTR)
#  define __LZOLIB_REQUIRE_HREAD_CH 1
        errno = 0; n = lzo_hread(fd, b, n);
#elif (LZO_OS_DOS32) && defined(__DJGPP__)
        errno = 0; n = _read(fd, b, n);
#else
        errno = 0; n = read(fd, b, n);
#endif
        if (n == 0)
            break;
        if (n < 0) {
#if defined(EAGAIN)
            if (errno == (EAGAIN)) continue;
#endif
#if defined(EINTR)
            if (errno == (EINTR)) continue;
#endif
            if (errno == 0) errno = 1;
            return l;
        }
        b += n; l += n;
    }
    errno = saved_errno;
    return l;
}
LZOLIB_PUBLIC(long, lzo_safe_hwrite) (int fd, const lzo_hvoid_p buf, long size)
{
    const lzo_hbyte_p b = (const lzo_hbyte_p) buf;
    long l = 0;
    int saved_errno;
    saved_errno = errno;
    while (l < size)
    {
        long n = size - l;
#if (LZO_HAVE_MM_HUGE_PTR)
#  define __LZOLIB_REQUIRE_HREAD_CH 1
        errno = 0; n = lzo_hwrite(fd, b, n);
#elif (LZO_OS_DOS32) && defined(__DJGPP__)
        errno = 0; n = _write(fd, b, n);
#else
        errno = 0; n = write(fd, b, n);
#endif
        if (n == 0)
            break;
        if (n < 0) {
#if defined(EAGAIN)
            if (errno == (EAGAIN)) continue;
#endif
#if defined(EINTR)
            if (errno == (EINTR)) continue;
#endif
            if (errno == 0) errno = 1;
            return l;
        }
        b += n; l += n;
    }
    errno = saved_errno;
    return l;
}
#endif
#if defined(LZO_WANT_ACCLIB_PCLOCK)
#  undef LZO_WANT_ACCLIB_PCLOCK
#define __LZOLIB_PCLOCK_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
#if 1 && (LZO_OS_POSIX_LINUX && LZO_ARCH_AMD64 && LZO_ASM_SYNTAX_GNUC && !LZO_CFG_NO_SYSCALL)
#ifndef lzo_pclock_syscall_clock_gettime
#define lzo_pclock_syscall_clock_gettime lzo_pclock_syscall_clock_gettime
#endif
__lzo_static_noinline long lzo_pclock_syscall_clock_gettime(long clockid, struct timespec *ts)
{
    unsigned long r = 228;
    __asm__ __volatile__("syscall\n" : "=a" (r), "=m" (*ts) : "0" (r), "D" (clockid), "S" (ts) __LZO_ASM_CLOBBER_LIST_CC);
    return LZO_ICAST(long, r);
}
#endif
#if 1 && (LZO_OS_POSIX_LINUX && LZO_ARCH_I386 && LZO_ASM_SYNTAX_GNUC && !LZO_CFG_NO_SYSCALL) && defined(lzo_int64l_t)
#ifndef lzo_pclock_syscall_clock_gettime
#define lzo_pclock_syscall_clock_gettime lzo_pclock_syscall_clock_gettime
#endif
__lzo_static_noinline long lzo_pclock_syscall_clock_gettime(long clockid, struct timespec *ts)
{
    unsigned long r = 265;
    __asm__ __volatile__("pushl %%ebx\n pushl %%edx\n popl %%ebx\n int $0x80\n popl %%ebx\n": "=a" (r), "=m" (*ts) : "0" (r), "d" (clockid), "c" (ts) __LZO_ASM_CLOBBER_LIST_CC);
    return LZO_ICAST(long, r);
}
#endif
#if 0 && defined(lzo_pclock_syscall_clock_gettime)
#ifndef lzo_pclock_read_clock_gettime_r_syscall
#define lzo_pclock_read_clock_gettime_r_syscall lzo_pclock_read_clock_gettime_r_syscall
#endif
static int lzo_pclock_read_clock_gettime_r_syscall(lzo_pclock_handle_p h, lzo_pclock_p c)
{
     struct timespec ts;
    if (lzo_pclock_syscall_clock_gettime(0, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ts.tv_nsec);
    LZO_UNUSED(h); return 0;
}
#endif
#if (HAVE_GETTIMEOFDAY)
#ifndef lzo_pclock_read_gettimeofday
#define lzo_pclock_read_gettimeofday lzo_pclock_read_gettimeofday
#endif
static int lzo_pclock_read_gettimeofday(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
        return -1;
#if defined(lzo_int64l_t)
    c->tv_sec = tv.tv_sec;
#else
    c->tv_sec_high = 0;
    c->tv_sec_low = tv.tv_sec;
#endif
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, (tv.tv_usec * 1000u));
    LZO_UNUSED(h); return 0;
}
#endif
#if defined(CLOCKS_PER_SEC) && !(LZO_CFG_NO_DOUBLE)
#ifndef lzo_pclock_read_clock
#define lzo_pclock_read_clock lzo_pclock_read_clock
#endif
static int lzo_pclock_read_clock(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    clock_t ticks;
    double secs;
#if defined(lzo_int64l_t)
    lzo_uint64l_t nsecs;
    ticks = clock();
    secs = LZO_STATIC_CAST(double, ticks) / (CLOCKS_PER_SEC);
    nsecs = LZO_STATIC_CAST(lzo_uint64l_t, (secs * 1000000000.0));
    c->tv_sec = LZO_STATIC_CAST(lzo_int64l_t, (nsecs / 1000000000ul));
    nsecs = (nsecs % 1000000000ul);
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, nsecs);
#else
    ticks = clock();
    secs = LZO_STATIC_CAST(double, ticks) / (CLOCKS_PER_SEC);
    c->tv_sec_high = 0;
    c->tv_sec_low = LZO_STATIC_CAST(lzo_uint32l_t, (secs + 0.5));
    c->tv_nsec = 0;
#endif
    LZO_UNUSED(h); return 0;
}
#endif
#if 1 && defined(lzo_pclock_syscall_clock_gettime)
#ifndef lzo_pclock_read_clock_gettime_m_syscall
#define lzo_pclock_read_clock_gettime_m_syscall lzo_pclock_read_clock_gettime_m_syscall
#endif
static int lzo_pclock_read_clock_gettime_m_syscall(lzo_pclock_handle_p h, lzo_pclock_p c)
{
     struct timespec ts;
    if (lzo_pclock_syscall_clock_gettime(1, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ts.tv_nsec);
    LZO_UNUSED(h); return 0;
}
#endif
#if (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__) && defined(UCLOCKS_PER_SEC) && !(LZO_CFG_NO_DOUBLE)
#ifndef lzo_pclock_read_uclock
#define lzo_pclock_read_uclock lzo_pclock_read_uclock
#endif
static int lzo_pclock_read_uclock(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    lzo_uint64l_t ticks;
    double secs;
    lzo_uint64l_t nsecs;
    ticks = uclock();
    secs = LZO_STATIC_CAST(double, ticks) / (UCLOCKS_PER_SEC);
    nsecs = LZO_STATIC_CAST(lzo_uint64l_t, (secs * 1000000000.0));
    c->tv_sec = nsecs / 1000000000ul;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, (nsecs % 1000000000ul));
    LZO_UNUSED(h); return 0;
}
#endif
#if 1 && (HAVE_CLOCK_GETTIME) && defined(CLOCK_PROCESS_CPUTIME_ID) && defined(lzo_int64l_t)
#ifndef lzo_pclock_read_clock_gettime_p_libc
#define lzo_pclock_read_clock_gettime_p_libc lzo_pclock_read_clock_gettime_p_libc
#endif
static int lzo_pclock_read_clock_gettime_p_libc(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ts.tv_nsec);
    LZO_UNUSED(h); return 0;
}
#endif
#if 1 && defined(lzo_pclock_syscall_clock_gettime)
#ifndef lzo_pclock_read_clock_gettime_p_syscall
#define lzo_pclock_read_clock_gettime_p_syscall lzo_pclock_read_clock_gettime_p_syscall
#endif
static int lzo_pclock_read_clock_gettime_p_syscall(lzo_pclock_handle_p h, lzo_pclock_p c)
{
     struct timespec ts;
    if (lzo_pclock_syscall_clock_gettime(2, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ts.tv_nsec);
    LZO_UNUSED(h); return 0;
}
#endif
#if (LZO_OS_CYGWIN || LZO_OS_WIN32 || LZO_OS_WIN64) && (LZO_HAVE_WINDOWS_H) && defined(lzo_int64l_t)
#ifndef lzo_pclock_read_getprocesstimes
#define lzo_pclock_read_getprocesstimes lzo_pclock_read_getprocesstimes
#endif
static int lzo_pclock_read_getprocesstimes(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    FILETIME ct, et, kt, ut;
    lzo_uint64l_t ticks;
    if (GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut) == 0)
        return -1;
    ticks = (LZO_STATIC_CAST(lzo_uint64l_t, ut.dwHighDateTime) << 32) | ut.dwLowDateTime;
    if __lzo_unlikely(h->ticks_base == 0)
        h->ticks_base = ticks;
    else
        ticks -= h->ticks_base;
    c->tv_sec = LZO_STATIC_CAST(lzo_int64l_t, (ticks / 10000000ul));
    ticks = (ticks % 10000000ul) * 100u;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ticks);
    LZO_UNUSED(h); return 0;
}
#endif
#if (HAVE_GETRUSAGE) && defined(RUSAGE_SELF)
#ifndef lzo_pclock_read_getrusage
#define lzo_pclock_read_getrusage lzo_pclock_read_getrusage
#endif
static int lzo_pclock_read_getrusage(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) != 0)
        return -1;
#if defined(lzo_int64l_t)
    c->tv_sec = ru.ru_utime.tv_sec;
#else
    c->tv_sec_high = 0;
    c->tv_sec_low = ru.ru_utime.tv_sec;
#endif
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, (ru.ru_utime.tv_usec * 1000u));
    LZO_UNUSED(h); return 0;
}
#endif
#if 1 && (HAVE_CLOCK_GETTIME) && defined(CLOCK_THREAD_CPUTIME_ID) && defined(lzo_int64l_t)
#ifndef lzo_pclock_read_clock_gettime_t_libc
#define lzo_pclock_read_clock_gettime_t_libc lzo_pclock_read_clock_gettime_t_libc
#endif
static int lzo_pclock_read_clock_gettime_t_libc(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = (lzo_uint32l_t) ts.tv_nsec;
    LZO_UNUSED(h); return 0;
}
#endif
#if 1 && defined(lzo_pclock_syscall_clock_gettime)
#ifndef lzo_pclock_read_clock_gettime_t_syscall
#define lzo_pclock_read_clock_gettime_t_syscall lzo_pclock_read_clock_gettime_t_syscall
#endif
static int lzo_pclock_read_clock_gettime_t_syscall(lzo_pclock_handle_p h, lzo_pclock_p c)
{
     struct timespec ts;
    if (lzo_pclock_syscall_clock_gettime(3, &ts) != 0)
        return -1;
    c->tv_sec = ts.tv_sec;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ts.tv_nsec);
    LZO_UNUSED(h); return 0;
}
#endif
#if (LZO_OS_CYGWIN || LZO_OS_WIN32 || LZO_OS_WIN64) && (LZO_HAVE_WINDOWS_H) && defined(lzo_int64l_t)
#ifndef lzo_pclock_read_getthreadtimes
#define lzo_pclock_read_getthreadtimes lzo_pclock_read_getthreadtimes
#endif
static int lzo_pclock_read_getthreadtimes(lzo_pclock_handle_p h, lzo_pclock_p c)
{
    FILETIME ct, et, kt, ut;
    lzo_uint64l_t ticks;
    if (GetThreadTimes(GetCurrentThread(), &ct, &et, &kt, &ut) == 0)
        return -1;
    ticks = (LZO_STATIC_CAST(lzo_uint64l_t, ut.dwHighDateTime) << 32) | ut.dwLowDateTime;
    if __lzo_unlikely(h->ticks_base == 0)
        h->ticks_base = ticks;
    else
        ticks -= h->ticks_base;
    c->tv_sec = LZO_STATIC_CAST(lzo_int64l_t, (ticks / 10000000ul));
    ticks = (ticks % 10000000ul) * 100u;
    c->tv_nsec = LZO_STATIC_CAST(lzo_uint32l_t, ticks);
    LZO_UNUSED(h); return 0;
}
#endif
LZOLIB_PUBLIC(int, lzo_pclock_open) (lzo_pclock_handle_p h, int mode)
{
    lzo_pclock_t c;
    int i;
    h->h = LZO_STATIC_CAST(lzolib_handle_t, 0);
    h->mode = -1;
    h->read_error = 2;
    h->name = NULL;
    h->gettime = LZO_STATIC_CAST(lzo_pclock_gettime_t, 0);
#if defined(lzo_int64l_t)
    h->ticks_base = 0;
#endif
    switch (mode)
    {
    case LZO_PCLOCK_REALTIME:
#     if defined(lzo_pclock_read_clock_gettime_r_syscall)
        if (lzo_pclock_read_clock_gettime_r_syscall(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_r_syscall;
            h->name = "CLOCK_REALTIME/syscall";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_gettimeofday)
        if (lzo_pclock_read_gettimeofday(h, &c) == 0) {
            h->gettime = lzo_pclock_read_gettimeofday;
            h->name = "gettimeofday";
            break;
        }
#     endif
        break;
    case LZO_PCLOCK_MONOTONIC:
#     if defined(lzo_pclock_read_clock_gettime_m_syscall)
        if (lzo_pclock_read_clock_gettime_m_syscall(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_m_syscall;
            h->name = "CLOCK_MONOTONIC/syscall";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_uclock)
        if (lzo_pclock_read_uclock(h, &c) == 0) {
            h->gettime = lzo_pclock_read_uclock;
            h->name = "uclock";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_clock)
        if (lzo_pclock_read_clock(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock;
            h->name = "clock";
            break;
        }
#     endif
        break;
    case LZO_PCLOCK_PROCESS_CPUTIME_ID:
#     if defined(lzo_pclock_read_getprocesstimes)
        if (lzo_pclock_read_getprocesstimes(h, &c) == 0) {
            h->gettime = lzo_pclock_read_getprocesstimes;
            h->name = "GetProcessTimes";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_clock_gettime_p_syscall)
        if (lzo_pclock_read_clock_gettime_p_syscall(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_p_syscall;
            h->name = "CLOCK_PROCESS_CPUTIME_ID/syscall";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_clock_gettime_p_libc)
        if (lzo_pclock_read_clock_gettime_p_libc(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_p_libc;
            h->name = "CLOCK_PROCESS_CPUTIME_ID/libc";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_getrusage)
        if (lzo_pclock_read_getrusage(h, &c) == 0) {
            h->gettime = lzo_pclock_read_getrusage;
            h->name = "getrusage";
            break;
        }
#     endif
        break;
    case LZO_PCLOCK_THREAD_CPUTIME_ID:
#     if defined(lzo_pclock_read_getthreadtimes)
        if (lzo_pclock_read_getthreadtimes(h, &c) == 0) {
            h->gettime = lzo_pclock_read_getthreadtimes;
            h->name = "GetThreadTimes";
        }
#     endif
#     if defined(lzo_pclock_read_clock_gettime_t_syscall)
        if (lzo_pclock_read_clock_gettime_t_syscall(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_t_syscall;
            h->name = "CLOCK_THREAD_CPUTIME_ID/syscall";
            break;
        }
#     endif
#     if defined(lzo_pclock_read_clock_gettime_t_libc)
        if (lzo_pclock_read_clock_gettime_t_libc(h, &c) == 0) {
            h->gettime = lzo_pclock_read_clock_gettime_t_libc;
            h->name = "CLOCK_THREAD_CPUTIME_ID/libc";
            break;
        }
#     endif
        break;
    }
    if (!h->gettime)
        return -1;
    if (!h->h)
        h->h = LZO_STATIC_CAST(lzolib_handle_t, 1);
    h->mode = mode;
    h->read_error = 0;
    if (!h->name)
        h->name = "unknown";
    for (i = 0; i < 10; i++) {
        __LZOLIB_FUNCNAME(lzo_pclock_read)(h, &c);
    }
    return 0;
}
LZOLIB_PUBLIC(int, lzo_pclock_open_default) (lzo_pclock_handle_p h)
{
    if (__LZOLIB_FUNCNAME(lzo_pclock_open)(h, LZO_PCLOCK_PROCESS_CPUTIME_ID) == 0)
        return 0;
    if (__LZOLIB_FUNCNAME(lzo_pclock_open)(h, LZO_PCLOCK_MONOTONIC) == 0)
        return 0;
    if (__LZOLIB_FUNCNAME(lzo_pclock_open)(h, LZO_PCLOCK_REALTIME) == 0)
        return 0;
    if (__LZOLIB_FUNCNAME(lzo_pclock_open)(h, LZO_PCLOCK_THREAD_CPUTIME_ID) == 0)
        return 0;
    return -1;
}
LZOLIB_PUBLIC(int, lzo_pclock_close) (lzo_pclock_handle_p h)
{
    h->h = LZO_STATIC_CAST(lzolib_handle_t, 0);
    h->mode = -1;
    h->name = NULL;
    h->gettime = LZO_STATIC_CAST(lzo_pclock_gettime_t, 0);
    return 0;
}
LZOLIB_PUBLIC(void, lzo_pclock_read) (lzo_pclock_handle_p h, lzo_pclock_p c)
{
    if (h->gettime) {
        if (h->gettime(h, c) == 0)
            return;
    }
    h->read_error = 1;
#if defined(lzo_int64l_t)
    c->tv_sec = 0;
#else
    c->tv_sec_high = 0;
    c->tv_sec_low = 0;
#endif
    c->tv_nsec = 0;
}
#if !(LZO_CFG_NO_DOUBLE)
LZOLIB_PUBLIC(double, lzo_pclock_get_elapsed) (lzo_pclock_handle_p h, const lzo_pclock_p start, const lzo_pclock_p stop)
{
    if (!h->h) { h->mode = -1; return 0.0; }
    {
#if 1 && (LZO_ARCH_I386 && LZO_CC_GNUC) && defined(__STRICT_ALIGNMENT__)
    float tstop, tstart;
    tstop  = LZO_STATIC_CAST(float, (stop->tv_sec  + stop->tv_nsec  / 1000000000.0));
    tstart = LZO_STATIC_CAST(float, (start->tv_sec + start->tv_nsec / 1000000000.0));
#elif defined(lzo_int64l_t)
    double tstop, tstart;
#if 1 && (LZO_CC_INTELC)
    { lzo_int64l_t a = stop->tv_sec; lzo_uint32l_t b = stop->tv_nsec;
    tstop = a + b / 1000000000.0; }
    { lzo_int64l_t a = start->tv_sec; lzo_uint32l_t b = start->tv_nsec;
    tstart = a + b / 1000000000.0; }
#else
    tstop  = stop->tv_sec  + stop->tv_nsec  / 1000000000.0;
    tstart = start->tv_sec + start->tv_nsec / 1000000000.0;
#endif
#else
    double tstop, tstart;
    tstop  = stop->tv_sec_low  + stop->tv_nsec  / 1000000000.0;
    tstart = start->tv_sec_low + start->tv_nsec / 1000000000.0;
#endif
    return tstop - tstart;
    }
}
#endif
LZOLIB_PUBLIC(int, lzo_pclock_flush_cpu_cache) (lzo_pclock_handle_p h, unsigned flags)
{
    LZO_UNUSED(h); LZO_UNUSED(flags);
    return -1;
}
#endif
#if defined(LZO_WANT_ACCLIB_MISC)
#  undef LZO_WANT_ACCLIB_MISC
#define __LZOLIB_MISC_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)                r __LZOLIB_FUNCNAME(f)
#endif
#if !defined(LZOLIB_PUBLIC_NOINLINE)
#  if !defined(__lzo_noinline)
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     r __LZOLIB_FUNCNAME(f)
#  elif (LZO_CC_CLANG || (LZO_CC_GNUC >= 0x030400ul) || LZO_CC_LLVM)
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     __lzo_noinline __attribute__((__used__)) r __LZOLIB_FUNCNAME(f)
#  else
#    define LZOLIB_PUBLIC_NOINLINE(r,f)     __lzo_noinline r __LZOLIB_FUNCNAME(f)
#  endif
#endif
#if (LZO_OS_WIN32 && LZO_CC_PELLESC && (__POCC__ >= 290))
#  pragma warn(push)
#  pragma warn(disable:2007)
#endif
LZOLIB_PUBLIC(const char *, lzo_getenv) (const char *s)
{
#if (HAVE_GETENV)
    return getenv(s);
#else
    LZO_UNUSED(s); return LZO_STATIC_CAST(const char *, 0);
#endif
}
LZOLIB_PUBLIC(lzo_intptr_t, lzo_get_osfhandle) (int fd)
{
    if (fd < 0)
        return -1;
#if (LZO_OS_CYGWIN)
    return get_osfhandle(fd);
#elif (LZO_OS_EMX && defined(__RSXNT__))
    return -1;
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
    return -1;
#elif (LZO_OS_WIN32 || LZO_OS_WIN64)
# if (LZO_CC_PELLESC && (__POCC__ < 280))
    return -1;
# elif (LZO_CC_WATCOMC && (__WATCOMC__ < 1000))
    return -1;
# elif (LZO_CC_WATCOMC && (__WATCOMC__ < 1100))
    return _os_handle(fd);
# else
    return _get_osfhandle(fd);
# endif
#else
    return fd;
#endif
}
LZOLIB_PUBLIC(int, lzo_set_binmode) (int fd, int binary)
{
#if (LZO_ARCH_M68K && LZO_OS_TOS && LZO_CC_GNUC) && defined(__MINT__)
    FILE* fp; int old_binary;
    if (fd == STDIN_FILENO) fp = stdin;
    else if (fd == STDOUT_FILENO) fp = stdout;
    else if (fd == STDERR_FILENO) fp = stderr;
    else return -1;
    old_binary = fp->__mode.__binary;
    __set_binmode(fp, binary ? 1 : 0);
    return old_binary ? 1 : 0;
#elif (LZO_ARCH_M68K && LZO_OS_TOS)
    LZO_UNUSED(fd); LZO_UNUSED(binary);
    return -1;
#elif (LZO_OS_DOS16 && (LZO_CC_AZTECC || LZO_CC_PACIFICC))
    LZO_UNUSED(fd); LZO_UNUSED(binary);
    return -1;
#elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
    int r; unsigned old_flags = __djgpp_hwint_flags;
    LZO_COMPILE_TIME_ASSERT(O_BINARY > 0)
    LZO_COMPILE_TIME_ASSERT(O_TEXT > 0)
    if (fd < 0) return -1;
    r = setmode(fd, binary ? O_BINARY : O_TEXT);
    if ((old_flags & 1u) != (__djgpp_hwint_flags & 1u))
        __djgpp_set_ctrl_c(!(old_flags & 1));
    if (r == -1) return -1;
    return (r & O_TEXT) ? 0 : 1;
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
    if (fd < 0) return -1;
    LZO_UNUSED(binary);
    return 1;
#elif (LZO_OS_DOS32 && LZO_CC_HIGHC)
    FILE* fp; int r;
    if (fd == fileno(stdin)) fp = stdin;
    else if (fd == fileno(stdout)) fp = stdout;
    else if (fd == fileno(stderr)) fp = stderr;
    else return -1;
    r = _setmode(fp, binary ? _BINARY : _TEXT);
    if (r == -1) return -1;
    return (r & _BINARY) ? 1 : 0;
#elif (LZO_OS_WIN32 && LZO_CC_MWERKS) && defined(__MSL__)
    LZO_UNUSED(fd); LZO_UNUSED(binary);
    return -1;
#elif (LZO_OS_CYGWIN && (LZO_CC_GNUC < 0x025a00ul))
    LZO_UNUSED(fd); LZO_UNUSED(binary);
    return -1;
#elif (LZO_OS_CYGWIN || LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_EMX || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_WIN16 || LZO_OS_WIN32 || LZO_OS_WIN64)
    int r;
#if !(LZO_CC_ZORTECHC)
    LZO_COMPILE_TIME_ASSERT(O_BINARY > 0)
#endif
    LZO_COMPILE_TIME_ASSERT(O_TEXT > 0)
    if (fd < 0) return -1;
    r = setmode(fd, binary ? O_BINARY : O_TEXT);
    if (r == -1) return -1;
    return (r & O_TEXT) ? 0 : 1;
#else
    if (fd < 0) return -1;
    LZO_UNUSED(binary);
    return 1;
#endif
}
LZOLIB_PUBLIC(int, lzo_isatty) (int fd)
{
    if (fd < 0)
        return 0;
#if (LZO_OS_DOS16 && !(LZO_CC_AZTECC))
    {
        union REGS ri, ro;
        ri.x.ax = 0x4400; ri.x.bx = fd;
        int86(0x21, &ri, &ro);
        if ((ro.x.cflag & 1) == 0)
            if ((ro.x.ax & 0x83) != 0x83)
                return 0;
    }
#elif (LZO_OS_DOS32 && LZO_CC_WATCOMC)
    {
        union REGS ri, ro;
        ri.w.ax = 0x4400; ri.w.bx = LZO_STATIC_CAST(unsigned short, fd);
        int386(0x21, &ri, &ro);
        if ((ro.w.cflag & 1) == 0)
            if ((ro.w.ax & 0x83) != 0x83)
                return 0;
    }
#elif (LZO_HAVE_WINDOWS_H)
    {
        lzo_intptr_t h = __LZOLIB_FUNCNAME(lzo_get_osfhandle)(fd);
        LZO_COMPILE_TIME_ASSERT(sizeof(h) == sizeof(HANDLE))
        if (h != -1)
        {
            DWORD d = 0;
            if (GetConsoleMode(LZO_REINTERPRET_CAST(HANDLE, h), &d) == 0)
                return 0;
        }
    }
#endif
#if (HAVE_ISATTY)
    return (isatty(fd)) ? 1 : 0;
#else
    return 0;
#endif
}
LZOLIB_PUBLIC(int, lzo_mkdir) (const char* name, unsigned mode)
{
#if !(HAVE_MKDIR)
    LZO_UNUSED(name); LZO_UNUSED(mode);
    return -1;
#elif (LZO_ARCH_M68K && LZO_OS_TOS && (LZO_CC_PUREC || LZO_CC_TURBOC))
    LZO_UNUSED(mode);
    return Dcreate(name);
#elif (LZO_OS_DOS32 && LZO_CC_GNUC) && defined(__DJGPP__)
    return mkdir(name, mode);
#elif (LZO_OS_WIN32 && LZO_CC_GNUC) && defined(__PW32__)
    return mkdir(name, mode);
#elif ((LZO_OS_DOS16 || LZO_OS_DOS32) && (LZO_CC_HIGHC || LZO_CC_PACIFICC))
    LZO_UNUSED(mode);
    return mkdir(LZO_UNCONST_CAST(char *, name));
#elif (LZO_OS_DOS16 || LZO_OS_DOS32 || LZO_OS_OS2 || LZO_OS_OS216 || LZO_OS_WIN16 || LZO_OS_WIN32 || LZO_OS_WIN64)
    LZO_UNUSED(mode);
    return mkdir(name);
#elif (LZO_CC_WATCOMC)
    return mkdir(name, LZO_STATIC_CAST(mode_t, mode));
#else
    return mkdir(name, mode);
#endif
}
LZOLIB_PUBLIC(int, lzo_rmdir) (const char* name)
{
#if !(HAVE_RMDIR)
    LZO_UNUSED(name);
    return -1;
#elif ((LZO_OS_DOS16 || LZO_OS_DOS32) && (LZO_CC_HIGHC || LZO_CC_PACIFICC))
    return rmdir(LZO_UNCONST_CAST(char *, name));
#else
    return rmdir(name);
#endif
}
#if defined(lzo_int32e_t)
LZOLIB_PUBLIC(lzo_int32e_t, lzo_muldiv32s) (lzo_int32e_t a, lzo_int32e_t b, lzo_int32e_t x)
{
    lzo_int32e_t r = 0;
    if __lzo_likely(x != 0)
    {
#if defined(lzo_int64l_t)
        lzo_int64l_t rr = (LZO_ICONV(lzo_int64l_t, a) * b) / x;
        r = LZO_ITRUNC(lzo_int32e_t, rr);
#else
        LZO_UNUSED(a); LZO_UNUSED(b);
#endif
    }
    return r;
}
LZOLIB_PUBLIC(lzo_uint32e_t, lzo_muldiv32u) (lzo_uint32e_t a, lzo_uint32e_t b, lzo_uint32e_t x)
{
    lzo_uint32e_t r = 0;
    if __lzo_likely(x != 0)
    {
#if defined(lzo_int64l_t)
        lzo_uint64l_t rr = (LZO_ICONV(lzo_uint64l_t, a) * b) / x;
        r = LZO_ITRUNC(lzo_uint32e_t, rr);
#else
        LZO_UNUSED(a); LZO_UNUSED(b);
#endif
    }
    return r;
}
#endif
#if (LZO_OS_WIN16)
LZO_EXTERN_C void __far __pascal DebugBreak(void);
#endif
LZOLIB_PUBLIC_NOINLINE(void, lzo_debug_break) (void)
{
#if (LZO_OS_WIN16)
    DebugBreak();
#elif (LZO_ARCH_I086)
#elif (LZO_OS_WIN64) && (LZO_HAVE_WINDOWS_H)
    DebugBreak();
#elif (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    __asm__ __volatile__("int $3\n" : : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
#elif (LZO_ARCH_I386) && (LZO_ASM_SYNTAX_MSC)
    __asm { int 3 }
#elif (LZO_OS_WIN32) && (LZO_HAVE_WINDOWS_H)
    DebugBreak();
#else
    volatile lzo_intptr_t a = -1;
    * LZO_STATIC_CAST(volatile unsigned long *, LZO_REINTERPRET_CAST(volatile void *, a)) = ~0ul;
#endif
}
LZOLIB_PUBLIC_NOINLINE(void, lzo_debug_nop) (void)
{
}
LZOLIB_PUBLIC_NOINLINE(int, lzo_debug_align_check_query) (void)
{
#if (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
# if (LZO_ARCH_AMD64)
    lzo_uint64e_t r = 0;
# else
    size_t r = 0;
# endif
    __asm__ __volatile__("pushf\n pop %0\n" : "=a" (r) : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
    return LZO_ICONV(int, (r >> 18) & 1);
#elif (LZO_ARCH_I386) && (LZO_ASM_SYNTAX_MSC)
    unsigned long r;
    __asm {
        pushf
        pop eax
        mov r,eax
    }
    return LZO_ICONV(int, (r >> 18) & 1);
#else
    return -1;
#endif
}
LZOLIB_PUBLIC_NOINLINE(int, lzo_debug_align_check_enable) (int v)
{
#if (LZO_ARCH_AMD64) && (LZO_ASM_SYNTAX_GNUC)
    if (v) {
        __asm__ __volatile__("pushf\n orl $262144,(%%rsp)\n popf\n" : : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
    } else {
        __asm__ __volatile__("pushf\n andl $-262145,(%%rsp)\n popf\n" : : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
    }
    return 0;
#elif (LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    if (v) {
        __asm__ __volatile__("pushf\n orl $262144,(%%esp)\n popf\n" : : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
    } else {
        __asm__ __volatile__("pushf\n andl $-262145,(%%esp)\n popf\n" : : __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
    }
    return 0;
#elif (LZO_ARCH_I386) && (LZO_ASM_SYNTAX_MSC)
    if (v) { __asm {
        pushf
        or dword ptr [esp],262144
        popf
    }} else { __asm {
        pushf
        and dword ptr [esp],-262145
        popf
    }}
    return 0;
#else
    LZO_UNUSED(v); return -1;
#endif
}
LZOLIB_PUBLIC_NOINLINE(unsigned, lzo_debug_running_on_qemu) (void)
{
    unsigned r = 0;
#if (LZO_OS_POSIX_LINUX || LZO_OS_WIN32 || LZO_OS_WIN64)
    const char* p;
    p = __LZOLIB_FUNCNAME(lzo_getenv)(LZO_PP_STRINGIZE(LZO_ENV_RUNNING_ON_QEMU));
    if (p) {
        if (p[0] == 0) r = 0;
        else if ((p[0] >= '0' && p[0] <= '9') && p[1] == 0) r = LZO_ICAST(unsigned, p[0]) - '0';
        else r = 1;
    }
#endif
    return r;
}
LZOLIB_PUBLIC_NOINLINE(unsigned, lzo_debug_running_on_valgrind) (void)
{
#if (LZO_ARCH_AMD64 && LZO_ABI_ILP32)
    return 0;
#elif (LZO_ARCH_AMD64 || LZO_ARCH_I386) && (LZO_ASM_SYNTAX_GNUC)
    volatile size_t a[6];
    size_t r = 0;
    a[0] = 0x1001; a[1] = 0; a[2] = 0; a[3] = 0; a[4] = 0; a[5] = 0;
#  if (LZO_ARCH_AMD64)
    __asm__ __volatile__(".byte 0x48,0xc1,0xc7,0x03,0x48,0xc1,0xc7,0x0d,0x48,0xc1,0xc7,0x3d,0x48,0xc1,0xc7,0x33,0x48,0x87,0xdb\n" : "=d" (r) : "a" (&a[0]), "d" (r) __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
#  elif (LZO_ARCH_I386)
    __asm__ __volatile__(".byte 0xc1,0xc7,0x03,0xc1,0xc7,0x0d,0xc1,0xc7,0x1d,0xc1,0xc7,0x13,0x87,0xdb\n" : "=d" (r) : "a" (&a[0]), "d" (r) __LZO_ASM_CLOBBER_LIST_CC_MEMORY);
#  endif
    return LZO_ITRUNC(unsigned, r);
#else
    return 0;
#endif
}
#if (LZO_OS_WIN32 && LZO_CC_PELLESC && (__POCC__ >= 290))
#  pragma warn(pop)
#endif
#endif
#if defined(LZO_WANT_ACCLIB_WILDARGV)
#  undef LZO_WANT_ACCLIB_WILDARGV
#define __LZOLIB_WILDARGV_CH_INCLUDED 1
#if !defined(LZOLIB_PUBLIC)
#  define LZOLIB_PUBLIC(r,f)    r __LZOLIB_FUNCNAME(f)
#endif
#if (LZO_OS_DOS16 || LZO_OS_OS216 || LZO_OS_WIN16)
#if 0 && (LZO_CC_MSC)
LZO_EXTERN_C int __lzo_cdecl __setargv(void);
LZO_EXTERN_C int __lzo_cdecl _setargv(void);
LZO_EXTERN_C int __lzo_cdecl _setargv(void) { return __setargv(); }
#endif
#endif
#if (LZO_OS_WIN32 || LZO_OS_WIN64)
#if (LZO_CC_MSC && (_MSC_VER >= 1900))
#elif (LZO_CC_INTELC || LZO_CC_MSC)
LZO_EXTERN_C int __lzo_cdecl __setargv(void);
LZO_EXTERN_C int __lzo_cdecl _setargv(void);
LZO_EXTERN_C int __lzo_cdecl _setargv(void) { return __setargv(); }
#endif
#endif
#if (LZO_OS_EMX)
#define __LZOLIB_HAVE_LZO_WILDARGV 1
LZOLIB_PUBLIC(void, lzo_wildargv) (int* argc, char*** argv)
{
    if (argc && argv) {
        _response(argc, argv);
        _wildcard(argc, argv);
    }
}
#endif
#if (LZO_OS_CONSOLE_PSP) && defined(__PSPSDK_DEBUG__)
#define __LZOLIB_HAVE_LZO_WILDARGV 1
LZO_EXTERN_C int lzo_psp_init_module(int*, char***, int);
LZOLIB_PUBLIC(void, lzo_wildargv) (int* argc, char*** argv)
{
    lzo_psp_init_module(argc, argv, -1);
}
#endif
#if !(__LZOLIB_HAVE_LZO_WILDARGV)
#define __LZOLIB_HAVE_LZO_WILDARGV 1
LZOLIB_PUBLIC(void, lzo_wildargv) (int* argc, char*** argv)
{
#if 1 && (LZO_ARCH_I086PM)
    if (LZO_MM_AHSHIFT != 3) { exit(1); }
#elif 1 && (LZO_ARCH_M68K && LZO_OS_TOS && LZO_CC_GNUC) && defined(__MINT__)
    __binmode(1);
    if (isatty(1)) __set_binmode(stdout, 0);
    if (isatty(2)) __set_binmode(stderr, 0);
#endif
    LZO_UNUSED(argc); LZO_UNUSED(argv);
}
#endif
#endif

/* vim:set ts=4 sw=4 et: */
