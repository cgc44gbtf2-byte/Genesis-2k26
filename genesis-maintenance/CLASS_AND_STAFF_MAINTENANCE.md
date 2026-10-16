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
