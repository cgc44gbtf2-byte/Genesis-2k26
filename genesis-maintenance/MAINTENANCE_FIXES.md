# Nine-finding class maintenance batch

This batch repairs the nine findings in the class-ability audit. It is source
maintenance only: nothing was deployed, and no live player files were edited.

## Repairs

| Finding | Change | Regression coverage |
| --- | --- | --- |
| Mage resource inputs | Bounded integer parsing and shared cost validation for Rift, Beginner's Luck, Mage Invisibility and Paradox Ward | Invalid live commands; deterministic exact-cost, overflow and Heart's Blood tests |
| Obedience command bypass | Resolve commands to handlers and allow only movement, speech, posture, look and flee; validate named and all-target requests | Full/abbreviated ownership and quit commands, recursion, eligible speech, PK/staff/safe-room guards |
| Jam Technology protection | Check staff/trust, PK eligibility, room safety and normal combat protections before touching equipment | Claimed weapons remain equipped on protected targets; eligible target can still be disarmed |
| Elemental saved slows | Persist all three remaining timers; repair orphaned flags on load | Actual casts followed by save, process restart, reload and expiry; old/malformed save fixtures |
| Ignored attribute affects | Derive all five effective attributes from live character and worn template/instance affects | Penalties, buffs, minimum, removal, unchanged trained stats, equipment and a loaded Weakness affect |
| Ignored saving modifiers | Use effective WIS and subtract SavingThrow before the existing clamp | Both modifier signs, both threshold limits, WIS penalties and Philodox independence |
| Healing Touch overspend | Calculate the actual rounded cost and check it before healing or charging | Low-mana and affordable live casts; deterministic rounding across the heal range |
| Almighty Favor group result | Initialize the result flag to FALSE | First eligible group blessing succeeds; repeating on an all-blessed group does not charge again |
| Totemgift armor direction | Change the protective modifier to +20; update help and mechanics notes | Actual cast and dismissal; real char_ac/250 threshold increases with a positive modifier |

## Explicit mechanics and compatibility decisions

### Mage costs and input rules

Mage Invisibility still costs `5000 + 300 * quintessence`; Paradox Ward still
costs the same formula and requires at least one point. Beginner's Luck retains
its flat 3500 mana cost. Rift now spends its previously declared but unused
500 mana cost and the requested quintessence; no additional per-point mana
charge was invented for Rift. A failed Rift roll no longer creates a portal
or teleports the caster. As with the other spells, an eligible attempted cast
can consume its cost even if the spell roll fails.

Negative, malformed, out-of-range and unaffordable requests are rejected before
changing mana, quintessence or lifeforce. An already active Mage Invisibility
also refuses a second charge.

For these four paths, Heart's Blood can supply at most five missing points
after the complete mana cost is checked. It does not make BODY quintessence
negative, and a rolled lifeforce cost that would kill the caster is refused
without spending resources. Previously corrupted or exploited player resource
balances are not globally rewritten by this batch.

Audit correction: Paradox Ward already had a `quintessence < 1` guard.
The original audit incorrectly described its signed-input resource pattern
as equivalent to Mage Invisibility's negative-credit exploit. Its new work is
overflow-safe parsing and shared, atomic cost handling, not removal of an
observed negative-credit path.

### Control and equipment safety

Obedience and Mastery now share the same resolved-handler allowlist and target
protection policy. Inventory, account, administrative and recursive-control
commands cannot be compelled through full names or abbreviations. Obedience's
existing rank requirements, NPC resistance and single/all-target willpower
costs remain unchanged.

Jam Technology retains its original chance, equipment behavior and wait state
for eligible targets. It is not changed into a timed weapon curse or a new
damage ability.

### Saved statuses

The new player fields are `Slow_timer`, `Liquify_timer` and
`Atoningwinds_timer`. Valid remaining values are retained up to the normal
20-count duration. A flag without a positive timer is cleared; a timer without
its flag is zeroed. This repairs old saves rather than preserving an indefinite
combat-delay penalty.

These remain online countdowns, consistent with the legacy update loop:
offline time does not itself advance a saved counter. No skill-table indices,
class IDs or existing player keys were reordered.

### Shared stats, saves and Monk costs

Trained base attributes are not mutated by temporary effects. Existing base
training caps and class bonuses remain; live modifiers are then applied with
an effective minimum of one and integer-overflow protection. Worn template
and instance modifiers are both included; carried but unworn objects are not.
Removing an affect or unequipping an item removes its derived contribution.

SavingThrow uses the existing negative-is-better convention. The saving
threshold remains clamped from 15 to 85, with the original roll comparison.
Strength of Purpose remains a separate capped bonus. These shared repairs
affect other abilities and equipment using the same modifier locations, not
only the two Vampire powers; review combat balance before a public launch.

Healing Touch retains its 5000 mana entry requirement and `1.5 * heal` charge.
Integer rounding preserves the old affordable-cast remaining balance, including
odd heal amounts. It now refuses a rolled cost the caster cannot afford before
healing the target. Almighty Favor's costs and prerequisites are unchanged.

### Totemgift

The aura remains a noncombat apparition with +15 hitroll and duration 2. Its
armor modifier is now +20 because higher armor improves this engine's avoidance
term. There are still no pet inventories, combat mobiles or stacking auras.
An already saved old -20 aura is not rewritten during upgrade: dismiss and
resummon it, or let its short normal duration expire.

## Additional bounded-output maintenance

The strict sanitizer build exposed legacy `sprintf` overflow warnings in the
Who list, its XP summary, a telepathy log message, staff flag confirmation,
wall descriptions and OLC command construction. Those sites now use the
existing capacity-aware `format_buffer` helper. All Who-list appends are bounded,
not just the first warning sites. Oversized group output may truncate instead
of writing beyond its buffer; ordinary output is unchanged.

The class-test fixtures also receive infrared visibility so outdoor nighttime
does not randomly invalidate unrelated target-selection tests. Test equipment
is claimed so the normal save policy includes it when checking equipped state.
Neither fixture adjustment grants abilities or bypasses safety in gameplay code.

Wrath/Fear assertions run after the other combat scenarios. Leaving Fear active
on their shared Vampire fixture allowed normal combat pulses to make that caster
flee, intermittently invalidating later same-room target selection. Gameplay
Fear behavior was not weakened to make the test pass.

## Re-run verification

With `src/merc` built:

```sh
python3 tests/class_abilities.py
python3 tests/maintenance_fixes.py
python3 tests/smoke_runtime.py
sh tests/test-resolve.sh
sh tests/test-random.sh
```

Python runtime tests require the system crypt library. If it is not discoverable,
set `CRYPT_LIBRARY` to the installed library's absolute path. Debian/Ubuntu
normally resolve it without this workspace's special path override.

The state, Mage and Monk C tests use AddressSanitizer/UndefinedBehaviorSanitizer.
The runtime suites create disposable accounts and temporary area copies and
never use the package's player records. Real ticks and expiry make the class
suite take several minutes.

## Upgrade precautions

Back up source, player files and mutable area data before an external upgrade.
New timer fields are ignored by older builds, so do not rely on an older server
to retain these repaired expiry semantics. Preserve the matching backup if
you need to roll back. No production-host compatibility, load capacity,
complete balance or full security audit is claimed.

## Completed verification

The final source snapshots matched between the normal and sanitizer builds.
Both builds completed with `-std=gnu89 -Wall -Werror` (and the existing
`-Wno-builtin-declaration-mismatch` compatibility flag).

The normal build used `-O2`. The sanitizer build used `-O1 -g`,
AddressSanitizer, UndefinedBehaviorSanitizer, frame pointers and non-PIE
instrumentation. Compiler overflow warnings found during that stricter build
were repaired with bounded output rather than suppressed.

Both binaries passed the complete class suite, the focused maintenance suite
and runtime smoke checks. The focused suite included actual Elemental casts
followed by a process restart and successful expiry of the reloaded statuses.
No ASan or UBSan findings were reported by the final runtime suites.

The deterministic C regressions passed under ASan/UBSan for Strength of Purpose,
Lycaon form ownership, all five derived attributes, saving modifiers and limits,
the actual protective armor term, Mage parser/cost handling and Healing Touch's
exact rounded cost. The number-range regression also passed.

The delivery ZIP is a source/data/test package, not a prebuilt executable.
`MANIFEST.sha256` lists hashes of its payload files except the manifest itself.
After extracting, run `sha256sum -c MANIFEST.sha256` from the package directory
to check the extracted files.
