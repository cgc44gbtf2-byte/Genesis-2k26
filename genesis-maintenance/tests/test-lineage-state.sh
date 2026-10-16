#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CC:-cc} -O1 -g -std=gnu89 -Wall -Werror -fsanitize=address,undefined \
    -fno-omit-frame-pointer -fno-pie -no-pie \
    "$root/tests/test_lineage_state.c" "$root/src/lineage_state.c" "$root/src/reference_state.c" \
    -o "$tmp/state"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/state"
