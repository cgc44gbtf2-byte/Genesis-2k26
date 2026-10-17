# Class-kit completion and verification status

## Current scope

The current legacy-help/variant pass adds twelve loaded help files and an
exact-topic audit: **398/398 registered legacy topics have public exact
entries**, with classic spells documented under `HELP SPELL <name>`.
Documentation does not grant a level-99 spell or bypass command permissions.
Elemental's class page also explains the different Werewolf ELEMENTAL command.

Selected Mage costs/effects, shared positive-armor spells, Earthquake flight,
Drow/Ninja movement/combat, Werewolf Bridge Walk/Fetish Doll, and
Monk/Elemental/Vampire travel (including Shadow Step) have new
effect/protection assertions. The existing ABILITYSTATUS restored-command
page is preserved rather than shadowed by an upkeep alias.
The staff Familiar helper is restored with its original permission level,
owned-charmed-NPC binding and occupied-body protection; it is not a new
ordinary-player class power. Familiar control also checks ownership, refuses
occupied bodies and avoids dereferencing a character removed by its action.
See LEGACY_VERIFICATION.md for exact scope and reproducible checks.

### Current class/staff maintenance follow-up

The current source passes 21 linked sanitizer suites, expanded training feedback
and exact-payment checks, derived-benefit checks, and role-filtered staff-menu
checks. Native class gameplay and 295 public help entries / 662 exact topics
and aliases also passed. See CLASS_AND_STAFF_MAINTENANCE.md and CLASS_BENEFITS.md.
All 16 implemented Vampire discipline training tracks are exercised; the old
help-only Daimoinon/Mortis/Melpominee names are not implemented or advertised.
This does not close the full individual-ability/variant certification gap below.

### Previous named-roster follow-up

Named hybrid follow-up: all 171 combinations now have stable registry
identities and read-only roster/pair previews; see HYBRID_ROSTER.md.
This does not enable hybrid characters, inheritance or ascension.
The follow-up passed its independent 171-entry oracle, strict build,
all twelve sanitizer suites, focused live roster/nonmutation test and focused
normal/Chaos book-mage test. Save/combat/progression engine source is unchanged
from the preceding kit/help batch; see HYBRID_ROSTER.md for verification scope.

All 19 standalone class identities have gameplay code; there are no empty
function bodies among the 385 compiler-active legacy class/spell handlers
covered by `tests/audit_legacy_coverage.py`. Neither fact is a certification
that every ability, argument variant, balance rule or save-state combination
works. The request to finish and verify every kit remains **open**.

| Kit | Current evidence / remaining gate |
|---|---|
| Vampire | Original disciplines/shared code; Earth Shift actual travel, both-room/timer/restraint/plane checks tested; full discipline/variant matrix still open |
| Werewolf | Restored gifts plus bounded Bridge Walk, exact-track/protected travel and Fetish Doll format/template safety tested; full gift matrix still open |
| Sorcerer | Native writing/binding/chant system; normal/Chaos starter spells, all color roles and real save/relogin tested; arbitrary user-written scripts not exhaustively certified |
| Drow | 14 scoped handlers; real Skip Disc, guarded spider setup, actual dimensional travel and crossbow damage/sleep/poison/protection cases; full matrix still open |
| Highlander | Original seven scoped handlers present; broader individual variant verification still open |
| Mage | 31 scoped handlers; ten numeric parsers guarded, nine effect repairs, Prime costs, Heart's Blood, actual combat Contingency, all five Doublecast choices and Rift variants tested; full matrix still open |
| Dragon | Original seven scoped handlers present; broader individual variant verification still open |
| Ninja | 16 scoped handlers; real sleep, cloak/Michi/Orimasu, WEB rank, shadow travel and exact-20-Ki escape cases tested; full principles/ability matrix still open |
| Monk | 17 scoped handlers; support/cost/group tests plus Celestial affordability, actual travel and both-room/restraint/plane rejection; full matrix still open |
| Elemental | 27 scoped handlers; casting/persistence/expiry/attributes plus Water Travel protections and actual cost/movement tested; every variant not certified |
| Angel | All 33 catalogued Genesis entries implemented; core/shared-reference effects plus every non-system grant, exact costs and grant save/reload covered |
| Cyborg | All 60 catalogued Genesis entries implemented; component, reference/effect, codec and dedicated completion suites |
| Demon | All 71 unified Genesis entries implemented; core, shared-reference, shape, soul, warp, enchantment, attack and every non-system grant covered |
| Phoenix | Eight implemented native abilities with linked and live progression/effect/persistence tests |
| Gargoyle | Implemented native kit and independent state; linked/native enrollment, progression and effect tests |
| Kuei-jin | Implemented independent tracks/Dharma/Chi/Akuma; linked/native effect/progression/state tests |
| Deathknight | Implemented original Genesis-native kit; linked/native enrollment/training/focus/effect/state tests |
| Moogle | Implemented original Genesis-native kit; linked/native enrollment/training/focus/effect/state tests |
| Wraith | Implemented independent tracks and reference adaptations; linked/native effect/progression/state tests |

Handler counts are scoped compiler inventory counts, not distinct powers.
Shared clan, discipline, spell and stance files serve multiple kits.
Aliases and dynamic book scripts make raw function counts unsuitable as a
claim about the total number of abilities. Test mentions are not effect proof.

## This update

- Finish Cyborg catalogue promotion only after real adapter/effect tests;
  keep hybrid conversion blocked.
- Reconcile Angel/Demon's remaining 55 catalogue flags with their real shared
  and legacy-reference adapters. Exercise every non-system delegation,
  reject system delegation, and round-trip every grant through the player
  parser. Verify low-rank granted scry and weapon/human form effects.
- Validate ten legacy sphere-Mage numeric inputs before paradox or costs;
  replace placeholder sphere pages with learned abilities and correct seven
  stale displayed mastery thresholds. Add the MAGEPOWERS command.
- Repair Brittle Bones' remote safety/restoration, atomic payment and free
  duplicate rejection. Verify actual soak reduction and finite charges;
  stop traversing freed affect nodes when the last charge expires.
- Repair full-message Cyborg radio dispatch, incapacitated-body gating and
  mine cancellation when moving into a safe room.
- Correct starter-book Ward to raise armor according to Genesis's actual
  positive-armor avoidance calculation. Existing previously written page
  scripts retain their stored content; this is not a migration of arbitrary
  player-authored spell pages.
- Correct Darkblessing's armor-stat sign to match Genesis positive armor;
  verify the native wand/NPC stat result. Its +1 hitroll/+1 damroll are real
  offensive buffs; NPC avoidance is currently level-based, not armor-based.
  Add in-game Know Alignment, Frenzy and
  Darkblessing help matching their actual effects and eligibility.
- Add `area/kits.hlp`, loaded before historical help so current class pages
  are actually returned. Cover all 19 kits, normal/Chaos books, current Cyborg
  commands, kit status and disabled hybrid/ascension/respec.
- Replace the legacy five-name `relevel` lists with exact case-insensitive
  Admin-only highest-rank bootstrap. See ADMIN_BOOTSTRAP.md for the required
  account-security step before public registration.

## Remaining work before declaring every kit finished

1. Expand the older ten-class legacy effect/argument matrix. There are still
   225 scoped handlers without test mentions; that is a discovery list,
   not proof that those handlers are broken.
2. Broaden remaining sphere-Mage argument/target/status combinations beyond
   the repaired and asserted casts. Heart's Blood, Prime, combat Contingency,
   Doublecast and Rift now have dedicated cost/target/transition assertions;
   this does not certify every possible spellpower/backlash combination.
3. Verify each repaired path through native routing and save/relogin where
   relevant, keeping this document and player-facing help consistent.
4. Continue auditing other armor-related paths against Genesis avoidance.
   The common Armor/Shield/Stone Skin and starter-book Ward now use positive
   armor, with an actual player-avoidance boundary assertion.
   NPC avoidblow uses level, ignoring armor; a displayed NPC armor change is
   not proof of a defensive combat benefit. No NPC avoidance rebalance is
   included in this update.

## Verification of this legacy-help/variant pass

- Current-source GNU89 build with `-Wall -Werror` passed, retaining the
  documented `-Wno-builtin-declaration-mismatch` exception for legacy names.
- All thirteen linked ASan/UBSan suites passed after the Shadow Step change,
  including the four travel variants and Familiar ownership/body checks.
- Native help lookup passed for 266 entries / 583 exact topics and aliases
  across the twelve new files. Additional checks preserve ABILITYSTATUS and
  distinguish the Elemental parent from the Werewolf ELEMENTAL command.
- The independent 171-entry hybrid roster oracle passed; this verifies
  read-only registry identities, not hybrid conversion or inheritance.
- All seven native gameplay groups passed: smoke_runtime, system_fixes,
  legacy_spells, reconstructed_abilities, maintenance_fixes, crafted_classes
  and class_abilities. These include the restored ABILITYSTATUS page and
  actual reconstructed ability effects. This is not all-variants certification.

The compiler inventory now includes the 34 selected `more.c` class helpers;
the exact help audit covers their 39 registrations as well as the original
359 topics. Ordinary engine commands and internal `spell_null` identities
remain outside the help inventory.

## Verification of the preceding kit/help batch

- Clean strict GNU89 build with Wall/Werror passed.
- All twelve linked ASan/UBSan suites passed, including complete Cyborg,
  Angel/Demon grants, Mage numeric/soak regressions and Admin permissions.
- All six native groups passed against the rebuilt executable: smoke_runtime,
  system_fixes, legacy_spells, maintenance_fixes, crafted_classes and
  class_abilities. These include actual help lookup, spell/item effects,
  enrollment, training and relevant save/relogin/expiry checks.
- All four standalone state-test script groups passed: Cyborg core/runtime/
  systems, shared reference state, crafted state and lineage state.

Native Python probes execute the existing src/merc; they do not rebuild it.
Always build current source before running them. An earlier stale-executable
run was discarded and all six groups rerun after a clean strict build.
Sanitizer success covers these probes, not every possible execution path;
the suites disable leak detection for the legacy pool allocator.

The development ZIP is not a promise of universal upstream parity or
production multiplayer balance. Hybrid conversion, ascension and respec
remain deliberately disabled and are not enabled by catalogue completion.
