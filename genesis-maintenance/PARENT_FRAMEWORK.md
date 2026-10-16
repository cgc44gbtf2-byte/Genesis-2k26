# Parent/class expansion — development batch 1

**Current snapshot:** 19 standalone parents and 171 planned two-parent pairs.
The five latest native kits and their exact ability lists are documented in
`CLASS_GUIDE.md`. Historical 75%-selection notes below are superseded by full
inheritance from both parents, with a comparable ascended-pure alternative.
Hybrid conversion, pure-class ascension and path respecs remain disabled.

Named roster follow-up: all 171 pairings now have stable explicit registry
IDs and exact display names from the supplied Genesis Reforged list.
Use `hybrid roster [parent]` or `hybrid pair <parent> <parent>`.
See HYBRID_ROSTER.md for the current read-only scope and runtime work still
required; registration alone does not make a hybrid playable.

Current defined catalogues: Angel 33/33, Demon 71/71 and Cyborg 60/60
implemented with targeted effect tests. Every non-system Angel/Demon grant
is tested for payment and player-parser persistence. See CLASS_KIT_STATUS.md
for the remaining legacy gates and CYBORG_KIT.md for the full Cyborg kit.
Hybrid conversion is still disabled; registry readiness flags remain blocked.

Historical batch: playable Angel, Cyborg and Demon kits are active, with 22 Angel,
27 Demon and 32 Cyborg inventory entries implemented. The latest expansion
adds 11 Angel and 11 Demon entries; see NEW_CLASSES_IMPLEMENTATION.md.
All 14 parents now map their own
standalone class flags. Counts remain 14 parents and 91 planned pairs.
The historical backend-only notes below do not override the latest status.
Full reference kits, selected-power inheritance and hybrid conversion remain
unfinished; all hybrid-ready flags stay false.

This is an unfinished development build, NOT the completed hybrid expansion.
Do not replace a production server with it on the assumption that the full
hybrid expansion is complete. A standalone Phoenix kit has now been added;
Angel, Cyborg, Demon and hybrid conversion remain unfinished.

## Implemented

- Fourteen explicit parent identities, independent of legacy `CLASS_*` values.
- One Demon parent, with reference-derived powers consolidated by stable,
  parent-scoped keys; no separate Demon variants.
- Exact, case-insensitive parent lookup, with no prefix-based authorization.
- Validation and canonical ordering of two distinct, known parents.
- Ninety-one unordered planned pairings. This is a scope count, not a count
  of implemented or playable hybrids.
- A read-only `hybrid` command and `hybrid <parent>` inventory preview.
- Readiness checks require complete catalogues, gated effects, verified
  persistence, real implemented powers and a nonempty gameplay kit.
- Native/live and linked sanitizer regression coverage for the foundation.

## Inventory is not implementation

`src/parent_powers.def` inventories 33 Angel and 71 Demon entries identified
from the inspected reference sources. This includes support/management
systems; it is NOT an ability quota denominator or an exhaustive final list.
Unported Angel/Demon entries remain explicitly unimplemented. Eight independently designed
standalone Phoenix entries have been added; hybrid gates are still blocked.
Sources are identified by D (Deluxe),
E (Ethereal Mists), L (GWLow5), U (UMGW), V (Vortex), M (Mindcloud)
and G (new Genesis design).

Alternative implementations of Leech, Graft, Blink and similar powers share
one Demon entry. Angel and Demon powers remain separate parent-scoped entries
even when they use the same name. Mechanically different effects are kept
separate pending review; e.g. Infernal Portal, Chaos Portal and Demon Gate.
Emergency Inferno does not replace Genesis Mage's existing Inferno.

The ten old classes are mapped to their unchanged existing class IDs.
Their individual active/passive catalogues and hybrid gates are not finished.
Cyborg is planned as a standalone class AND a hybrid parent, without a
playable empty kit. This replaces the earlier hybrid-only requirement.
Its 24-component install/dependency backend is implemented in isolation, and
60 pending inventory entries cover Vortex and Mindcloud. Combat effects,
selection integration and full gameplay-state persistence remain unfinished.
Independent component state now has an optional player-file record and
class-clear cleanup; linked parser and native logout/login checks passed.
See `CYBORG_IMPLEMENTATION.md`. Phoenix now maps
to its implemented standalone class; see `PHOENIX_IMPLEMENTATION.md`.

## Deliberately unavailable

No hybrid conversion, selectable 75% budgets, hybrid power grants or hybrid
save/load has been implemented. Standalone Phoenix has its own training and
eight powers and is offered through `autoclass`. Every parent remains unready for
hybrid conversion, including existing standalone classes.

The preview reads metadata only. It does not change `ch->class`, class tracks,
stats, stances, inventory, resources or player files. There is no migration and
no new character-save record. Existing standalone behavior remains unchanged;
Phoenix uses existing class/power/affect save fields.

## Required next implementation work

1. Complete the distinct active/passive catalogue for every parent, including
   the existing ten, and separate administrative/cosmetic entries from powers.
2. Implement Angel and Demon effects, costs, prerequisites, cooldowns, combat/
   target safety, object templates, ownership, expiry and form cleanup.
3. Adapt the supplied Cyborg references as a standalone class AND hybrid parent;
   preserve attribution, replace incompatible state/flags and repair effects.
   The standalone Phoenix eight-power kit is now implemented separately.
4. Gate every selected power's active and passive effects, including alternate
   dispatch routes, transformations, extra attacks and training paths.
5. Implement mastery checks using actual existing class caps; preview, select
   and confirm; separate 75% budgets and explicit integer rounding.
6. Implement compatible, validated and failure-safe persistence; test reload,
   interrupted saves, unknown powers, and old single-class characters.
7. Run class-specific effect regressions and all existing native/sanitizer
   suites before labeling any expanded kit complete or playable.

## Testing

Build first:

```sh
make -C src CFLAGS='-O2 -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch'
python3 tests/class_abilities.py
sh tests/test-combat-system.sh parents
```

The linked parent probe uses real server modules, the real argument parser
and output buffering. It checks every planned pairing, exact lookups,
old-class mappings, deduplicated inventory, blocked readiness, invalid inputs,
oversized arguments, NPC/missing-player handling, and byte-for-byte unchanged
character/player state after each preview.

The live class suite invokes the registered command on disposable characters,
saves between actions and checks class/experience preservation. All tests use
temporary worlds/probes, never production player data.

Verified for this batch:

- Strict native build: `-O2 -std=gnu89 -Wall -Werror`.
- Full linked ASan/UBSan build and the new parent/preview probe.
- All five native live suites: runtime smoke, system fixes, legacy spells,
  maintenance fixes and class abilities, including the new preview checks.
- Existing sanitizer resolve/resource/derived-stat and random-range probes.
- Unique command registration with `hybrid` mapped to `do_hybrid`.

These results verify the foundation and existing behavior covered by the
suites. They do not verify new class effects, which are not implemented.

On Debian/Ubuntu install `build-essential libcrypt-dev python3`. This workspace
required an explicit path to its installed crypt runtime library:
`LDLIBS=/lib/x86_64-linux-gnu/libcrypt.so.1` for Make and
`CRYPT_LIBRARY=/lib/x86_64-linux-gnu/libcrypt.so.1` for probe/live tests.
This is a workspace linking workaround, not a gameplay source change.

## Release separation

The earlier finalized `Genesis-MUD-maintenance-updated.zip` is unchanged.
This batch belongs in a separately labeled development archive. It is not
published to GitHub, does not enable Actions, and does not deploy a server.
No upstream implementation bodies were copied into this batch.
