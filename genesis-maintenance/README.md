# Genesis MUD maintenance copy

This is a legacy C/telnet MUD server and its area data. It is not a web app.
This maintenance copy intentionally excludes the original compiled binary,
object files, server logs, player backups, and private notes.

## Class kits and current verification

The roster now has **19 standalone parent identities**: the fourteen described
below plus Gargoyle, Kuei-jin, Deathknight, Moogle and Wraith. The five added
kits provide 113 active commands, independent progression, bounded resource
records and native combat/visibility/movement hooks. See `CLASS_GUIDE.md`
for every class and complete new-kit command lists. These are conservative
Genesis conversions/designs, not a promise of literal upstream parity or
final PvP balance.

This source includes playable standalone Angel, Cyborg and one Demon, plus
Phoenix and the original ten classes. Use `autoclass angel|cyborg|demon` after
meeting the existing awake and new-class base-stat requirements, then
`angel`, `cyborg` or `demon` for commands and costs. The expanded inventory
has 33 Angel, 71 Demon and 60 Cyborg entries implemented; these counts include
variants and management systems, not a finalized selectable-ability quota.
See `CLASS_KIT_STATUS.md`, `CLASS_GUIDE.md` and `CYBORG_KIT.md` for current
commands, evidence and limitations. Historical implementation notes describe
earlier stages; they are not the current completion checklist.

Phoenix has eight independently designed powers and its own mastery track;
see `PHOENIX_IMPLEMENTATION.md`. Existing class IDs and saved power arrays
are not renumbered. Cyborg uses optional, bounded version-1 records; old
player files remain readable.

The `hybrid` command is still a read-only parent/inventory preview.
Playable hybrids, pure-class ascension and path respecs are **not implemented**.
The current future design is two parents with all of both kits, not the
superseded 75%-selection proposal. All 171 named pairs have stable registry
identities and read-only previews; see `HYBRID_ROSTER.md`.
See `PARENT_FRAMEWORK.md` for the foundation and remaining work.
The finalized maintenance-only ZIP remains a separate, unchanged release.

The current maintenance update passes 19 linked ASan/UBSan suites with strict
GNU89 and `-Werror`, including exact training-price/deduction checks and
116 read-only older-menu views. See `COMBAT_STYLE_AND_MENUS.md`.
The maintenance baseline also passed seven isolated native gameplay groups
and a fresh-extraction build; those older ZIP snapshots are not replaced.
The help audit covers 398/398 registered legacy topics. The native help probe
checked 266 entries / 583 exact topics and aliases, plus semantic checks for
the Elemental and Abilitystatus pages. These are scoped checks, not a promise
that every legacy argument variant, balance rule or saved-state combination
has been tested. The broader legacy matrix remains open; see
`LEGACY_VERIFICATION.md` and `CLASS_KIT_STATUS.md`.

## Build

On Linux, install a C compiler, GNU Make, and the system development package
that provides `crypt(3)` (commonly `libcrypt-dev` on Debian/Ubuntu).

```sh
make -C src
```

The source uses GNU C89 extensions to retain compatibility with its original
style. The build is incremental; `make -C src clean` removes generated build
files.

## Runtime maintenance checks

This pass fixes wide-range random-number overflow, caps mob health before
narrowing squared levels, and corrects an out-of-bounds value copy in forging.
Character saves now use a temporary file and replace the destination only after
a successful write. Save failures no longer double-close a reserved file handle,
and the `save` command no longer claims success for failures or ineligible
sleeper characters. The existing awakening requirement is unchanged.

Run the random-number regression tests with GCC's sanitizers:

```sh
sh tests/test-random.sh
```

For the runtime smoke test, first build the server, then run:

```sh
python3 tests/smoke_runtime.py
```

The smoke test uses temporary copies of the area data and disposable characters;
it does not use your player or backup directories. It covers character creation,
common information commands, saving/reloading, and unavailable save directories.
It requires Python 3 and the system crypt library. If the library is installed
outside the normal loader search path, set `CRYPT_LIBRARY` to its absolute path.

To run it with AddressSanitizer and UndefinedBehaviorSanitizer enabled:

```sh
make -C src clean
make -C src \
  CFLAGS="-O2 -g -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch -fsanitize=address,undefined -fno-omit-frame-pointer" \
  LDFLAGS="-fsanitize=address,undefined"
python3 tests/smoke_runtime.py
```

Clean again before switching back to a normal build. These are focused regression
checks, not a complete gameplay or security audit.

Run all 19 linked gameplay/network sanitizer probes in one isolated build, plus the
separate component-codec and resolve probes:

```sh
sh tests/test-combat-system.sh all
sh tests/test-cyborg-core.sh
sh tests/test-resolve.sh
```

The extended class suite also exercises real login persistence and class
clearing for Angel/Demon physical effects and automatic passive bonuses.

## Class ability restoration

The recoverable class abilities have been repaired and connected to the
interpreter. See `ABILITY_RESTORATION.md` for the changes, safety restrictions,
and the newly reconstructed mechanics. In game, use `help abilitystatus` and
`help reconstructedabilities`. The eight reconstructed powers use new rules,
not recovered original balance; see `ABILITY_RECONSTRUCTION.md`.
This is not a claim that all class abilities or combat balance have been tested.

With the server built, run the isolated class-ability regression test:

```sh
python3 tests/class_abilities.py
python3 tests/maintenance_fixes.py
sh tests/test-resolve.sh
```

Like the runtime smoke test, this needs Python 3 and the system crypt library
and accepts `CRYPT_LIBRARY`. It creates disposable characters and a temporary
copy of the area data, including temporary changes to a room's safety flags.
It never changes your live area or player files.
The class suite can take several minutes while combat timers and normal effect
expiry run. The resolve test requires GCC-compatible sanitizer and linker flags.

The nine-finding repair batch is documented in `MAINTENANCE_FIXES.md`,
including Mage cost rules, saved-status migration, shared stat/save changes
and external upgrade precautions. The maintenance regression test exercises
the fixes with disposable accounts, including a real save/restart/expiry cycle.

`COMBAT_SYSTEM_FIXES.md` documents the follow-up skill, spell and stance
repairs, including newly active item procs and intentionally deferred legacy
item spells at that point in the maintenance history. Run its isolated C
and live-server regressions with:

```sh
sh tests/test-combat-system.sh
python3 tests/system_fixes.py
```

The subsequent `LEGACY_SPELL_RESTORATION.md` documents 20 reconstructed
item/NPC spell effects, restored spell-pill activation, green-potion
targeting, and saved door-barrier loading. The final three are NPC-only
Know Alignment, a small timed Frenzy offensive buff, and Darkblessing for evil
NPCs only. Darkblessing applies +1 hitroll, +1 damroll and +10 armor according
to Genesis's positive-armor convention; NPC avoidance remains level-based.
Ventriloquate now provides safe, attributed object-based speech. Spell indices,
slots and level-99 casting restrictions are unchanged. The hybrid command is
a read-only preview, not playable multiclassing.
Run its focused sanitizer and disposable live-server tests with:

```sh
sh tests/test-combat-system.sh legacy
python3 tests/legacy_spells.py
```

## Connect

The supplied replacement IPv6 address is **2604:a880:400:d1:0:5:331d:4001**,
port **4000**.
Use a MUD client or:

```sh
telnet -6 2604:a880:400:d1:0:5:331d:4001 4000
```

This documents the supplied endpoint; it is not a claim of verified uptime,
server configuration or deployment. See `MUD_LISTING.txt` for the listing draft.
The server supports a Linux dual-stack IPv6/IPv4 listener, with an explicit
IPv4 fallback where IPv6 is unavailable. No source-level IP setting is needed.
An IPv6-only public address requires IPv6 connectivity from the player's network.

## Staff balance editing and IPv6 details

Use `pset Player currencies` for a read-only resource table, or for example
`pset Player classpoints 50000` / `pset Player exp 5000000` for absolute
balances. Native class resources are class-gated; edits do not grant mastery.
Staff permissions, bounds, persistence and local IPv6-test limitations are
documented in `IPV6_AND_PSET.md`; in-game staff can use `help pset`.

## Run locally or on your own host

```sh
PORT=4000 ./run.sh
```

The server stays in the foreground so a process manager can supervise it.
The launcher creates the player, backup, and log directories if they are missing.
It loads area files relative to `area/` and stores player records in `player/`.
The default port is 4000; choose an unprivileged port above 1024. Back up
player records and mutable area files before upgrades.
Run as an unprivileged account. Before allowing public registrations, follow
`ADMIN_BOOTSTRAP.md`: the exact case-insensitive name Admin is the only
highest-rank `relevel` bootstrap identity and must be secured.

## Before public hosting

The source contains an upstream God Wars notice stating that running or
redistributing the code requires permission. The archive did not include the
referenced license documents, so verify that you have permission before
publishing or operating a public server. This cleanup does not modernize the
game's password hashing or audit the legacy networking and memory-management
code for production security.
