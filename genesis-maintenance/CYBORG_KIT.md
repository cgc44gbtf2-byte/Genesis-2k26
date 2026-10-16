# Cyborg — complete Genesis kit

This is the canonical current Cyborg guide. It supersedes earlier
backend-only and partial-port status in the historical implementation notes.
All **60 catalogued entries** have Genesis implementations and regression
coverage. This includes components and training/management systems, not 60
different attacks. Aliases are not counted twice.

These are conservative Genesis adaptations of the catalogued Vortex/Mindcloud
mechanics, not a claim that every command or numeric rule in every upstream
version was copied. Existing source credits and license notices remain.
Standalone Cyborg is playable. Hybrid conversion remains disabled.

## Start and progression

Use `autoclass cyborg`, then `cyborg status`.
All commands below use the `cyborg` prefix.

- `install <component>` installs an implant/replacement; `implant` and `replace`
  are aliases. `uninstall <component>` refuses removals with installed
  dependents, restores humanity once and never refunds primal.
- `train` increases rifle mastery to 12; cost is
  `10000 * next_rank * next_rank` experience.
- `convert <amount>` converts mana into nanites with a battery/factory,
  capped at 10,000 stored nanites.
- `bionics <amount>` converts experience into bionic training currency,
  capped at 10,000,000 stored bionics.
- `limb_implants improve`, `body_implants improve`, `face_implants improve`
  train independent tracks to 5/6/5, costing `12500 << current_rank` bionics.
  Body needs a limb rank; face rank 5 requires body rank 4.
- `demolitions improve` trains to 12, costing `15 * next_rank` primal.
  `show` displays a track without purchasing it.

Modifications/training require peace, standing, sufficient resources and a
living usable body. Dependency, currency and humanity failures are atomic.
The ordinary humanity budget is 100: all components are available, but they
are not intended to be installed simultaneously within that budget.

## All 24 components

| Component | Genesis role |
|---|---|
| heart | +1 CON; adrenal prerequisite |
| liver | Factory-assisted removal of named poison for 25 nanites |
| stomach | Nanite-factory prerequisite |
| neuroprocessor | +1 INT; neural-system prerequisite |
| battery | Bounded nanite storage and powered-system prerequisite |
| nanite_factory | Conversion, factory production, repair and bomb fabrication |
| left_arm / right_arm | +5 armor each; installed weapon prerequisites |
| left_leg / right_leg | +1 DEX each; missile prerequisites |
| eyes | Ordinary night sight; optical-system prerequisites |
| ears | Concealment perception; radio prerequisite |
| adrenal_boost | +5 damage roll |
| boosterware | +5 hit roll |
| boostmaster | +10 hit roll |
| wolvers | Automatic +10 damage roll from installed wrist blades |
| chameleon | `cloak`, bounded timed concealment |
| muscle | +1 STR |
| bonelace | +20 armor |
| radiosplice | `radio <message>`, bounded Cyborg-only channel |
| targetcomp | +10 hit roll; missile targeting |
| teleoptics | +5 hit roll; railgun prerequisite |
| railgun | `snipe <target>` |
| taser | `taser <target>` |

Armor is positive in Genesis's avoidance calculation. Component traits rebuild
without stacking on installation, removal, load and relevant cleanup.

## Weapons, nanites and explosives

| Command | Requirements and effect |
|---|---|
| `factory on/off` | Installed factory; generates up to 100 nanites/tick for equal mana, storage-capped |
| `repair` | Factory; 400 nanites; heals up to `300 + 30 * rifle`, capped at missing HP |
| `taser <target>` | Installed taser; 200 nanites; native damage and save-dependent temporary accuracy penalty |
| `snipe <target>` | Railgun and rifle rank 1; 300 nanites; same-room native damage, no caster relocation |
| `make_bomb` | Demolitions 1, factory, 1,000 nanites; creates a safe charge, maximum 20 |
| `arm` / `arm off` | Demolitions 1 and a charge; arm or safely disarm without losing stock |
| `detonate <target>` | Demolitions 1 and armed charge; native damage, consumes one charge on valid use |
| `mine` | Demolitions 1 and armed charge; consumes one for a three-tick defensive, room-bound deployment |
| `launch <target>` | Demolitions 1, targeting computer, both replacement legs; 1,500 nanites; native missile damage |
| `stuntubes <target>` / `oilspit <target>` | Body 5 and limbs 5; 1,000 nanites; native damage and save-dependent named, removable poison |

Targets pass native safety, visibility and PK checks. Self/group/protected
targets are rejected without spending ammunition or power costs. Damage uses
native handling rather than direct HP subtraction; no post-damage target
dereference. A mine does not attack bystanders and cannot be moved to a new room.

## Reference active abilities

| Command | Requirement; Genesis behaviour |
|---|---|
| `cubeform` / `terminator` | Body 5; 2,000 mana + 2,000 nanites; +35 hit/damage for three ticks |
| `mechsight` | Face 1; 250 mana; timed optical enhancement, with total-darkness vision at face 3 |
| `scry <target>` | Face 3; 500 mana; remote location information without moving the caster |
| `read_aura <NPC>` | Face 4; 100 mana; visible local NPC alignment and health, not player alignment |
| `lloth` | Face 4; 1,500 mana; +15 hit/damage for three ticks |
| `darkness` | Body 3; 1,000 mana; timed room darkness while its living owner remains there |
| `web <target>` | Face 2; 500 mana; saving throw, named removable two-tick web |
| `transmit <ally>` | Face 5; 1,500 mana + 500 nanites; protected travel to a remote consenting grouped player |
| `shadowwalk` / `upload` | Body 4; 1,000 mana; timed concealment, not a second shadow-plane travel system |
| `cyborgeq <piece>` | Body 1; 100 primal; bound equipment from the Genesis protoplasm template |

Travel respects room privacy/wards, plane, combat, curse/web/root and usable-body
restrictions. Sensory effects do not grant administrator visibility.
Cubeform and Lloth are timed combat adaptations, not raw permanent stat changes
or imported movement-resource transformations.

Equipment pieces: whip, ring, collar, armor, helmet, leggings, boots, gloves,
sleeves, cloak, belt, bracer, mask. Duplicate carried pieces are refused.
Missing templates are a clean failure, without charging primal.

## Nine automatic passive entries

- `armor_plating`: +20 armor per body rank; body 1 also absorbs native damage to 4/7.
- `limb_damage`: +30% melee damage at limbs 1; another +50% of that result at limbs 3.
- `limb_speed`: native attack-speed count reduced by limb rank.
- `limb_damage_cap`: +450 per limb rank through 4, then +500 at rank 5.
- `limb_defense`: +10 armor per limb rank.
- `limb_accuracy`: +10 hit roll per limb rank.
- `weapon_proficiency`: weapon training ceiling `100 + 20 * limbs`, maximum 200.
- `regeneration`: body 6 heals up to 300/tick, limited by missing HP and available nanites.
- `darkvision`: face 3 with active Mechsight sees native total darkness.

Calling these entries reports their trained status; their gameplay effects use
native combat, perception, training and update hooks.

## Persistence and verification

Independent versioned `CyborgCore`, `CyborgRuntime` and `CyborgSystems` records
preserve installed components, permissions/humanity, rifle/nanites/factory,
face/limb/body/demolitions/bionics and bomb/armed state. Strict codec tests reject
malformed, oversized, binary and orphaned state without swallowing following
player fields. Loading rebuilds derived traits without stacking and removes
ground mine deployments; ordinary save/relogin does not duplicate inventory.

Verification entry points:

    sh tests/test-cyborg-core.sh
    sh tests/test-combat-system.sh all
    python3 tests/class_abilities.py

The linked suites exercise all 24 components, every track/cap, all 13 gear
variants, nine passive entry points, real combat/status/sensing/transport,
aliases, native saves and failure atomicity. The dedicated completion suite
adds grouped-target protection, radio's full-message dispatch, incapacitation,
real web/poison and removal, mine locality, ammunition bounds and weapon caps.
Native tests cover live command routing, training, read-only 60-entry preview
and save/relogin of hardware, resource and reference-system state.

This completes the catalogued Cyborg kit, not the separate broader legacy-class
audit, hybrid/ascension implementation, every possible upstream variant or
long-term multiplayer balance/playtesting.
