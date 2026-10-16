#!/bin/sh
# Link the real game modules into the focused combat/skill/spell probe.
# Build in isolation so the user's normal server binary and player data stay untouched.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_names="test_combat_system"
if [ "$#" -eq 1 ] && [ "${1-}" = legacy ]; then
    test_names="test_legacy_spells"
elif [ "$#" -eq 1 ] && [ "${1-}" = parents ]; then
    test_names="test_parents"
elif [ "$#" -eq 1 ] && [ "${1-}" = phoenix ]; then
    test_names="test_phoenix"
elif [ "$#" -eq 1 ] && [ "${1-}" = newclasses ]; then
    test_names="test_newclasses"
elif [ "$#" -eq 1 ] && [ "${1-}" = crafted ]; then
    test_names="test_crafted"
elif [ "$#" -eq 1 ] && [ "${1-}" = lineages ]; then
    test_names="test_lineages"
elif [ "$#" -eq 1 ] && [ "${1-}" = reference ]; then
    test_names="test_reference_completion"
elif [ "$#" -eq 1 ] && [ "${1-}" = variants ]; then
    test_names="test_legacy_variants"
elif [ "$#" -eq 1 ] && [ "${1-}" = mage ]; then
    test_names="test_mage_variants"
elif [ "$#" -eq 1 ] && [ "${1-}" = books ]; then
    test_names="test_sorcerer_starter"
elif [ "$#" -eq 1 ] && [ "${1-}" = cyborg ]; then
    test_names="test_cyborg_completion"
elif [ "$#" -eq 1 ] && [ "${1-}" = relevel ]; then
    test_names="test_relevel"
elif [ "$#" -eq 1 ] && [ "${1-}" = all ]; then
    # Compile the large server once; each probe still runs in its own process.
    test_names="test_combat_system test_legacy_spells test_parents test_phoenix test_newclasses test_crafted test_lineages test_reference_completion test_legacy_variants test_mage_variants test_sorcerer_starter test_cyborg_completion test_relevel"
elif [ "$#" -ne 0 ]; then
    echo "Usage: sh tests/test-combat-system.sh [legacy|parents|phoenix|newclasses|crafted|lineages|reference|variants|mage|books|cyborg|relevel|all]" >&2
    exit 2
fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc=${CC:-cc}
crypt=${CRYPT_LIBRARY:--lcrypt}
flags='-O1 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie'
cp -a "$root/src" "$tmp/src"
make -C "$tmp/src" clean >/dev/null
make -C "$tmp/src" -j2 CC="$cc" CFLAGS="$flags" \
    LDFLAGS='-fsanitize=address,undefined -no-pie' LDLIBS="$crypt" >/dev/null
"$cc" $flags -DMERC -Dunix -DMEMWATCH -Dmain=genesis_server_main \
    -c "$tmp/src/comm.c" -o "$tmp/comm-probe.o"
set --
for obj in "$tmp/src/"*.o; do
    [ "$obj" = "$tmp/src/comm.o" ] || set -- "$@" "$obj"
done
status=0
for test_name in $test_names; do
    echo "=== $test_name (ASan/UBSan) ==="
    if ! "$cc" $flags -DMERC -Dunix -DMEMWATCH -no-pie \
        "$root/tests/$test_name.c" "$tmp/comm-probe.o" "$@" "$crypt" \
        -o "$tmp/$test_name"; then
        status=1
        continue
    fi
    if ! ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 "$tmp/$test_name"; then
        status=1
    fi
done
exit "$status"
