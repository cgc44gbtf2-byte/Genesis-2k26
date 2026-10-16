# Combat-style repair and class-menu organization

## Reported unclassed combat failure

`kill lizard` reported:

```text
Erm...'Oh Shit(unarmed)' bug alert. Inform Akuma.
```

The matching branch in `multi_hit()` refuses an empty main hand when the
cached selected style expects a weapon. This is a style/equipment mismatch,
not a requirement to choose a class. The precise event that left the live
character's style stale has not been established from server state.

`multi_hit()` now calls the existing `update_style()` for players before
reading the selected style and choosing attacks. This covers both command
entry and later combat rounds. Valid two-handed grip, shields, offhand weapon
promotion and natural weapons continue to use the existing style rules.
The null-character guard also now precedes the selected-style dereference.
NPC attack handling and target-protection checks retain their existing paths.

The regression exercises actual `do_kill()` and `multi_hit()` calls for an
unclassed character, invalid/stale styles, equipped weapon combinations,
natural weapons, safe rooms, protected NPCs and null inputs.

## Class menus and help

The default Cyborg display is a compact resource/training overview:

| In-game command | Display |
| --- | --- |
| `cyborg` / `cyborg status` | Resources, training and navigation |
| `cyborg systems` / `cyborg sys_status` | Same overview |
| `cyborg implants` | 24 components, primal/humanity costs and eligibility |
| `cyborg powers` | Component powers, system powers and passive thresholds |
| `cyborg all` | All sections together |

The tables are plain text for Telnet/MUD clients. Rank, resource cost and
mastery/eligibility columns are separate. Displaying a menu does not train,
spend resources or modify class state.

Angel, Demon, Phoenix, Deathknight, Moogle, Gargoyle, Kuei-jin and Wraith
also have aligned kit tables. The ten older classes now have organized
discipline, sphere, gift, skill and power tables too; their progression
commands and balance rules remain intact. All 19 classes are organized
in `help classes`. Werewolf `powers` no longer appends the fallback `Huh?`
after successfully displaying its gifts.

## Exact next-upgrade prices

Training menus quote one next upgrade in its actual currency: XP, primal,
bionics, Drow favor/class points, or Dragon DP plus QP. Quotes preserve
Vampire clan/Caitiff/Panders discounts, selected Werewolf gift discounts,
Mage favored-sphere caps, Elemental focus restrictions, Cyborg dependencies
and Kuei-jin Dharma gates. Capped tracks show `MAXED` and `--`, never a
misleading free upgrade. Already-learned Dragon powers are labeled.

`Affordable` means the currency balance passes, not that combat, body,
target, cooldown or other action rules are bypassed. The update does not
change training prices, allow staff-only bypasses or enable hybrids.
Drow prayer quotes grant fees; the intended target's eligibility is still
checked by the grant command. Free initial Werewolf gift selection is
separate from paid gift training.
Demon gift purchases quote primal, while the warp table quotes soul power
for acquisition, negative-warp redemption and the total destructive reset
fee. Soul power is converted from XP at one to one.

Mage training syntax is `sphere <name> learn`. The read-only sphere menu
also quotes Arete: `arete` itself buys the next rank, even without arguments.

`help classmenus` defines the columns. `help cyborg` groups installation,
training, weapons, explosives, forms, senses, utility and equipment commands.
Existing aliases and exact help topics are retained.

Known means the rank/track threshold is met, not that every casting requirement
is satisfied. Eligible means implant permissions, parts and resources pass;
standing, peace, body state and staff-rank restrictions still apply.
Staff training/installation remains blocked, but now reports an explicit
message instead of silently returning. Use a mortal character for gameplay
testing; this change does not remove staff safeguards.

## Reproduce the checks

From `genesis-maintenance/`:

```sh
sh tests/test-combat-system.sh styles
sh tests/test-combat-system.sh menus
sh tests/test-combat-system.sh training
sh tests/test-combat-system.sh oldmenus
sh tests/test-combat-system.sh all
python3 tests/audit_ability_help.py --strict
sha256sum --check MANIFEST.sha256
```

If the validation environment lacks the `-lcrypt` development link, set
`CRYPT_LIBRARY` to its installed compatible crypt shared library. Debian and
Ubuntu operators should install `libcrypt-dev` for the normal build.

`styles` failed on the unfixed code at the selected-style assertion and passed
after the synchronization fix. Seventeen linked suites run under ASan/UBSan
with strict GNU89 and `-Werror`. The newer-menu suite covers nine class
displays, visible widths, read-only behavior, Cyborg navigation, component
states, actual installation and staff rejection feedback. The older-menu
suite checks 116 read-only views across ten classes at <=79 visible columns.
The training suite compares displayed prices with real purchases at each
tested rank, including clan/gift discounts, caps, exact balances,
one-short rejection, dual-currency Dragon ages, and Cyborg/Dharma gates.

These tests do not certify every legacy gameplay variant or production
balance/security. Hybrids remain read-only previews.

## Hosting and archives

These source changes do not update or restart a running server. Back up live
player and mutable area data before applying an update, stop the game cleanly,
rebuild as an unprivileged user and retain your secured Admin account.

The existing ZIP packages are immutable earlier snapshots and are not replaced
by this change. They do not contain this combat/menu patch. Use the updated
Git source once the pull request is merged, not an older source ZIP.
