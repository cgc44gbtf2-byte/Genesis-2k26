# Shared combat defenses and short recovery clocks

## Scope

This change implements the shared-rule repair approved after the all-class
balance audit. It does **not** tune individual stat caps, damage caps, class
bonuses, implant budgets, weapon coefficients, equipment or resource prices.
It changes gameplay: hostile legacy attacks now respect shared class
absorption, and five recovery families use short pulse timers.

Hybrid conversion, ascension and respec remain disabled. No live host/player
data or main branch was changed.

## Damage handling

`combat_damage_amount()` owns the existing Cyborg and lineage absorption
calculations. The normal `damage()` path and new `legacy_damage()` adapter
both use it once. The adapter also applies the existing NPC-evil protection
rule before committing HP loss.

Converted legacy hostile paths include:

- Book Mage single-target and area page damage, including Chaos books.
- Sphere Mage Alter Weight, electrical and inferno damage.
- Elemental legacy attack/area/Night Blade damage.
- Monk Aura Shot, Wrack and kick damage.
- Drow crossbow, Highlander bash and the legacy Stargazer attack.
- Feeding-related legacy damage and Elemental burning-form retaliation.

Physical crossbow/bash/kick attacks retain physical types; spell attacks
retain spell types. Gargoyle physical-only resilience is not turned into
spell immunity. Cyborg protection applies to both through the shared helper.

`hurt_person()` remains the explicit raw HP commit for self-harm, backlash,
percentage-health costs and the two shared damage implementations. A source
scan found no other remaining direct hostile commits outside those shared
implementations. Raw self-damage is not accidentally reduced by the user's
own class protection.

This deliberately preserves legacy attack narration, control effects,
casting checks, antimagic, combat initiation and retaliation/reflection
semantics. It does not force every legacy power through all unrelated
`damage()` side effects. Some legacy numeric narration reports the original
pre-defense attack amount; HP deltas are authoritative for actual loss.
Spell damage is not automatically clamped by the melee damage cap.

## Cooldown rules

One combat recovery unit now means **three seconds**, or 12 pulses at four
pulses per second. Existing recovery multipliers remain:

| Recovery units | Cooldown |
|---:|---:|
| 1 | 3 seconds |
| 2 | 6 seconds |
| 3 | 9 seconds |
| 4 | 12 seconds |

Families covered: parent recovery (Angel/Demon/Cyborg), Phoenix recovery,
crafted recovery (Moogle/Deathknight), lineage recovery
(Gargoyle/Kuei-jin/Wraith), and reconstructed Werewolf area-gift recovery.
Existing command waits still apply; for example, an area's 24-pulse wait
is not reduced just because its recovery marker lasts 12 pulses.

The game heartbeat advances these timers once per pulse, including outside
combat. Minute updates explicitly skip them. Ordinary buff/debuff durations
are unchanged, and Book Mage's native damage-chain wait remains 15 pulses.
Failed/blocked casts retain their existing atomic cost behavior.

Dispel Magic does not clear combat recovery. It still removes ordinary
magical affects and restores innate passives. `affects` displays combat
recovery in seconds rather than mislabeling pulses as hours.

## Save compatibility

No player-file field or class/skill ID was added or renumbered. The existing
named `AffectData` record is used. Only the five private recovery identities
use `APPLY_NONE`, no bits, and modifier `-1` as a pulse-clock marker.

Marked records preserve bounded remaining pulses when loaded. Old
modifier-zero recovery records convert to three-second units exactly once,
with at least one unit retained instead of giving a reconnect reset.
Repeated normalization does not multiply an already converted timer.
Invalid or wrong-class kit state continues to follow its existing cleanup
rules. Time is saved as remaining online pulses, not an offline timestamp.

## Verification

Run the complete strict ASan/UBSan regression and three paired-seed matrices:

```sh
GENESIS_DUEL_RESULTS=/tmp/genesis-balance-results \
CRYPT_LIBRARY=/path/to/libcrypt.so \
sh tests/test-combat-system.sh balanceverify
```

`all` runs the 22 linked regression suites without the matrices; `balance`
runs only the new focused regression. Leak detection stays disabled for the
legacy memory pool, matching the existing test runner.

The final verification run passed all 22 suites and 300 fresh-process fights.
Focused checks cover shared absorption without double application, physical
versus spell defenses, raw self-harm, exact cooldown expiry, no minute
double-decrement, remaining-time save-record loading, old-record conversion,
idempotence, Dispel Magic, ordinary buff clocks, and actual normal/Chaos Book
Mage chants with deterministic paired rolls.

The first diagnostic failures were test-fixture errors: a body implant
without its required limb rank was invalid, and a disconnected standing
player was correctly protected by native linkdead safety. Both fixtures were
corrected; no production safety checks were relaxed.

## New controlled matchup results

Same 50 paired seeds, both opening attackers; each row is 100 fights.
Wraith retains the original matched-armor setup. Demon and Book Mage retain
fabricated Cyborg class armor. Therefore these rows do not rank the three
opponents directly against one another.

| Opponent | Cyborg wins | Opponent wins | Unresolved | Mean seconds | Mean Cyborg HP left |
|---|---:|---:|---:|---:|---:|
| Wraith | 100 | 0 | 0 | 56.76 | 84,041.46 |
| Demon | 100 | 0 | 0 | 44.07 | 71,365.39 |
| Normal Book Mage | 100 | 0 | 0 | 144.60 | 27,584.81 |

Previously Book Mage won 99/100. That comparison changed both mitigation and
recovery simultaneously, so the new outcome does not isolate their separate
contributions.

Cyborg now performs 13–26 successful resource-spending combat actions against
Wraith, 11–21 against Demon, and more frequent actions against Book Mage,
rather than the previous one or two. No infinite resource refill or combat
wait reset was introduced. The harness now advances short cooldowns during
preparation and combat, and no longer zeroes waits at each minute update.

These are fixed-build/rotation experiments at 100,000 base HP/mana, not a full
nineteen-class balance certification. The shared-rule repair does **not**
make the classes balanced: the remaining Cyborg advantage needs a separate,
explicit class-number tuning pass.
