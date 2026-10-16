# Genesis classes and abilities — development guide

Genesis currently represents these identities as **classes/parents**, not a
separate race-selection system. There are 19 standalone parent identities.
The table summarizes every parent; the following sections give the complete
active-command lists for the five newest kits.

This is a development guide, not a claim of literal upstream parity or final
PvP balance. Legacy abilities and reference extensions do not all have the
same verification coverage. New names below have native Genesis effects;
some mechanics were intentionally adapted for safety and compatibility.

## All 19 paths

| Class | Main abilities and systems |
|---|---|
| Vampire | Blood and clan disciplines: Animalism, Auspex, Celerity, Chimerstry, Dominate, Fortitude, Necromancy, Obfuscate, Obtenebration, Potence, Presence, Protean, Quietus, Serpentis, Thaumaturgy and Vicissitude. |
| Werewolf | Breed, auspice and tribe gifts; shapeshifting, Primal Anger, Burrow, Spirit Ward, Jam Technology, Blur, Mother’s Touch, Wyld gifts, spirit interaction and tribal attacks/defenses. |
| Sorcerer | Color-based book magic, chants, magical writing, crafting, enchantment and vision. Normal and Chaos Book Mage playstyles; free one-time primer, three starter spells and inscription instructions. Distinct from sphere-based Mage. |
| Drow | Acid Bolt, Faerie Fire, Summon Spider, Globe of Darkness, prayer, Door, armor and ranged shooting; discipline progression. |
| Highlander | Blade Flash, Bash, Hilt Twist, Judgement, Lunge, Forge and Increase progression. |
| Mage | Spheres and Arete; Rift, Analyze, invisibility, spirit senses, Step Sideways, healing, Better Body, enchantment, Polymorph Other, Friction Curse, Electrical Chaos, Inferno, Contingency and Double Cast. |
| Dragon | Dragonform, breath attacks, Dust, tail attacks, horns, wings and dragon-skill progression. |
| Ninja | Principles and Michi progression; Kakusu, Mienaku, Tsumazuku, Tsume, Shuurai, Hakunetsu and Harakiri among the ki/stealth/combat commands. |
| Monk | Aura Shot, Blitz, Cripple, Darkblaze, Stun Fist, Chimera, Healing Touch, Almighty, Celestial and Life Cloak; monk training. |
| Elemental | Fire, water, wind, earth and light/dark-themed powers: Flare Arrow, Fireblade, Icicle Storm, Water Travel, Aqua Barrier, Wind Wings, Wind Blades, Stone Blast, Local Quake, Holy Aura, Solar Flash, Ultima Wave, Slow and related defenses/senses. |
| Angel | Aura, Claws, Angelform, Feet, Nightsight, Wings, Might, Toughness, Speed, Travel, Shadowsight, Emergency Inferno, Windwall, Heightened Senses, Read Aura, Spiritshare, Strengthen, Guardian Angel, Purify Spirit, Empower, Angel Armour and Halo are marked implemented in the parent inventory. Further reference extensions have separate scope/coverage. |
| Cyborg | Complete 60-entry Genesis kit: 24 implant/replacement components, humanity/dependencies, nanites, repair, rifle/factory systems, face/limb/body upgrades, explosives, sensing, concealment, webbing, transport, 13 gear variants and native passives. See CYBORG_KIT.md; hybrid conversion remains disabled. |
| Demon | Fangs, claws, Demonform, hooves, horns, tail, wings, senses, Might/Toughness/Speed, infernal attacks, fear/chill and defensive aspects. The source also contains familiar/object-form and soul/pact/warp/gift extensions; do not treat all reference inventory entries as equally parity-verified. |
| Phoenix | Ember Bolt, Feather Ward, Flaming Wings, Ashwalk, Phoenix Tears, Solar Flare, Renewal and Ascendant Aspect. |
| Gargoyle | Visceratika, physical stone resilience, ground awareness, stone-body traits, Stone Fist, Stone Wings, Stone Rest and Living Statue. |
| Kuei-jin | 18 independent tracks, Dharma progression, Chi, Blood/Bone/Jade/Flesh/Ghost Shintai, Yin/Yang/Hun/Po, noble disciplines, battle aspects, spirit senses, travel, healing, chi crafting and Akuma. |
| Deathknight | Independent soul-powered martial kit: Deathstrike, Graveward, Soul Harvest, Frostedge, Dread Gaze, Unholy Might, Soulrend, Last Stand, Doomblade and Deathcoil. |
| Moogle | Pom attacks, protective magic, healing, party morale, fortune, dancing, Flutter and home travel. |
| Wraith | 11 Arcanoi tracks; concealment, phantom flight, controlled travel, spectral bonds, soul reading, animated object forms, phantasms, fire/force attacks, weather control and supportive Usury. |

## Book Mage onboarding

`autoclass sorcerer` grants the ordinary starter primer.
`autoclass chaosmage` (or `autoclass sorcerer chaos`) selects the Chaos playstyle.
Existing Sorcerers can claim their one-time kit using `sorccreate starter`.
Both have Spark, Mend and Minor Ward plus two instruction pages and a pen.
Chaos books reroll page colors per chant without changing a healing page into
an attack or changing a blessing's purpose. See `BOOK_MAGE_STARTER.md` for the
complete commands, costs, training and safe role-preservation rules.

## Five new kits: complete active lists

These lists total **113 active commands**. They include utility, crafting,
return-form and supportive actions, not 113 distinct damaging attacks.
Training, focus and inherent passives are additional systems.

### Gargoyle — 4 active commands

- `stonefist`: bounded physical attack through native combat.
- `wings`: temporary flight with independent flight ownership.
- `stone_rest`: bounded self-healing while at peace.
- `statue`: animated object form; `statue back` safely returns.

`visceratika` trains the independent track to 5. Physical damage reduction
is 5% per rank, capped at 25%. Ground awareness begins at rank 2 and applies
when both observer and target are grounded. Native hidden/earthmelded targets
can be noticed; this does not grant administrator visibility.
Stone-body mastery adds armor and bounded derived CON/STR traits.
Genesis has no matching reference limb-injury subsystem, so this is a
resilience adaptation, not a claim of imported broken-limb immunity.

### Kuei-jin — 48 active commands

| Family | Commands |
|---|---|
| Blood Shintai | `permeate`, `atemi`, `bloodlash`, `bloodawaken` |
| Bone Shintai | `corpseskin`, `whitetigercorpse`, `boneblade`, `bonearmor`, `visage`, `poison_cloud`, `chingshih` |
| Jade Shintai | `placate`, `harness`, `jadeservant`, `kotearmor` |
| Flesh Shintai | `extendedfingers`, `detatchment`, `lotuscloud`, `pelesit`, `thousandforms` |
| Ghost Shintai | `goblinface`, `goblinspark`, `goblinscorch`, `hima` |
| Yin/Yang/Hun/Po | `huntingsnake`, `piercetheshroud`, `yangheal`, `lifesight`, `sharpensenses`, `unleash`, `demonfangs`, `shadownature`, `unshadownature` |
| Other discipline powers | `adjustbalance`, `ridethedragon`, `chirift`, `chiweapon`, `yangmantle`, `scarletqueen`, `shroudedmoon`, `shadowcompanion`, `ebondragon`, `windwalker` |
| Rites and Chi | `awaken`, `enlighten`, `uplift`, `akuma`, `consume` |

Tracks: Yin, Yang, Hun, Po, Blood, Bone, Jade, Flesh, Ghost, Equilibrium,
Tapestry, Yang Prana, Yin Prana, Black Wind, Chi-Muh, Internalize,
Dragon Scales and Demon Art. Track levels cap at 5; Dharma caps at 5.
A track cannot exceed Dharma + 2.

**Akuma is an original completed design**, not recovered upstream code.
It requires Demon Art 5 and Dharma 5, spends 2,500 mana and 800 Chi,
and grants a temporary STR +2 / damage-roll +50 aspect with an armor
tradeoff and three-tick recovery. It does not change the player's class,
alignment, administrator rights or hybrid identity.

`detatchment` retains the upstream command spelling. It uses an owned,
bounded scout, not private input monitoring.

### Wraith — 40 active commands

| Arcanoi | Commands |
|---|---|
| Argos | `enshroud`, `phantomwings`, `flickering`, `jump`, `oubliette` |
| Castigate | `soulsight`, `coax`, `secrets`, `purify` |
| Embody | `phantommask`, `statue`, `materialize`, `imitate` |
| Inhabit | `objectride`, `itemjump`, `gremlinize` |
| Keening | `dirge`, `ballad`, `crescendo`, `requiem`, `muse` |
| Moliate | `rend`, `bodyshape`, `sculpt` |
| Outrage | `pinging`, `wraithgrasp`, `stonehand`, `pyro`, `obliviate`, `martialry` |
| Pandemonium | `weirdness`, `befuddlement`, `ether`, `tempus` |
| Phantasm | `dreams`, `phantasmagoria`, `agon` |
| Puppetry | `skinride`, `embracesoul` |
| Usury | `charitable` |

The six previously absent/commented power bodies were filled with original
Genesis designs:

- **Charitable:** bounded healing for self or a willing party member.
- **Muse:** temporary WIS/saving support for a willing party member.
- **Imitate:** a bounded evasive aspect, not impersonation of another player.
- **Sculpt:** temporary armor/saving protection.
- **Martialry:** a temporary offensive aspect with an armor tradeoff.
- **Gremlinize:** a legal damaging/disrupting attack with a saved weakening effect.

`ether rain|lightning|clear` changes weather outdoors. `materialize` returns
from an owned object/aspect. `objectride` uses known animated-form choices,
not arbitrary item ownership takeover.

### Deathknight — 10 active commands

- `deathstrike`: physical attack.
- `graveward`: armor/saving protection.
- `soulharvest`: bounded energy from an NPC corpse, once per corpse.
- `frostedge`: temporary real equipped-weapon bonus and saved chill effect.
- `dreadgaze`: saved offensive-roll penalty.
- `unholy_might`: temporary STR/damage boost.
- `soulrend`: attack with saved saving-throw weakness.
- `laststand`: bounded self-healing only below a quarter of maximum health.
- `doomblade`: attack with a saved damage-roll penalty.
- `deathcoil`: bounded magical attack.

This is an independent Genesis class, not ADG's Demon/Highlander class-bit
combination. Mastery caps at 8; inherent Deathbound traits are derived,
reversible and do not permanently alter base statistics.

### Moogle — 11 active commands

- `pom_bolt`, `pomflare`, `pomnova`: progressively stronger pom attacks;
  Pomflare can apply saved blindness.
- `pom_ward`: armor/saving protection.
- `kupo_heal`: bounded healing for self or a willing party member.
- `cheer`: nonstacking party hit/damage support.
- `lucky_star`: temporary armor/saving protection.
- `mogdance`: temporary offensive boost with an armor tradeoff.
- `flutter`: temporary flight.
- `kupostep`: peaceful travel to the player's stored, unprotected home.
- `serenade`: bounded party healing and poison removal.

This is an original Genesis kit; no missing ADG Moogle implementation was
claimed recovered. Mastery caps at 8; Pomcraft grants bounded derived traits.

## Commands and safety

Examples:

```text
autoclass gargoyle
gargoyle
gargoyle train visceratika
gargoyle focus 1000

autoclass kuei-jin
kueijin
kueijin dharma train
kueijin train blood
kueijin bloodlash
kueijin bloodlash off

autoclass wraith
wraith arcanoi argos
wraith enshroud
wraith statue
wraith materialize
```

The new five require the existing awakened eligibility and base
STR/DEX/CON/INT/WIS of at least 2/2/3/3/2. Enrollment is for unclassed players,
not a respec mechanism. Class menus show exact mastery, mana and energy costs.
Focus converts mana into class energy at 1:1 with caps and recovery.

Most aspects, shells, edges, sight effects and concealment have exclusive
family slots rather than allowing every named version to stack.
Most temporary lineage self-effects end on reload; mastery, Dharma and energy
use versioned independent records, and valid recovery is retained.

Travel respects protected rooms and targets willing party members; source
unrestricted portals, abduction and possession were not copied. Skinride
is controlled self-travel to a party member, not control of their character.
Ballad reads permitted status, not typed input. Crescendo uses an owned scout.
Oubliette/Wraith Grasp use temporary movement bonds rather than kidnapping.
Rend/Gremlinize weaken rather than importing a nonexistent limb-severing model.
Dreams can sleep an eligible NPC; player targets receive a saved distraction,
not forced player control. Rites grant bounded help, not administrative ranks.
Masks/aspects do not impersonate player identities. Ether retains weather
control; there are no empty success-message-only implementations for the
listed new commands.

## Hybrid and ascension status

The 19 identities define **171 possible two-parent pairs**, not 171 playable
hybrids. The current hybrid interface remains a read-only preview.

The agreed future design is exactly two parents with all abilities from both,
plus a comparable ascended-pure alternative and expensive path respecs.
Hybrid conversion, pure-class ascension and those respecs are not enabled.
Phoenix's existing temporary **Ascendant Aspect** is a normal Phoenix power;
it is not the planned pure-class ascension system.

## Source and scope

- Roster: `src/parents.c`.
- New five kits: `src/crafted_powers.def`, `src/crafted.c`,
  `src/lineage_powers.def`, `src/lineages.inc`, `src/lineage_registry.def`.
- Persistence: `src/crafted_state.*`, `src/lineage_state.*`.
- Existing parents and reference inventory: `src/parent_powers.def`,
  `src/newclasses.c`, `src/reference_abilities.inc`, `src/reference_legacy.inc`.
- Legacy class implementations: class-named C files, `clan.c`, `disc.c`,
  `wwbreed.c`, `wwauspice.c`, `wwtribe.c`.
- New-kit effect regressions: `tests/test_crafted.c`, `tests/test_lineages.c`.
- Full socket/enrollment/save/relogin checks: `tests/crafted_classes.py`.

References supplied by the user inform themes and mechanics, but the new
lineage bodies are independent Genesis conversions/reconstructions, not
verbatim imported implementations. Preserve the original game's license
and credit notices. This guide does not certify every legacy/reference
variant, production deployment, live-player migration or PvP balance.
