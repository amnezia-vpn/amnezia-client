#! /bin/sh
set -e

#
# usage: util/checkasm.sh [directory]
#
# This script runs lzotest with all assembler decompressors
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

LFLAGS="-q"

for m in 11; do
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -A
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -A -S
done

for m in 61; do
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -F
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -F -S
done

for m in 71 81; do
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -A
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -A -S
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -F
    cat "$TMPFILE" | "$LZOTEST" -m${m} -@ $LFLAGS -F -S
done

rm -f "$TMPFILE"
echo "Done."
exit 0

# vim:set ts=4 sw=4 et:
