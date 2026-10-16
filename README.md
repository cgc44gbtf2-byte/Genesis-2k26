# Genesis Reforged — maintenance source

Genesis Reforged is a legacy C/telnet, GodWars-derived MUD. This repository
contains source, area data, tests and maintenance documentation for building
and operating it on your own Linux host. It is not a web application, and
this update does not deploy or modify a running server.

## Connect

**Address:** `159.223.164.151` · **Port:** `4000`

Connect with a MUD client, or:

```sh
telnet 159.223.164.151 4000
```

This is the supplied connection address, not a guarantee of verified uptime.
The [MUD listing draft](genesis-maintenance/MUD_LISTING.txt) still requires
confirmation of the server's PK, roleplaying and live/testing policies.

## Current gameplay scope

There are 19 standalone parent classes:

Angel, Cyborg, Deathknight, Demon, Dragon, Drow, Elemental, Gargoyle,
Highlander, Kuei-jin, Mage, Monk, Moogle, Ninja, Phoenix, Sorcerer,
Vampire, Werewolf and Wraith.

- Angel, unified Demon and Cyborg have 33, 71 and 60 implemented catalogue
  entries respectively. Counts include variants and management systems,
  not guaranteed one-to-one upstream powers.
- Phoenix has eight Genesis-native powers. Gargoyle, Kuei-jin, Deathknight,
  Moogle and Wraith have independent kits and progression/state handling.
- Sorcerer book mages have a one-time instructional starter grimoire with
  Spark, Mend and Ward. Chaos book pages reroll colors while preserving
  their spell role; Chaos is a variant, not a twentieth parent class.
- Legacy spell, skill, stance, cost, travel and persistence repairs are
  included, with expanded exact-topic help.

**Hybrids are preview-only.** All 171 named two-parent combinations have
stable registry identities, but conversion, combined inheritance, pure-class
ascension and respec are not implemented. The future design grants both
parents' abilities; it supersedes the earlier 75%-selection proposal.
Do not advertise playable hybrids or ascension as available features.

See the [class guide](genesis-maintenance/CLASS_GUIDE.md),
[kit status](genesis-maintenance/CLASS_KIT_STATUS.md),
[Cyborg kit](genesis-maintenance/CYBORG_KIT.md),
[book-mage guide](genesis-maintenance/BOOK_MAGE_STARTER.md) and
[hybrid roster](genesis-maintenance/HYBRID_ROSTER.md).

## Build on Debian or Ubuntu

Install the build dependencies:

```sh
sudo apt update
sudo apt install build-essential libcrypt-dev python3 unzip git
```

Clone and build:

```sh
git clone https://github.com/cgc44gbtf2-byte/Genesis-MUD-maintenance.git
cd Genesis-MUD-maintenance
# Before this maintenance PR is merged, switch to its source branch:
# git switch maintenance/class-kits-help-2026-10-15
cd genesis-maintenance
sha256sum --check MANIFEST.sha256
make -C src
PORT=4000 ./run.sh
```

The compiler uses GNU89 compatibility. The launcher runs in the foreground,
loads area files relative to `area/`, and creates missing runtime directories.
Use an unprivileged account and a process supervisor on your own host.
`make -C src clean` removes generated binaries and objects.

Before public registration, secure the exact case-insensitive **Admin**
account and follow [ADMIN_BOOTSTRAP.md](genesis-maintenance/ADMIN_BOOTSTRAP.md).
Only that identity may use the highest-rank `relevel` bootstrap.
Back up live player records and mutable area files before upgrading; never
replace an existing live data directory with a clean source checkout.

## Verification and remaining limits

The current gameplay source passed:

- 13 linked AddressSanitizer/UndefinedBehaviorSanitizer regression suites.
- Seven isolated native gameplay groups: runtime smoke, combat/system fixes,
  legacy spells, reconstructed abilities, maintenance fixes, crafted classes
  and class abilities.
- A strict GNU89 build and an independent build from the extracted archive.
- A 398/398 exact-topic legacy help audit and a native help lookup probe
  covering 266 entries / 583 exact topics and aliases, plus two semantic checks.
- Archive CRC, safe paths, source-only content, byte equality and SHA256
  manifest checks.

The validation environment linked its installed crypt shared library
explicitly. On Debian/Ubuntu, install `libcrypt-dev` for the default `-lcrypt`
link used by the Makefile.

The compiler inventory contains 385 active scoped legacy handlers with no
empty function bodies. This is **not** proof that all gameplay variants work:
225 handlers have no test mentions, and the full legacy effect/argument,
save-state and balance matrix remains open. No comprehensive production
security or PvP-balance certification is claimed.

For reproducible scope and commands, see
[LEGACY_VERIFICATION.md](genesis-maintenance/LEGACY_VERIFICATION.md) and
[CLASS_KIT_STATUS.md](genesis-maintenance/CLASS_KIT_STATUS.md).
From `genesis-maintenance/`, the linked suite can be run with:

```sh
sh tests/test-combat-system.sh all
python3 tests/audit_ability_help.py
```

Know Alignment reads NPC prototype alignment; player alignment has not been
restored. Frenzy provides rage messaging and a modest +1 hitroll/+1 damroll
buff. Darkblessing supports evil NPCs only, with +1 hitroll/+1 damroll/+10
armor; NPC avoidance remains level-based. Ventriloquate is restored as safe,
attributed object-based speech. Existing level-99 spell restrictions remain.

## Source downloads

The current documentation-synced source package is
[Genesis-MUD-GitHub-maintenance-source.zip](Genesis-MUD-GitHub-maintenance-source.zip).
It contains 229 files, including a manifest covering 228 payloads. Build
outputs, runtime logs, player records and private notes are excluded.

SHA256: `40fde68c26f3188d09a2203c0048633cd94c7a450190245ba52d290f0f439c09`

The earlier [Genesis-MUD-maintenance-updated.zip](Genesis-MUD-maintenance-updated.zip)
is retained unchanged as the original maintenance-only baseline. It predates
the expanded standalone class kits, help and hybrid roster preview.

## Upstream permissions and hosting precautions

The source contains an upstream God Wars notice requiring permission to run
or redistribute the code. The supplied archive did not include the referenced
license documents. Verify the necessary permissions before redistribution
or public operation; this repository is not a blanket open-source license grant.
Preserve upstream notices and reference-attribution documentation.

Legacy password hashing, networking and memory management have not been
comprehensively modernized or audited. Stage upgrades with disposable accounts,
test backups and restoration, and review your host's firewall and process
supervision before inviting players.
