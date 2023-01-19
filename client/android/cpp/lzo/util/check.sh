#! /bin/sh
set -e

#
# usage: util/check.sh [directory]
#
# This script runs lzotest with all algorithms
# on a complete directory tree.
# It is not suitable for accurate timings.
#
# Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
#

if test "X$LZOTEST" = X; then
LZOTEST="./lzotest/lzotest"
for d in ./lzotest .; do
    for ext in "" .exe .out; do
        if test -f "$d/lzotest$ext" && test -x "$d/lzotest$ext"; then
            LZOTEST="$d/lzotest$ext"
            break 2
        fi
    done
done
fi

dir="${1-.}"

TMPFILE="/tmp/lzotest_$$.tmp"
rm -f "$TMPFILE"
(find "$dir/." -type f -print | LC_ALL=C sort > "$TMPFILE") || true

## methods=`"$LZOTEST" -m | sed -n 's/^ *-m\([0-9]*\).*/\1/p'`
## methods="9721 9722 9723 9724 9725 9726 9727 9728 9729"
methods="21 31 1 2 3 4 5 6 7 8 9 11 12 13 14 15 16 17 18 19 61 71 81"
methods="$methods 111 112 115"
methods="$methods 921 931 901 911"
methods="$methods 902 912 942 962 972 982 992"
##methods="71 972"
##methods="1101 1102 1103 1104 1105 1106 1107"

LFLAGS="-q -T -n2 -S"
LFLAGS="-q -T -n2"

for m in $methods; do
    cat "$TMPFILE" | "$LZOTEST" "-m$m" -@ $LFLAGS
done

rm -f "$TMPFILE"
echo "Done."
exit 0

# vim:set ts=4 sw=4 et:
