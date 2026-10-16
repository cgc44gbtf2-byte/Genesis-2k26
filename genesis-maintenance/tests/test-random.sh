#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc=${CC:-cc}
"$cc" -O2 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch \
    -DMERC -Dunix -ffunction-sections -fdata-sections \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -c "$root/src/db.c" -o "$tmp/db.o"
"$cc" -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -Wl,--gc-sections "$root/tests/test_random.c" "$tmp/db.o" -o "$tmp/test-random"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test-random"
