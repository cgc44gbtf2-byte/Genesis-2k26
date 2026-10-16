# Reconstructed abilities: new conservative rules

The supplied source did not contain recoverable effects for these eight powers.
These replacements were implemented after approval to design conservative
mechanics. They are deliberately limited, not an assertion of original God Wars
or tabletop balance.

## Rules

| Power | Prerequisite | Mana per cast | Effect |
| --- | --- | ---: | --- |
| Disquiet | Werewolf, Homid 4 | 1000 | Single enemy: -15 hitroll and -10 damroll; duration 1 |
| Quicksand | Werewolf, Red Talons 4 | 1500 | Eligible room enemies: +2 combat-delay units; duration 1 |
| Lycaon | Werewolf, Red Talons 5 | 2000 | Normal-form player becomes a temporary spirit-cursed wolf; -10 hitroll and +2 combat-delay units; duration 1 |
| Totemgift | Werewolf, Metis 5 and a learned gift of the selected tribe | 1500 | Noncombat tribal spirit apparition grants +20 armor and +15 hitroll; duration 2 |
| Scream of Gaia | Werewolf, Get of Fenris 4 | 2000 | Eligible room enemies: 8-pulse command delay and -10 hitroll; duration 0 |
| Horridreality | Vampire, Chimerstry 7 | 2000 | Single enemy: -25 hitroll, -15 damroll and an 8-pulse command delay; duration 1 |
| Maskempathy | Vampire, Presence 7 | 1000 | Aura privacy and resistance to the three reconstructed mental attacks; duration 2 |
| Strength of Purpose | Werewolf, Philodox 2 | Passive | +10 percentage points to the existing spell-saving chance, with the same 85% cap |

Disquiet, Lycaon and Horridreality accept a target name, or use the current
opponent when no target is given. Quicksand and Scream affect only eligible
enemies present in the room when cast. No persistent room flags are changed.
Quicksand requires outdoor solid ground, not indoors, air or water.

Totemgift uses the character's selected tribe and a bounded table of named
totems. It is an aura/apparition, **not a combat mobile or inventory-bearing pet**.
`totemgift dismiss` removes its effects without a mana cost.
Repeating Maskempathy removes its mask without a mana cost.

## Timing, stacking and resource limits

- All seven active powers impose a caster command delay: 12 pulses for single
  target/self powers and 24 pulses for area powers. At the default rate these
  are about 3 and 6 seconds. A Scream/Horridreality victim's 8 pulses is about
  2 seconds, not permanent paralysis.
- Duration values use the existing character-update clock, normally 60 seconds.
  Duration 0 expires at the next update; 1 lasts roughly 1-2 minutes; 2 lasts
  roughly 2-3 minutes. Offline time does not consume duration.
- Quicksand and Scream share a duration-1 `gift recovery` effect. An area cast
  charges mana once, not once per target. The cooldown applies even if all
  eligible enemies resist. No eligible enemies means no mana or cooldown cost.
- The same power cannot stack on one target. Different powers may coexist.
  Totemgift cannot summon multiple overlapping auras. Maskempathy is a toggle.
- Hostile attempts start combat through the existing combat helper. A resisted
  attempt still costs mana and caster delay. Invalid/protected targets,
  insufficient mana, existing effects, and invalid Lycaon forms cost no mana.
- Each hostile effect uses the existing level-75 saving-throw calculation.
  Maskempathy additionally resists Disquiet, Scream and Horridreality outright.
  Those are the reconstructed mental effects; this does not add immunity to
  every older mental power, fear spell, or control command in the game.

## Protection and reversibility

Hostile gifts exclude the caster, their group, staff, protected/non-PK players,
and safe-room targets. Both player caster and player target must be PK-enabled.
NPC eligibility still follows the existing combat safety checks. These rules
apply even when older combat exceptions permit an existing fight to continue.

Lycaon:

- Only affects normal-form player characters; existing transformations,
  disguises and nonempty morph appearances are refused.
- Changes a dedicated form bit, a timed polymorph affect and the appearance.
  It does not change class, base identity, inventory, equipped items, weapon
  values, or permanent statistics.
- Cannot be overwritten using the registered class-form handlers. The Mage
  Polymorph Other handler also rejects an actively cursed target. The new
  reconstructed active powers cannot be channeled while cursed.
- Natural expiry, Dispel Magic and Remove Curse reverse its timed effects.
  Staff Unpolymorph also removes it. Central affect-removal cleanup restores
  appearance and removes only its own form bit, preserving an unrelated form
  installed by staff.
- Named affect records, form flags and appearance use the existing textual save
  format. Logging out does not evade the remaining duration.

Maskempathy blocks other non-staff characters' Read Aura assessment while
active. Staff can still inspect the aura, and it does not prevent ordinary
visibility, combat, scrying or character identification.

## Compatibility and tests

The new effect records are appended to the skill table; existing skill entries
and object spell-slot numbers are not reordered. They are level-99 effect
records rather than generic spells available through `cast`. No external
dependency, database migration or deployment is required.

`python3 tests/class_abilities.py` includes the reconstructed-power tests, using
only temporary accounts and area files. It covers prerequisite and protection
failures, effects, area recovery, anti-stacking, toggles, the saved Lycaon curse,
Remove Curse, and normal update expiry. Tests may take several minutes.
`sh tests/test-resolve.sh` uses controlled random rolls with the real saving-throw
and auspice functions to verify Strength of Purpose deterministically. It also
checks the actual affect-removal handler preserves unrelated form flags and
cannot subtract a missing Lycaon bit into a negative form value.

These are focused regressions, not a complete security, combat or class-balance
audit. Preserve backups and test the new rules on a private server before public
use. No hosting or production deployment is performed by this maintenance work.
