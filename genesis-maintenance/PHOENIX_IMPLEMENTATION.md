# Standalone Phoenix — independently designed kit

The user chose to reuse Cyborg source and design Phoenix separately.
This is new Genesis gameplay, not a recovered GodWars implementation.
Angel/Demon effects and the hybrid conversion feature remain unfinished.
The user subsequently supplied The Vortex's root `cyborg.c` as the Cyborg
reference. Its effects have not been ported; Cyborg is planned for standalone
AND hybrid use following the user's latest correction.

## Class and progression

`autoclass phoenix` uses the existing awakened-player requirement and requires
trained STR 2, DEX 2, CON 3, INT 3 and WIS 2. It starts Phoenix mastery at zero.
`phoenix` lists powers. `phoenix train` advances one rank, maximum eight,
at `50000 * next_rank * next_rank` experience. Failed training is free.
Training is unavailable in combat or with a fight timer.

Normal nonstaff stat caps are STR 9, DEX 8, CON 9, INT 10 and WIS 11.
Existing staff allowances, temporary-stat rules and stat-training accounting
are unchanged.

The new standalone `CLASS_PHOENIX=1024` is independent of all ten old class
IDs and of `GEN_PARENT_PHOENIX`. Mastery uses the existing saved Powers[0]
field only when the character is a standalone Phoenix. There is no character
file migration and no new player-data field.

## Complete initial eight-power kit

| Rank | Command after `phoenix` | Mana | Effect | Recovery |
|---|---|---:|---|---:|
| 1 | `ember [enemy]` | 500 | Spell-pipeline attack, base `300 + 100 * mastery`; a save halves damage. | 3 seconds |
| 2 | `ward` | 750 | Protective AC +20 for 3 ticks, matching Genesis's positive-is-protective convention. | 3 seconds |
| 3 | `wings` | 1000 | Flight for 3 ticks; rejects preexisting flight and preserves other affect/equipment flight owners during expiry. | 3 seconds |
| 4 | `ashwalk` | 1500 | Return to the existing saved home through the recall engine. | 9 seconds |
| 5 | `tears [ally]` | 1000 | Heal a living self/group ally by at most 10% maximum health, capped at 3000 and at maximum health. | 6 seconds |
| 6 | `flare [enemy]` | 1500 | Spell-pipeline attack, base `200 + 60 * mastery`; a save halves damage, otherwise apply removable 1-tick blindness if not already blind. | 3 seconds |
| 7 | `renew` | 2000 | Out-of-combat self-healing, at most 20% maximum health, capped at 6000 and at maximum health. | 9 seconds |
| 8 | `ascend` | 3000 | Hitroll +10 and damroll +10 for 3 ticks; no automatic extra attacks or other power grants. | 3 seconds |

Recovery is a shared named pulse-timed affect, retained through reload and
not reset by Dispel Magic. Successful actions also apply the
normal 12-pulse command wait. Buffs cannot stack with another instance.
Existing combat modifiers and protections still apply to spell damage.
Genesis has no separate movement resource: Renewal restores health, not an
invented movement pool. No automatic resurrection or death escape was added.

## Safety and compatibility

- Hostile powers reject safe rooms, self/group/staff targets, unseen/different-
  room targets and protected/non-PK players before costs or combat effects.
- Attacks use `hurt_fighting` and the real five-argument `damage` API; no raw
  hostile hit-point subtraction and no access after possible extraction.
- Healing requires positive health and appropriate group membership; it never
  revives zero/negative-health targets or overfills health.
- Ashwalk rejects invalid home IDs before lookup, missing destinations, combat/
  fight timers, curses, webs, source no-recall/no-teleport flags and private/
  solitary/no-teleport destinations. Failed travel is free.
- New powers reject Lycaon's curse. Existing Elemental `phoenixform` remains
  unchanged. The `phoenix` abbreviation still dispatches Elementals to that
  handler with its original standing and Lycaon requirements.
- Existing skill indices are preserved: six named metadata entries are appended
  to the skill table, with MAX_SKILL increased accordingly.
- Named buffs/recovery use the existing affect serialization and removal paths.
  Clearing Phoenix class strips Phoenix-specific buffs and recovery.
- Hybrid selection/gating is NOT enabled. The Phoenix parent remains unready
  for conversion until the entire hybrid system is implemented and verified.

The strengths and costs above are explicit conservative starting rules, not
claims of proven PvP balance across the eventual 91 hybrid pairings.

## Verification commands

```sh
make -C src CFLAGS='-O2 -std=gnu89 -Wall -Werror -Wno-builtin-declaration-mismatch'
sh tests/test-combat-system.sh phoenix
sh tests/test-combat-system.sh parents
python3 tests/class_abilities.py
```

The linked sanitizer Phoenix probe exercises class choice, rank costs/caps,
insufficient resources, each of the eight effects, failed and successful
restricted travel, target protection, healing bounds, shared flight ownership,
nonstacking/removal, class cleanup and malformed input.

The live class suite uses disposable accounts to test real command dispatch,
autoclass/training, mastery/ward save records and logout/login restoration,
while retaining the existing class regressions. No real player files are used.

## Verified development batch

- Strict native build and all five native live suites passed.
- Full linked ASan/UBSan Phoenix probe passed, including successful/restricted
  Ashwalk and flight-sharing checks; the linked parent probe also passed.
- Resolve/resource/derived-stat and random-range sanitizer probes passed.
- The resolve probe initially failed to link because its lightweight fixture
  did not define the new Phoenix Wings skill global. The fixture now defines
  it and the probe passes; this was not a server runtime failure.
- Unique Phoenix/Phoenixform command registration was checked.

These checks do not establish balanced Phoenix/Cyborg hybrids or working
Angel/Demon effects. Those features remain unfinished and conversion is blocked.
