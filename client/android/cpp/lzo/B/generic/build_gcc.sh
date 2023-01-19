#! /bin/sh
## vim:set ts=4 sw=4 et:
set -e
echo "// Using GNU C compiler."
echo "//"

test "X${top_srcdir}" = X && top_srcdir=`echo "$0" | sed 's,[^/]*$,,'`../..
UNAME_MACHINE=unknown

CC="gcc -fPIC"
CC="gcc -static"
CC="gcc"
CFLAGS="-Wall -O2 -fomit-frame-pointer"

# delete the next line to disable assembler support
UNAME_MACHINE=`(uname -m) 2>/dev/null` || UNAME_MACHINE=unknown
case $UNAME_MACHINE in
    i[34567]86)
        CC="$CC -m32"
        CPPFLAGS="-DLZO_USE_ASM"
        LZO_EXTRA_SOURCES=$top_srcdir/asm/i386/src_gas/*.S
        ;;
esac

. $top_srcdir/B/generic/build.sh
