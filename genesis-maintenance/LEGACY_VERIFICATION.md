# Legacy verification status — development snapshot

This snapshot is **not a certification that every legacy skill and every
argument variant is functional**. Compiler-active inventory and a reference
to a handler in a test are not effect verification.
See CLASS_KIT_STATUS.md for the current 19-kit audit and remaining work.
The defined Angel (33), Demon (71) and Cyborg (60) Genesis catalogues now
have implementations and targeted effect coverage. This does not certify
every variant in the older ten-class legacy matrix or literal upstream parity.

The reproducible inventory covers 385 compiler-active `do_*`/`spell_*` handlers:
the original 351 in 16 class/spell files plus 34 selected class helpers in
the mixed engine/class file `more.c`. `LEGACY_COVERAGE.csv` lists their
direct command/spell registrations and test mentions. Wrapper-only and
internally called handlers may legitimately lack a direct registration.
Other engine command files are outside this inventory.
Regenerate the inventory with `python3 tests/audit_legacy_coverage.py`.

## Repairs made during this pass

- Friction Curse: remove uninitialized power use, parse the second argument
  correctly, validate quintessence, check exact resource costs atomically,
  preserve the caster's original room after remote safety checks, and reject
  protected, non-PK and duplicate targets without spending resources.
- Ninja sleep bomb: enforce combat safety instead of discarding `is_safe`'s
  result; reject protected/group/self/already-sleeping targets; stop the native
  combat relationship before putting a valid target to sleep.
- Ventriloquate: replace an empty handler with bounded, clearly attributed
  object-based illusory speech. User text remains literal and control
  characters are rejected; it cannot forge a player/staff speaker.
  The existing spell slot remains unchanged; its casting level is now
  `LEV_SOR`, and native book-mage tests exercise the actual `cast` command.
- Clan-name warning: remove an impossible unsigned string-length comparison;
  blank names still remove a clan, and the 25-character maximum remains.
- Granted Angel/Demon perception/body powers: low-mastery recipients now gain
  their actual passive perception/flight/body benefits, including on reload.
- Reference state/management: validate consenting pact recipients and power
  recipients; clear foreign class state; reject rooted/protected travel and
  invalid courier weights; show bounded visible Demon/Cyborg aspects.

`tests/test_legacy_variants.c` asserts actual Friction Curse speed and recoil,
Ninja sleep and combat cleanup, and actual Ventriloquate output. It includes
positive, cost, prerequisite and protection cases, but is not exhaustive
coverage of the complete legacy source tree.

## Current help/variant follow-up

The exact-help audit now covers all **398 registered legacy topics**, with
zero missing public exact entries. Aliases are included; this is not a count
of distinct powers. Classic spells use the `SPELL <name>` namespace so they
do not replace unrelated HEAL, GATE, QUEST, WEB or other command interfaces.
The Elemental class page explicitly directs Werewolves to their distinct
Lupus ELEMENTAL command. Staff-only helpers remain permission restricted.

Twelve new area-list-loaded files document Mage spheres, classic spells,
book scripts, Dragon/Highlander, Drow, Ninja, Werewolf, Monk, Elemental,
advanced Vampire disciplines, shared commands and `more.c` class helpers.
The latter add 39 registered command/alias topics that were outside the
original 16-file inventory. Ordinary `more.c` engine commands remain excluded.
The ABILITYSTATUS restored-command page is not replaced by an upkeep alias.
Historical lore is
corrected where actual gates differ: preferred/nonpreferred gift caps 5/4,
ADVANCE quest/kill counters, Body Wrack/Balor/Mastery, Spirit Guardian,
Elemental Sun Bath/Mirage Slice/Far Sight, and Quietus Leech.

Implemented and asserted repairs in this follow-up:

- `tests/test_mage_variants.c`: usable-actor gates, Heart's Blood atomicity
  and nonlethal limits, native sphere effects and costs, Prime payment,
  native combat-start Contingency dispatch, all five Doublecast variants,
  Rift direct movement versus linked portals, and protected/invalid targets.
  Alter Weight's existing `get_speed` hook really adds three delay points;
  the test checks application and removal rather than just an affect label.
- `tests/test_legacy_spells.c`: positive Armor/Shield/Stone Skin and actual
  player avoidance across the armor boundary; Earthquake considers the
  target's ordinary/vampiric/Dragon flight, not the caster's flight.
- `tests/test_legacy_variants.c`: Drow real Skip Disc toggle, missing spider
  prerequisites, guarded Dimensional Door, actual crossbow damage/sleep,
  protected/group/non-PK/NOKILL rejection, room-list restoration and player
  poison immunity; Ninja real cloak/Michi/Orimasu, WEB 8, both-room shadow
  travel and root-blocked/actual Mienaku escape costing exactly 20 Ki.
- The same variant test checks Werewolf Bridge Walk Galliard 5, free protected
  rejection, actual movement and bounded empty-world misfire; Fetish Doll
  treats held-object percent sequences literally and preserves its component
  when the doll prototype is absent.
- Monk Celestial Path now requires its actual 1500 mana before relocation.
  Celestial, Elemental Water Travel and Vampire Earth Shift/Shadow Step assert both-room
  protections, restraints, combat timers, incompatible planes and actual
  allowed movement/payment.
- The previously unreachable staff Familiar binding now accepts only the
  caller's charmed, unclaimed, unoccupied NPC follower. Low-trust callers and
  foreign/unowned/occupied creatures are rejected; clearing releases the
  link, not the follower. `[` controls only an owned, unoccupied NPC and
  preserves an existing commanded flag. It scans live characters before
  post-action cleanup because an NPC action can extract its actor.

Reproduce the documentation check with:

```
python3 tests/audit_ability_help.py --strict
GENESIS_HELP_ONLY=1 python3 tests/class_abilities.py
```

The native helper boots a disposable server and requests each topic/alias
in all twelve new files. It also checks the Elemental/Werewolf namespace
and preservation of ABILITYSTATUS.
It does not grant the caller access to staff-only execution.

## Remaining scope

Many original class argument variants still need explicit positive and
negative effect assertions. This pass expands specific sphere-Mage cost,
effect and remote-room cases; it does not certify every remaining spellpower,
backlash, target/status and save-state permutation.
An inventory entry with test mentions must not be promoted to full functional
verification without those assertions.

Angel/Demon/Cyborg adapter coverage has expanded through
`tests/test_reference_completion.c` and `tests/test_reference_state.c`.
Implemented adapter behavior is a conservative Genesis conversion, not a
claim of exact upstream mechanics or final PvP balance. Existing parent
inventory readiness gates have not been relaxed.

Hybrid conversion, ascension and respec remain disabled.
