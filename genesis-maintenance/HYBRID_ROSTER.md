# Genesis Reforged — named hybrid registry

## Implemented in this stage

The supplied roster is now a canonical compiled registry of all **171**
unordered combinations of the existing **19** standalone parents.
Every display name matches the supplied list. Each entry has an explicit,
stable internal ID and both stable parent IDs.

Examples:

| Parents | Name | Registry ID |
|---|---|---|
| Vampire + Werewolf | Bloodfang | 200001 |
| Sorcerer + Cyborg | Technomancer | 200044 |
| Angel + Demon | Nephilim | 200137 |
| Moogle + Wraith | Phantom Pom | 200171 |

Internal IDs occupy the 200001–200171 registry namespace. They are **not**
CLASS_* bits or assignments to a character's class. Parent IDs 1001–1019
also remain distinct from the legacy CLASS_* values. No existing pure-class
IDs, PC/character structs, save fields, progression or combat rules change.
Existing registry IDs must never be renumbered, recycled or derived from
mutable display names. Append explicit new identities for future expansion.

## Player-facing preview

- `hybrid roster` lists all 171 names and their parents.
- `hybrid roster vampire` lists the 18 Vampire combinations.
- `hybrid roster kuei-jin` lists the 18 Kuei-jin combinations.
- `hybrid pair vampire werewolf` previews Bloodfang.
- `hybrid pair werewolf vampire` returns the same ID and name.
- `hybrid pair angel demon` previews Nephilim.
- `help hybridroster` explains these commands.

Parent names are exact and case-insensitive; ambiguous prefixes are rejected.
Self-pairs, unknown parents and extra arguments are rejected without mutation.
The existing `hybrid` and `hybrid <parent>` inventory previews remain available.

## Safety and what is NOT enabled

This is **registry/preview implementation, not playable hybrid conversion**.
The commands do not enroll characters, grant powers, spend resources or write
hybrid state to saves. All existing readiness gates stay blocked.

There is no new hybrid player save schema yet. Full inheritance, eligibility,
conversion, respec and pure-class ascension still require implementation and
verification. The roster does not claim that every older legacy ability is
hybrid-aware simply because it has an implementation for its own pure class.

The required Class Leader policy is: hybrids cannot challenge for, hold,
inherit or retain those positions. This rule is recorded in the preview and
design, **not claimed as live enforcement on hybrid characters**. Character
hybrid state and all relevant authority/position paths must be implemented
and tested together before allowing conversion.

## Source and verification

`src/hybrid_roster.def` is the single table of explicit IDs, names and pairs.
The registry and exact lookup/validation APIs live in `parents.c/parents.h`.
`hybrid.c` renders the read-only roster. No parent ability bodies are copied.

The runtime registry validator rejects missing coverage, invalid/self/
noncanonical parent pairs, duplicate IDs and case-insensitive duplicate names.
Unique valid unordered pairs plus the complete combinatorial count establish
coverage of every pair, not just the first and last entries.

`tests/test_hybrid_roster.py` compares every ID/parent/name mapping with an
independent checksum computed from the supplied roster. It also locks the
19 existing class bit values and stable parent IDs. `test_parents.c` checks
every pair in both orders, every ID/name lookup, 18 appearances per parent,
invalid inputs and character/PC-data nonmutation across preview commands.

Focused native regression (build current src/merc first):

    python3 tests/test_hybrid_roster.py
    sh tests/test-combat-system.sh parents
    GENESIS_ROSTER_ONLY=1 python3 tests/class_abilities.py

The native probe uses disposable accounts/world data. It checks all 171
actual output rows, filtering, reverse lookups, invalid input, help and
unchanged saved class, progression and resources. It never enrolls a hybrid.

This stage passed the independent supplied-roster oracle, a clean strict
GNU89 Wall/Werror build, all twelve linked ASan/UBSan suites, the focused
native roster probe and the focused native normal/Chaos book-mage probe.
Source comparison with the preceding verified kit ZIP confirms that only
Makefile, hybrid.c, parents.c and parents.h changed among existing game
source files; hybrid_roster.def is new. Save/load, class-ID definitions,
character/PC schemas, ability bodies and combat/progression engines were not
changed. The earlier six-group live kit run is not claimed as a fresh full
six-group rerun for this metadata-only stage.

## Next runtime stage

Add a bounded/versioned two-parent player-state codec, then wire source-
verified authority and full parent ability access through centralized checks.
Test existing-save compatibility, class switching, derived effects and
cleanup. Identify and guard all Class Leader acquisition/retention paths.
Pure ascension remains independent of hybrid inheritance.
Only then may tested parent pairs become selectable.
