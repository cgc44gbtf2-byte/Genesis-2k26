#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc=${CC:-cc}
for file in magic act_info handler; do
    "$cc" -O2 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch \
        -DMERC -Dunix -ffunction-sections -fdata-sections \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        -c "$root/src/$file.c" -o "$tmp/$file.o"
done
"$cc" -O2 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch \
    -fsanitize=address,undefined -Wl,--gc-sections \
    "$root/tests/test_resolve.c" "$tmp/magic.o" "$tmp/act_info.o" "$tmp/handler.o" \
    -o "$tmp/test-resolve"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test-resolve"
for test in mage_cost monk_cost; do
"$cc" -O2 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch \
    -DMERC -Dunix -ffunction-sections -fdata-sections \
    -fsanitize=address,undefined -Wl,--gc-sections \
    "$root/tests/test_$test.c" -o "$tmp/test-$test"
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/test-$test"
done
