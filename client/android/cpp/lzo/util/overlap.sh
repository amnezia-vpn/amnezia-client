#! /bin/sh
set -e

#
# usage: util/overlap.sh [directory]
#
# This script runs the overlap example program
# on a complete directory tree.
#
# Copyright (C) 1996-2017 Markus Franz Xaver Johannes Oberhumer
#

OVERLAP="./examples/overlap"
for d in ./examples .; do
    for ext in "" .exe .out; do
        if test -f "$d/overlap$ext" && test -x "$d/overlap$ext"; then
            OVERLAP="$d/overlap$ext"
            break 2
        fi
    done
done

dir="${1-.}"

TMPFILE="/tmp/lzo_$$.tmp"
rm -f "$TMPFILE"
(find "$dir/." -type f -print0 | LC_ALL=C sort -z > "$TMPFILE") || true

cat "$TMPFILE" | xargs -0 -r "$OVERLAP"

rm -f "$TMPFILE"
echo "Done."
exit 0

# vim:set ts=4 sw=4 et:
