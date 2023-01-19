#
# CMakeLists.txt --- a "CMake" file for building LZO
#
# This file is part of the LZO data compression library.
#   http://www.oberhumer.com/opensource/lzo/
#
# Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
# All Rights Reserved.
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

#
# simple usage example (Unix):
#     mkdir -p build && cd build && cmake .. && make
#
# simple usage example (Windows MSVC):
#     md build
#     cd build
#     cmake .. -G "NMake Makefiles"
#     nmake
#
# another usage example (Unix):
#     mkdir -p build/release-i686
#     cd       build/release-i686
#     cmake ../.. -DENABLE_STATIC=0 -DENABLE_SHARED=1 \
#         -DCMAKE_C_COMPILER=gcc -DCMAKE_C_FLAGS="-m32 -march=i686" \
#         -DCMAKE_INSTALL_PREFIX=/opt/local/prefix-i686
#     make VERBOSE=1
#     make install
#
# see http://www.cmake.org/ for more info
#

# /***********************************************************************
# // init
# ************************************************************************/

# Disallow in-source builds. Note that you will still have to manually
# clean up a few files if you accidentally try an in-source build.
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
if(",${CMAKE_SOURCE_DIR}," STREQUAL ",${CMAKE_BINARY_DIR},")
    message(FATAL_ERROR "ERROR: In-source builds are not allowed.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

project(lzo VERSION 2.10 LANGUAGES C)

# configuration options
option(ENABLE_STATIC "Build static LZO library." ON)
option(ENABLE_SHARED "Build shared LZO library." OFF)
if(NOT ENABLE_STATIC AND NOT ENABLE_SHARED)
    set(ENABLE_STATIC ON)
endif()
if(ENABLE_SHARED AND WIN32)
    cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR) # needed for WINDOWS_EXPORT_ALL_SYMBOLS
endif()

# install directories
if(NOT CMAKE_INSTALL_PREFIX)
    message(FATAL_ERROR "ERROR: CMAKE_INSTALL_PREFIX is not defined.")
endif()
include(FindPkgConfig QUIET)
include(GNUInstallDirs)

# /***********************************************************************
# // targets
# ************************************************************************/

file(GLOB lzo_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
list(SORT lzo_SOURCES)

# LZO library
if(NOT DEFINED lzo_USE_OBJECT_LIB)
    if(ENABLE_STATIC AND ENABLE_SHARED)
        if(CMAKE_POSITION_INDEPENDENT_CODE OR MSVC)
            # all objects are PIC on this system => can share objects
            set(lzo_USE_OBJECT_LIB 1)
        endif()
    endif()
endif()
set(lzo_COMPILE_TARGETS )
if(lzo_USE_OBJECT_LIB)
    add_library(lzo_object_lib OBJECT ${lzo_SOURCES})
    list(APPEND lzo_COMPILE_TARGETS lzo_object_lib)
    set_property(TARGET lzo_object_lib PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
if(ENABLE_STATIC)
    if(lzo_USE_OBJECT_LIB)
        add_library(lzo_static_lib STATIC $<TARGET_OBJECTS:lzo_object_lib>)
    else()
        add_library(lzo_static_lib STATIC ${lzo_SOURCES})
        list(APPEND lzo_COMPILE_TARGETS lzo_static_lib)
    endif()
    set_target_properties(lzo_static_lib PROPERTIES OUTPUT_NAME lzo2)
endif()
if(ENABLE_SHARED)
    if(lzo_USE_OBJECT_LIB)
        add_library(lzo_shared_lib SHARED $<TARGET_OBJECTS:lzo_object_lib>)
    else()
        add_library(lzo_shared_lib SHARED ${lzo_SOURCES})
        list(APPEND lzo_COMPILE_TARGETS lzo_shared_lib)
    endif()
    set_target_properties(lzo_shared_lib PROPERTIES OUTPUT_NAME lzo2)
    if(ENABLE_STATIC)
        # avoid name clash between static lzo2.lib and DLL import-lib lzo2.lib
        set_target_properties(lzo_shared_lib PROPERTIES ARCHIVE_OUTPUT_NAME lzo2_dllimport)
    endif()
    set_target_properties(lzo_shared_lib PROPERTIES SOVERSION 2 VERSION 2.0.0) # ABI version is 2.0.0 !
    set_target_properties(lzo_shared_lib PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# tests & examples
macro(lzo_add_executable t)
    add_executable(${t} ${ARGN})
    if(ENABLE_STATIC)
        target_link_libraries(${t} lzo_static_lib)
    else()
        target_link_libraries(${t} lzo_shared_lib)
        if(DEFINED CMAKE_INSTALL_FULL_LIBDIR)
            set_target_properties(${t} PROPERTIES INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
        endif()
    endif()
endmacro()
# main test driver
lzo_add_executable(lzotest  lzotest/lzotest.c)
# examples
lzo_add_executable(dict     examples/dict.c)
lzo_add_executable(lzopack  examples/lzopack.c)
lzo_add_executable(overlap  examples/overlap.c)
lzo_add_executable(precomp  examples/precomp.c)
lzo_add_executable(precomp2 examples/precomp2.c)
lzo_add_executable(simple   examples/simple.c)
# some boring internal test programs
if(0)
    lzo_add_executable(align    tests/align.c)
    lzo_add_executable(chksum   tests/chksum.c)
    lzo_add_executable(promote  tests/promote.c)
    lzo_add_executable(sizes    tests/sizes.c)
endif()

# miniLZO
if(1)
    add_executable(testmini minilzo/testmini.c minilzo/minilzo.c)
    target_include_directories(testmini PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/lzo") # needed for "lzoconf.h"
endif()

# /***********************************************************************
# // compilation flags
# // this section currently mostly matches the Autoconf version
# ************************************************************************/

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

if(MSVC)
    # disable silly warnings about using "deprecated" POSIX functions like fopen()
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Checks for header files
macro(mfx_check_include_file f var)
    check_include_file("${f}" "mfx_${var}")
    if(NOT ",${mfx_${var}}," STREQUAL ",,")
        add_definitions(-D${var}=1)
        set(mfx_${var} 1)
    else()
        set(mfx_${var} 0)
    endif()
endmacro()
# mfx_ACC_CHECK_HEADERS
set(l assert.h ctype.h dirent.h errno.h fcntl.h float.h limits.h malloc.h memory.h setjmp.h signal.h stdarg.h stddef.h stdint.h stdio.h stdlib.h string.h strings.h time.h unistd.h utime.h sys/mman.h sys/resource.h sys/stat.h sys/time.h sys/types.h sys/wait.h)
foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_include_file("${f}" "HAVE_${var}")
endforeach()

# Checks for typedefs and structures
macro(mfx_check_type_size type var)
    check_type_size("${type}" "mfx_${var}")
    if("${mfx_${var}}" MATCHES "^[1-9][0-9]*$")
        add_definitions(-D${var}=${mfx_${var}})
    else()
        set(mfx_${var} 0)
    endif()
endmacro()
# mfx_ACC_CHECK_SIZEOF + mfx_CHECK_SIZEOF
set(l short int long "long long" __int16 __int32 __int64 "void *" size_t ptrdiff_t intmax_t uintmax_t intptr_t uintptr_t float double "long double" dev_t fpos_t mode_t off_t ssize_t time_t)
foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE " \\*" "_P" var "${var}")
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_type_size("${f}" "SIZEOF_${var}")
endforeach()

# Checks for library functions
macro(mfx_check_function_exists func var)
    check_function_exists("${func}" "mfx_${var}")
    if(NOT ",${mfx_${var}}," STREQUAL ",,")
        add_definitions(-D${var}=1)
        set(mfx_${var} 1)
    else()
        set(mfx_${var} 0)
    endif()
endmacro()
# mfx_ACC_CHECK_FUNCS
set(l access alloca atexit atoi atol chmod chown clock_getcpuclockid clock_getres clock_gettime ctime difftime fstat getenv getpagesize getrusage gettimeofday gmtime isatty localtime longjmp lstat memcmp memcpy memmove memset mkdir mktime mmap mprotect munmap qsort raise rmdir setjmp signal snprintf strcasecmp strchr strdup strerror strftime stricmp strncasecmp strnicmp strrchr strstr time umask utime vsnprintf)
foreach(f ${l})
    string(TOUPPER "${f}" var)
    string(REGEX REPLACE "[^0-9A-Z_]" "_" var "${var}")
    mfx_check_function_exists("${f}" "HAVE_${var}")
endforeach()

# mfx_LZO_CHECK_ENDIAN
TEST_BIG_ENDIAN(big_endian)
if ("${big_endian}" MATCHES "^1$")
    add_definitions(-DLZO_ABI_BIG_ENDIAN=1)
elseif ("${big_endian}" MATCHES "^0$")
    add_definitions(-DLZO_ABI_LITTLE_ENDIAN=1)
else()
    message(FATAL_ERROR "ERROR: TEST_BIG_ENDIAN failed with result '${big_endian}'.")
endif()

# LZO_HAVE_CONFIG_H
add_definitions(-DLZO_CFG_NO_CONFIG_HEADER=1)

# warnings
foreach(t ${lzo_COMPILE_TARGETS})
    if(CMAKE_C_COMPILER_ID MATCHES "^(Clang|GNU)$")
        target_compile_options(${t} PRIVATE -Wall -W -Wcast-qual)
    endif()
endforeach()

# /***********************************************************************
# // "make test"
# ************************************************************************/

include(CTest)
add_test(NAME simple     COMMAND simple)
add_test(NAME testmini   COMMAND testmini)
add_test(NAME lzotest-01 COMMAND lzotest -mlzo   -n2  -q "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
add_test(NAME lzotest-02 COMMAND lzotest -mavail -n10 -q "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
add_test(NAME lzotest-03 COMMAND lzotest -mall   -n10 -q "${CMAKE_CURRENT_SOURCE_DIR}/include/lzo/lzodefs.h")

# /***********************************************************************
# // "make install"
# ************************************************************************/

if(DEFINED CMAKE_INSTALL_FULL_LIBDIR)

set(f AUTHORS COPYING NEWS THANKS doc/LZO.FAQ doc/LZO.TXT doc/LZOAPI.TXT)
install(FILES ${f} DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}")

set(f include/lzo/lzo1.h include/lzo/lzo1a.h include/lzo/lzo1b.h
    include/lzo/lzo1c.h include/lzo/lzo1f.h include/lzo/lzo1x.h
    include/lzo/lzo1y.h include/lzo/lzo1z.h include/lzo/lzo2a.h
    include/lzo/lzo_asm.h include/lzo/lzoconf.h include/lzo/lzodefs.h
    include/lzo/lzoutil.h
)
install(FILES ${f} DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/lzo")

if(ENABLE_STATIC)
    install(TARGETS lzo_static_lib ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
endif()
if(ENABLE_SHARED)
    install(TARGETS lzo_shared_lib
        ARCHIVE DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}"
    )
endif()

if(1)
    set(f lzopack lzotest simple testmini) # examples
    install(TARGETS ${f} DESTINATION "${CMAKE_INSTALL_FULL_LIBEXECDIR}/lzo/examples")
endif()

if(PKG_CONFIG_FOUND)
    configure_file(lzo2.pc.cmakein lzo2.pc @ONLY)
    #if(EXISTS "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lzo2.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
    #endif()
endif()

endif() # CMAKE_INSTALL_FULL_LIBDIR

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.maint.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.maint.txt")
endif()

# finally print some info about the build-type
if(CMAKE_CONFIGURATION_TYPES)
message(STATUS "CMAKE_CONFIGURATION_TYPES = ${CMAKE_CONFIGURATION_TYPES}")
endif()
if(CMAKE_BUILD_TYPE)
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
endif()

# vim:set ft=cmake ts=4 sw=4 tw=0 et:
