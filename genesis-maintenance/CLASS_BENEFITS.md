# Class benefits and progression checks

Training must do something useful, but not every rank is a flat stat buff.
Some ranks unlock attacks, defense, healing, travel, perception, crafting
or resource management. Many passives start at a named threshold rather
than increasing on every rank. Active powers still need activation,
resources, legal targets and their ordinary prerequisites.

## Current findings

Two actual Werewolf wiring defects were reproduced with failing native
assertions, then corrected:

| Skill | Fault | Correction |
|---|---|---|
| Philodox 3 | Damage cap read a tribe slot using an auspice constant | Reads Philodox auspice; +100 cap at rank 3; unrelated tribe cannot grant it |
| Theurge 4 spirit orders | Named order required being both Vampire and Werewolf and used a Vampire discipline getter | Shared named/all NPC-spirit gate reads Theurge; cannot commandeer players or someone else's follower/familiar |

The Theurge order check applies to visible NPC spirits. A spirit with a
different master or wizard owner is excluded. Existing charm/consensual
order routes are separate; this change does not give unrestricted control
of player characters. Rank four preserves the existing execution threshold.

## Benefits by parent

This table describes representative real benefits, **not** exhaustive
positive/negative certification of every ability and target variant.

| Parent | Benefit / mechanism | Evidence |
|---|---|---|
| Vampire | Potence damage/cap; Fortitude armor/soak; active Celerity speed/hit | Benefit regression and existing legacy variants |
| Werewolf | Rank/tribe combat benefits; auspice unlocks | Benefit regression; Philodox/Theurge boundary and authority tests |
| Sorcerer / Chaos Book Mage | Color-dependent native chanting; usable healing/protection and written spells | All color getter ranks; existing starter-book healing/ward/casting suite |
| Drow | Speed training changes attacks, damage rolls and cap; prayer unlocks | Benefit regression; existing Drow variant tests |
| Highlander | Attack training changes speed, rolls/cap; Defense changes armor/soak | Benefit regression and all four getter tracks |
| Mage | Mind adds cap; Time improves speed; sphere ranks unlock commands | Benefit regression; existing sphere effect/argument suites |
| Dragon | Dragon form changes rolls, armor, cap, soak and speed | Benefit regression; existing parent/form tests |
| Ninja | Ningenno speeds attacks; Chikyu raises cap; principles unlock powers | Benefit regression; all three getters; existing Ki/variant tests |
| Monk | Art thresholds improve rolls, armor, cap, soak and speed | Benefit regression; all three getters; existing skill tests |
| Elemental | Fire/Infinity raise cap; Earth raises soak; Wind/Time affect speed | Benefit regression and existing elemental tests |
| Angel | Mastery derives STR/CON/DEX passives; activated support/defense | Benefit regression and existing parent/reference effect suites |
| Cyborg | Limbs/body add real cap/speed/roll/armor benefits; implants grant traits | Benefit regression and dedicated component/reference completion suites |
| Demon | Mastery derives STR/CON/DEX passives; active shapes/warps/powers | Benefit regression and existing parent/reference effect suites |
| Phoenix | Mastery unlocks paid flame powers, protection and flight | Rank-zero rejection/rank-three real flight; existing full Phoenix suite |
| Gargoyle | Visceratika adds CON/STR/armor and physical absorption | Benefit regression, including 1,000 physical damage reduced to 750 at rank 5 |
| Kuei-jin | Track thresholds derive CON/STR/WIS; disciplines unlock powers | Benefit regression with valid Dharma state; existing lineage effects |
| Deathknight | Mastery derives STR/CON/WIS; active deathbound abilities | Benefit regression and existing crafted-class suite |
| Moogle | Mastery derives INT/WIS/DEX; active healing/support/pom abilities | Benefit regression and existing crafted-class suite |
| Wraith | Arcanoi thresholds derive DEX/WIS/CON; active aspects/utility | Benefit regression and existing lineage effects |

## Regression scope

```sh
python3 tests/audit_class_skill_wiring.py
sh tests/test-combat-system.sh benefits
sh tests/test-combat-system.sh all
```

The new linked test checks observable derived rolls, damage cap, soak,
avoidance, speed and effective stats before/after progression. Repeated
passive restoration must not stack benefits or change trained base stats.
The fixture uses armor near an avoidance boundary to catch the actual
positive-armor convention; an armor increase need not change the integer
avoidance score at every starting value.

All normal combat stances and styles also have a tested derived benefit.
Actual weapon availability/routing remains covered by the separate
attack-style suite. All native ranks of the discipline, breed, auspice,
tribe, color, blademastery, principle and Monk getters are checked.

The source audit checks literal family/slot references throughout C/include
modules and rejects cross-family getters/arrays. Dynamic indices and
correctly named but semantically wrong formulas still require runtime
tests and review. Neither a registered command nor a passing getter proves
that every ability's effect, item route or saved-player variant is correct.
The existing older-kit variant gaps in `CLASS_KIT_STATUS.md` remain open.
No new flat bonuses, training prices or hybrid/ascension mechanics were added.
