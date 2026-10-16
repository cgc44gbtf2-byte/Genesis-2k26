# Class ability restoration

## Restored or repaired

- One `ward` command dispatches to the existing Werewolf or Mage implementation.
  Disabling `ward` through the existing administrator facility disables both.
- Exact command names now take precedence over abbreviations, subject to trust.
  Vampire `step` no longer resolves to Mage `stepsideways`. Existing abbreviated
  commands still use their original first-accessible-match ordering.
- Werewolf Reach, Wrath, Mastery and Invisibility, Dragon Wings, and Vampire
  Shadowplay are connected to the command table.
- Wrath checks the target's Beast, not the caster's, and respects groups and
  existing combat safety checks.
- Reach keeps visibility requirements and checks PK/safe-room protection for
  unwilling players. Players permitting summoning may be pulled. Immortals
  cannot be pulled. Its incorrect Theurge requirement message is corrected.
- Mastery retains Silver Fangs 5 and lower-rank Garou requirements. It respects
  combat protection and authority. Its restored command execution is deliberately
  limited to movement, speech, posture, looking and fleeing. It resolves aliases
  before checking allowed handlers, so abbreviated destructive commands cannot
  bypass that restriction. This is not unrestricted control of another account.
- Stun Fist's disable switch is removed. The existing Tech 7 requirement, 5000
  mana cost, and 24-pulse waits for both characters are retained. A successful
  strike now enters combat through the existing safety-aware combat helper.
  It imposes the original command delay, not a newly invented permanent paralysis.
- Matter 1 Analyze identifies carried objects using the existing Identify spell,
  following the established Diagnostics implementation.
- Shadowplay's sunlight/inside check is corrected before making it reachable.
- Spiritfriend, Illusionaryform and Changeling resolve to their existing
  Spiritguardian, Formillusion and Malleable implementations, respectively.
- Invalid historical Mage spell names are retained as compatibility aliases;
  the help text now uses the canonical command names.
- Black Furies and Bone Gnawers power lists use their own tribe levels instead
  of unrelated auspice levels.

## Reconstructed missing mechanics

Disquiet, Quicksand, Lycaon, Totemgift, Scream, Horridreality, Maskempathy and
passive Strength of Purpose now have replacement implementations, following
approval to reconstruct them with conservative rules. They are not recovered
original mechanics. See `ABILITY_RECONSTRUCTION.md` or in-game
`help reconstructedabilities` for the complete rules and safety limits.

Historical help also contains names replaced by different abilities in the
current class menus. This work does not restore every historical class design
or claim an exhaustive balance audit.

## Verification

`tests/class_abilities.py` runs an isolated server with disposable fixtures. It
checks both Ward dispatch paths, Step exact-name resolution, ordinary command
abbreviations, Analyze identification, compatibility aliases, corrected power
lists, Wings toggling, Invisibility and Shadowplay flags, Reach and Wrath effects,
PK/safe-room rejection, Mastery action restrictions and successful execution,
and Stun Fist prerequisites, safe-room rejection and mana use.

Run alongside `tests/smoke_runtime.py` and `tests/test-random.sh`. Sanitizer runs
are recommended. These focused regressions do not establish production security
or verify every class ability or encounter.

The class test now also invokes `tests/reconstructed_abilities.py` for the new
effects, including saved curses, removal and natural expiry. It can take several
minutes because it respects combat logout timers and waits for a real update
tick. `sh tests/test-resolve.sh` deterministically verifies the Strength of
Purpose saving-throw bonus and unchanged maximum.
