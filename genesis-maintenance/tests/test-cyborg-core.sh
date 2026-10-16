#!/bin/sh
# Independent backend probe: no real player files, server port or class grants.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CC:-cc} -std=gnu89 -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie \
    "$root/src/cyborg_core.c" "$root/tests/test_cyborg_core.c" -o "$tmp/probe"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
    "$tmp/probe"
