# Angel, Cyborg and Demon: playable development kits

Current defined Genesis catalogues: **Angel 33/33, Demon 71/71, Cyborg 60/60**
implemented with targeted effect tests. All non-system Angel/Demon grants
also have cost and player-parser save/reload coverage. See CYBORG_KIT.md and
CLASS_KIT_STATUS.md. Earlier partial counts in this document are historical.
This is not universal legacy-variant certification; hybrid conversion remains
disabled.

This development batch adds working standalone entry, training and initial
effects for all three missing parents, plus the expanded Angel/Demon powers
described below. The roster is now the existing ten,
Phoenix, Angel, Cyborg and one Demon. This historical batch covered fourteen
parents. The current roster adds Gargoyle, Kuei-jin, Deathknight, Moogle and
Wraith: **19 parents and 171 planned pairs**; see `CLASS_GUIDE.md`.
Cyborg is both a standalone class and a hybrid parent. No full hybrid
conversion, pure-class ascension or path respec is enabled. The 75%-selection
proposal has been superseded by two-parent, full-kit inheritance.

**This is NOT completion of every reference ability.** New effects are
conservative Genesis reconstructions, not verbatim reference implementations.
The inventory distinguishes implemented adapters from pending powers.
Historical initial batch: Angel had 22 implemented entries of 33, Demon 27
of 71, Cyborg 32 of 60. These counts are superseded by the current totals above.
Cyborg's count includes 24 components and management systems: it is not
32 independent selectable abilities or a finalized 75% denominator.
This expansion adds 11 Angel and 11 Demon entries. Names shared by both
parents and sight/ward variants are not claims of 22 unique mechanics.
It also corrects stale inventory flags for the previously working initial
kits; that correction does not represent newly implementing those old effects.

## Entry and progression

Use `autoclass angel`, `autoclass cyborg`, or `autoclass demon` after the
existing awake/unclassed gate. Each requires trained base STR/DEX/CON/INT/WIS
of at least 2/2/3/3/2. Extra clan arguments are rejected. No existing class
is automatically converted or reset.

Angel/Demon use powers[0] for bounded mastery 0..8 in their own standalone
identity; old class indices and fields remain unchanged. Training costs
50,000 * next_rank^2 XP. Cyborg rifle training is separate runtime state,
0..12, costing 10,000 * next_rank^2 XP. Training and implant changes require
peace/standing. Invalid input and failed affordability do not debit resources.

Base nonstaff stat caps, in STR/DEX/CON/INT/WIS order:

- Angel: 9/10/9/10/11.
- Demon: 11/9/11/9/9.
- Cyborg: 10/10/11/10/9.

Staff retain the existing cap adjustment. New class flags are independent
2048/4096/8192; upstream numeric class flags are not imported.

## Cyborg commands and conservative effects

Use `cyborg` for status, costs and component names. Commands are namespaced;
they cannot override existing class commands.

- install/implant/replace <component>: exact primal/humanity debit after all
  prerequisites and permissions pass. Starts with an explicit 100 humanity
  budget; not every component can be installed simultaneously.
- uninstall <component>: dependency-safe removal, exact humanity restoration,
  no primal refund or repeat refund.
- train: rifle training.
- convert <amount>: mana to nanites, 1:1, installed factory required.
- factory on/off: installed factory required; awake living users generate up
  to 100 nanites per normal character tick for equal mana, capped at 10,000.
- repair: 400 nanites; heal min(300+30*rifle, missing HP). No overheal,
  revival, free duplicate cast or full-health debit.
- taser <target>: installed taser, 200 nanites; real damage 900+25*rifle,
  save halves it, otherwise one-tick -4 accuracy. One damage callback only.
- snipe <target>: installed railgun, rifle >=1, 300 nanites; real damage
  800+50*rifle. This first version is SAME ROOM, never relocates the caster
  and never subtracts victim HP outside Genesis damage handling.
- cloak: installed chameleon, 250 nanites, three ticks, out of combat.
  Disabling is free. Named camouflage does not overwrite hide/sneak flags.
  Attacking breaks it. Detection/staff visibility retains normal rules.
- radio <message>: installed radiosplice; bounded cybernetic channel to
  connected Cyborgs with their own installed radiosplice.

All component adapters have actual role:
heart CON+1; neuroprocessor INT+1; arms AC+5 each; legs DEX+1 each;
adrenal DAM+5; boosterware HIT+5; boostmaster HIT+10; wolvers DAM+10;
muscle STR+1; bonelace AC+20; target computer HIT+10; teleoptics HIT+5.
Battery provides nanite capacity, stomach/liver/factory form the prerequisite
and conversion chain; enabled factory plus liver spends 25 nanites to remove
named poison. Eyes provide ordinary night vision, ears detect concealment;
chameleon/radiosplice/railgun/taser activate their respective commands.
These are intentionally conservative equivalents, not claims that unknown
Vortex passive consumers have been recovered verbatim.

Installing/removing rebuilds one named permanent trait group, preventing
stacking. Loading rebuilds it from valid installed component state, never
from arbitrary saved derived traits. Uninstalling required storage/factory
disables associated resources. NPCs, other classes, malformed state and missing
player data cannot access the adapters.

## Angel and Demon commands

Use `angel` or `demon` for a rank/cost menu and `train` for progression.

Angel: aura, claws, angelform, nightsight, wings, travel, read_aura,
heightened_senses, windwall, halo, emergency_inferno.

Demon: fangs, claws, demonform, nightsight, wings, travel, read_aura,
heightened_senses, shield, demonarmour, hellfire, emergency_inferno,
unnerve, curse, rage, calm.

### Expanded powers in this batch

All commands remain namespaced (`angel <power>` / `demon <power>`). New timed
effects use appended skill indices; no old skill index, class ID or player
record format changes. Passive stat bonuses are derived, not trained stats,
and do not satisfy base-stat mastery requirements for future hybrids.

| Parent | Power | Mastery | Mana | Conservative Genesis effect |
|---|---|---|---|---|
| Both | might | 4 | 0 | Automatic +1 derived STR |
| Both | toughness | 5 | 0 | Automatic +1 derived CON |
| Both | speed | 6 | 0 | Automatic +1 derived DEX, not extra combat attacks |
| Angel | feet | 3 | 750 | +8 damage for three ticks |
| Demon | hooves | 3 | 750 | +8 damage for three ticks |
| Demon | horns | 4 | 750 | +10 accuracy for three ticks |
| Demon | tail | 5 | 1,000 | +10 protective AC for three ticks |
| Both | shadowsight | 5 | 500 | Three-tick night/concealment sight; shares senses group |
| Demon | truesight | 5 | 500 | Same bounded senses variant; no staff/privacy bypass |
| Angel | angelarmour | 7 | 1,800 | Three-tick +35 protective AC; shares ward group |
| Angel | guardianangel | 6 | 2,000 | Three-tick +40 defensive spirit ward, not an NPC summon |
| Angel | spiritshare | 3 | 500 | Transfers up to min(1,000, caster HP/10, missing ally HP) |
| Angel | strengthen | 4 | 1,200 | Heals up to 500 missing HP |
| Angel | purifyspirit | 5 | 1,000 | Adjusts an allied Vampire's Beast, bounded to 5..90 |
| Angel | empower | 8 | 3,000 | Heals up to 2,000 missing HP, only while both at peace |
| Demon | soultwist | 6 | 1,800 | Good NPC only; alignment-based damage bounded to 351..1,000 |
| Demon | frostbreath | 4 | 1,200 | Single target 450+60*mastery; unsaved two-tick -1 STR |
| Demon | netherflames | 8 | 2,500 | Single target 800+60*mastery damage |

Support powers require the caster's named angelic wings. They may help self
or visible, living group members in the same room; Spiritshare requires
another ally. Evil NPCs, unrelated targets and dead targets are rejected.
No new player alignment model or immunity bypass is introduced.
No overheal, revival, mana regeneration loop, or debit on a useless/invalid
heal is allowed. Genesis has no movement-point resource, so the reference
movement restoration is deliberately omitted. Strengthen does not grant raw
permanent pain-immunity bits; Empower does not reset unrelated wounds,
blood, attributes, alignment or equipment.

Spiritshare transfers only the actual healing delivered; it never removes
extra caster HP. Purify Spirit retains the reference's two Beast paths:
values 6..49 decrease by up to 5; values 51..89 increase by up to 10.
Values <=5, 50, >=90, non-Vampires and unrelated players are free refusals.
Grouping is the explicit ally opt-in. Healing/Beast powers add six seconds
of shared recovery, Empower nine. Body/ward/sight/attack powers add three.
Saves halve all new damage; a saved Frostbreath never adds chill.
Soul Twist deliberately refuses player targets because player alignment
has not been restored. None of these attacks directly subtracts victim HP
or bypasses the legal-target/damage pipeline.

Might/Toughness/Speed are rebuilt after training and login, never stacked
by repeated commands or reloads. Real Dispel Magic and both ordinary death
and beheading restore inherent mastery/hardware bonuses after the purge/reset,
not active powers. This also prevents installed Cyborg traits being lost
until the next login. Class clearing still removes the bonuses entirely.
Feet/Hooves, Horns and Tail have separate
named ownership; duplicate applications cost nothing. Expiry and class
clearing remove their bonuses. Loading removes unauthorized low-rank or
wrong-class self-effects. Enemy-applied chill is not removed just because
its target has no parent class. This is not a complete reference port:
summoned helpers, object/weapon forms, inter-room powers and the other
pending inventory entries remain unavailable rather than simulated menus.

Initial ward variants are mutually exclusive three-tick +20/+30 AC effects;
the expanded +35/+40 variants use the same exclusive group.
Aspect variants are mutually exclusive three-tick hit/damage buffs of
+10/+15/+20; calm removes the actor's infernal aspect only. Wings last
three ticks, preserve other flight owners and do not stack with existing
flight. Nightsight/heightened senses are named vision effects, not raw
permanent flags. Aura reading reports NPC prototype alignment only; PC
alignment and NPC-only Darkblessing rules remain unchanged.

Travel uses existing home recall only after source/destination, curse/web,
combat/fight timer and protected-room checks. No valid move means no charge.
Attacks use the real damage pipeline once; no combatant accesses afterward.
Halo can apply named one-tick blindness before damage; saves halve damage.
Unnerve applies named -4 accuracy for two ticks, curse uses the existing
named curse with -4 accuracy for two ticks. Saves resist these effects.
Inferno/hellfire are single-target conservative versions, not unsafe AoE
ports. Every active power pays mana and adds shared recovery/wait only after
validation. Lycaon's curse blocks active class powers.

## Persistence, cleanup and compatibility

Existing old player files remain valid. Empty state adds no new records.
Optional version-1 CyborgCore and CyborgRuntime records hold independent
components/humanity and bounded nanites/rifle/factory, respectively.
Unknown versions, overflow, invalid masks, invalid resources, binary or
oversized lines reset only the affected state and consume the whole line.
Unrelated following fields and legacy power arrays are preserved.

Class clearing removes new self-effects and resources. Loading removes
unauthorized self-effects and rebuilds valid Cyborg traits without stacking;
enemy-applied hindrance remains on targets regardless of their class.
All compilation units depend on the state header to avoid stale PC_DATA
layouts. New named skill entries are appended after existing indices.

## Still pending

The inventory explicitly leaves all other reference abilities unported,
including Vortex bomb creation/mines/arming/demolitions/missiles, the complete
Mindcloud face/limb/body extension and equipment, Angel object/weapon forms,
and remaining Demon summoning, portals, object forms and spell families.
Do not advertise this batch as all reference kits completed.
Parent hybrid-ready flags remain false until full catalogues, per-selected-
ability effects, selection persistence and migration are implemented.

Vortex cost/dependency facts are credited to Kale/Jalerit. Mindcloud and other
reference effect bodies have not been copied; public availability is not
permission to remove copyright or license requirements. Existing Genesis
notices remain intact.

## Verification

Strict native build and the full linked ASan/UBSan initial-kit probe passed.
The focused resolver/stat/armor/cost probe passed after declaring the new
flight-owner symbol in its isolated harness. Resource codec, all five native
suites, linked registry and Phoenix compatibility checks passed final
batch verification. Live tests include real class enrollment/training,
independent Cyborg storage across logout/login and real class-clear cleanup.
The malformed-resource fixture intentionally logs a
reset warning; it is not an unexplained runtime error.

Expanded-powers verification: the full linked ASan/UBSan initial-kit probe
also checks all 22 new entry mastery gates, threshold training, passive
nonstacking/base preservation, physical effect expiry, defensive/sight
variants, wing/ally/living-target restrictions, exact and capped healing
costs, Spiritshare conservation, both Beast bounds, protected combat,
alignment restrictions, real attack HP loss, real dispel preservation of
mastery/hardware and enemy-chill persistence. Live fixtures also check real
death removes physical powers while preserving the three mastery bonuses.
Reproduction: `sh tests/test-combat-system.sh all` builds once and runs
the combat, legacy-spell, registry, Phoenix and new-class probes in separate
sanitized processes. `sh tests/test-cyborg-core.sh` and
`sh tests/test-resolve.sh` provide independent codec/stat/cost probes.

Final expanded-powers rerun: strict native compilation with warnings treated
as errors passed. All five disposable live suites passed (smoke_runtime,
system_fixes, legacy_spells, maintenance_fixes, class_abilities). All five
linked ASan/UBSan probes passed, as did the independent resource codec and
resolver/stat/cost probes. Real death/login/class-clear checks passed for
Angel and Demon. These are regression checks, not a complete balance or
security audit. The new source-only archive is
Genesis-MUD-abilities-development.zip; earlier release/development snapshots
remain unchanged.
