# Class, training and staff-menu follow-up

## Player-facing layout

- `HELP CLASS` / `HELP CLASSES`: short overview, then 19 class bullets pointing
  to individual help pages. The individual pages retain compact tables.
- `AUTOCLASS`: colored parent names with aligned plain-text fallback and
  wrapped clan/faction choices. Unsupported Malkavian enrollment is no longer
  advertised; no class-selection handler or entry requirement changed.
- `WIZHELP`: alphabetized commands grouped by registered minimum access:
  Builder 4, Immortal 5, Implementor 6, Storyteller 7. Optional role/numeric
  filters cannot expose higher-level commands. Safety-prefix `do_huh` entries
  are omitted. Effective switched-body trust is respected; permissions and
  command-specific identity checks are unchanged.
- `WIZLIST`: current curated maintainer/bootstrap information and access legend,
  not an online staff roster. All other 83 main help bodies, including credits
  and licensing notices, are unchanged.

## Training and feedback repairs

Angel/Demon/Cyborg malformed training, Phoenix staff/body-state rejection,
Moogle/Deathknight focus/training, lineage focus/Dharma/track training, Cyborg
system purchases, and Demon soul/gift/warp purchases now give explicit
rejection or purchase receipts where feedback was missing or misleading.
Rejected purchases retain resources and ranks. Phoenix's staff-training block
now matches its existing menu. Existing prices and rank caps are preserved.

All **16 implemented Vampire discipline tracks** now have every trainable
rank exercised for ordinary, clan-favored and Caitiff pricing/caps, including
ungranted and one-short rejection. Thaumaturgy retains its special cap of 5.
Reserved numeric slots are not presented as trainable "None" disciplines.
The old general help advertised Daimoinon, Mortis and Melpominee despite no
matching native implementations; those unsupported names were removed from
the current kit summary, not newly implemented or silently granted.

`CLASS_BENEFITS.md` records the separate derived-benefit checks and repairs to
Philodox's damage-cap getter and Theurge's owned-NPC spirit-order routing.
These checks do not certify every legacy ability/argument/target combination.

## Verification

- Strict GNU89 build with `-Wall -Werror` and the documented legacy
  `-Wno-builtin-declaration-mismatch` exception.
- All 21 linked ASan/UBSan suites passed, including expanded training feedback,
  class-benefit and staff-menu checks.
- Current native class gameplay regression passed.
- Native public help: 295 entries / 662 exact topics and aliases, including the
  19 class tables, CLASSBONUSES and singular CLASS overview.
- Exact legacy-help audit: 398/398 topics. Literal native skill wiring audit:
  63 source files / 645 references / zero reported family/slot mismatches.
- No claim of universal variant coverage, production balance or actual IPv6
  network testing in this environment. Hybrid conversion remains disabled.

## Reproducible Cyborg/Wraith fight

```sh
CRYPT_LIBRARY=/path/to/libcrypt.so sh tests/test-combat-system.sh duel
```

This separate experiment uses actual command dispatch, melee/violence updates,
character ticks, resource deductions, recovery and effect expiry. It is not one
of the 21 bounded regression suites above. It creates two isolated in-memory
PCs and never connects to or modifies the live host.

Both have 100,000 base HP/mana, their class-specific stat training caps,
all weapon/stance/style mastery at 200, and maximum class track ranks.
Both use Tiger stance, dual benchmark blades and identical benchmark armor.
The gear is controlled, not a claim of maximum obtainable world equipment.
Cyborg's real 100-humanity budget fits the documented 17-implant configuration;
all 24 implants cannot be installed simultaneously at that budget.

The trace prints preparation, exact starting/final derived values, class
commands, native combat messages and HP checkpoints. Commands respect waits
and recovery. A legal configured rotation does not cast every utility,
crafting, travel, minion and mutually exclusive power in one duel.
The fixed-seed/initiative result is not sufficient evidence to rebalance a kit.

### Paired-seed follow-up

```sh
GENESIS_DUEL_RESULTS=/tmp/genesis-duel-results \
  CRYPT_LIBRARY=/path/to/libcrypt.so sh tests/test-combat-system.sh duelbatch
```

The batch driver runs 50 deterministic seeds, each twice in a fresh engine
process. It reverses the opening attacker, combat-command priority and
character-list ordering, retaining the same gear, mastery, preparation and
rotation. Every fight passed under ASan/UBSan.

| Opening attacker | Fights | Cyborg wins | Wraith wins | Mean Cyborg HP left | Mean seconds |
|---|---:|---:|---:|---:|---:|
| Cyborg | 50 | 50 | 0 | 88,128.06 | 48.54 |
| Wraith | 50 | 50 | 0 | 86,227.48 | 52.50 |
| Overall | 100 | 100 | 0 | 87,177.77 | 50.52 |

No unresolved fights. Cyborg's remaining HP ranged from 78,731 to 94,419;
fight durations ranged from 38.75 to 66.25 simulated seconds.
All starting builds were checked identical across seeds. Initial derived
values were Cyborg/Wraith: damage cap 4100/1600, hitroll 105/0, AC 500/315,
soak 600/500 and attack delay 11/15 (lower delay acts more often).
In `get_damcap`, Cyborg limb mastery alone adds up to 2300; Wraith has no
corresponding case in that switch. These are observed/code-level differences,
not an isolated causal test of each bonus.

This establishes a large Cyborg advantage for this controlled build/rotation,
not every obtainable loadout or optimal player strategy. Benchmark gear has
no additional quest/enchantment hitroll/damroll bonuses. Cooldowns and utility
power exclusions remain as described above. The next balance review should
compare progression/cap parity and class-optimized strategies, rather than
changing individual spell damage based solely on these trials.
No production gameplay code, class stats, costs, caps or balance rules changed.

### Cyborg versus Demon, using available class armor

Use `dueldemonbatch` instead of `duelbatch` in the command above. This runs
the original 100-fight Wraith matrix as a regression, then 100 Demon fights.
The Wraith rows, starting values and aggregates reproduced exactly.
Both batches passed ASan/UBSan; the two real class-power actions per fight
were verified to spend resources, not merely print success messages.

Cyborg uses all 12 distinct wearable `cyborgeq` armor types, fabricated with
the real handler for 100 primal each (armor value20). The neutral template
matches `kavir.are` #30037. Duplicate-item restrictions leave the second
ring, neck and wrist slots using benchmark armor15. Creation is fixture
construction before timed combat preparation, not a live-server transaction.
Demon has no class armor fabrication handler: `demonarmour` is a timed
+30 AC ward, used over benchmark armor. Both retain the same dual blades,
Tiger stance, 200 weapon/stance/style mastery, HP/mana training caps and
class-specific stat caps as the original experiment.

Demon mastery is8, all five gifts owned, souls at the legitimate cap, with
the legal maximum ten combat-oriented warps: chitin, scales, strongarms,
spiketail, quickness, stamina, regeneration, damcap, horns and shards.
This is one legal maximum build, not all mutually budgeted choices at once.
Preparation is Terminator/Demon Armour followed by two minute updates.
Demon's rotation is Netherflames, or Leech below75% HP; the ward is chosen
instead of a simultaneous free form/Baal buff. Real cooldowns allowed one
successful combat-power action per fighter in each of these100 fights.

| Opening attacker | Fights | Cyborg wins | Demon wins | Mean Cyborg HP left | Mean seconds |
|---|---:|---:|---:|---:|---:|
| Cyborg | 50 | 50 | 0 | 75,777.80 | 49.695 |
| Demon | 50 | 50 | 0 | 71,243.64 | 53.490 |
| Overall | 100 | 100 | 0 | 73,510.72 | 51.5925 |

No unresolved fights. Cyborg HP remaining ranged58,815..86,933;
durations36..77.25 seconds. Starting Cyborg/Demon values:
cap4100/2400, HR105/10, DR40/25, AC585/370, soak600/650, delay11/13.
Default seed1792094400: Cyborg opening wins in71.75s with59,477HP;
Demon opening still gives Cyborg victory in77.25s with58,815HP.

The class-armor configuration differs from the earlier matched-armor Wraith
experiment, so the results do not independently rank Demon against Wraith.
Nor do they certify all Demon rotations or optimally timed buff combinations.
No production gameplay source or balance rules changed.

### Cyborg versus Book Mage: method

`duelbookbatch` replays both previous100-fight matrices, then runs50 paired
seeds against a normal, non-Chaos `CLASS_SORCERER` Book Mage. Sorcerer uses
its native `LEV_SOR` command access (not staff access), all six colors5 and
the same HP/mana/stat/weapon/stance/style training policy as earlier tests.
Holding the book replaces its off-hand blade; active style is ONEHAND.
Cyborg retains the prior legal implant build, fabricated class armor and
dual blades. Book Mage retains benchmark armor under its crafted ward.

The book and pen are created with `sorccreate`;21 pages are actually written
with `write` and bound using `cast 'insert page'`. No page flags, damage
coefficients or binding links are injected directly. Crafting is pre-owned
fixture construction, before the timed preparation phase. Spell construction
mana is restored before preparation, as if arriving with a prepared book.
No resources or cooldowns are reset during preparation/combat.

Page1: green War Ward, duration50, armor/hitroll/damroll and Sanctuary.
At color5 its actual +50 AC/+50 HR/+50 DR and Sanctuary are asserted.
It costs3000mana; no maximum-HP/mana bonuses are added.
Pages2..11: ten linked red damage pages. Pages12..21: ten blue healing pages.
Both chains stop at their tenth page. Each script includes its required
messages, completion flags and binding-link assertions.

Preparation is Terminator/War Ward plus two real minute updates. Combat
rotation selects the red chain, or blue self-healing below75%HP. Native
15-pulse chant wait is respected. Each full chain costs6000/4000mana;
partially affordable chains are permitted, charged per funded page and
verified to stop at the engine's insufficient-mana boundary. Only mana below
the first page's cost causes the fixture to skip a chant and retain melee.
The caster does not receive infinite mana or force simultaneous color wards.

This is a deliberately specified combat-ready book, not starter Spark/Mend,
not the sphere-based Mage and not an all-strategy/class-balance certification.
The first diagnostic batch used a full-chain-only reserve guard; its results
are superseded by the final batch that permits native partial chains.

Final results (partial chains enabled):

| Opening attacker | Fights | Book Mage wins | Cyborg wins | Mean seconds |
|---|---:|---:|---:|---:|
| Cyborg | 50 | 49 | 1 | 66.95 |
| Book Mage | 50 | 50 | 0 | 58.51 |
| Overall | 100 | 99 | 1 | 62.73 |

No unresolved fights. Durations37.75..151.50seconds. Mean Book Mage HP
remaining across all fights, defeated clamped0:72,640.90; among its99 wins,
73,374.65. Starting Cyborg/Book Mage: cap4100/1600, HR105/50, DR40/50,
AC585/350, soak600/400, attack delay11/17, mana97800/97000.
Book Mage made11..21 successful combat chants per fight, charged per page;
Cyborg made1..2 successful class combat actions. All100 processes passed
ASan/UBSan, plus200 exact numerical/statistical Wraith/Demon regression
replays. The full-chain-only diagnostic batch is not the delivered dataset.

Default seed1792094400: Book Mage wins with75,065HP in52.75s when Cyborg
opens; with79,055HP in56.50s when Book Mage opens. Sole Cyborg victory:
seed1792260699, Cyborg opening,151.50s,1,780HP left; Book Mage ends
with200mana, below the first page's minimum400 healing/600 attacking cost.
These are recorded outcomes, not isolated causal tests of individual buffs.
No production gameplay source, spell coefficients, class balance, live host
or player data changed. This normal Book Mage fixture is not Chaos Mage.
