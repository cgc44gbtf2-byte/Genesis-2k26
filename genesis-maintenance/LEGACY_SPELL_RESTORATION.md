# Legacy item and NPC spell restoration

## Scope and compatibility

This maintenance batch conservatively reconstructs 20 formerly empty handlers:
17 used by supplied items or NPC special functions, plus three previously
unreferenced handlers finalized with explicitly agreed rules. These are replacement rules,
not recovered original implementations. It does not deploy or enable a public
server.

The supplied area's inventory contains 62 scroll/wand/staff/potion/pill spell
payload references across 47 object prototypes, four additional Dispel Evil
weapon procs, and Geoffrey's Giant Strength sword activation: 67 configured
references across 52 objects. A configured reference is not proof an object is
currently spawned. Seven NPC prototypes use `spec_cast_adept`, which can select
Armor, Bless and Refresh. Forty use `spec_cast_undead`, which can select Weaken
when its level condition permits.

All skill-table indices and area spell slots remain unchanged.
Level-99 spells remain unavailable to direct player casting.
**Know Alignment, Frenzy and Darkblessing**
now have effects, but no original-area item payloads or class unlocks are
added for them. **Ventriloquate** now provides bounded, explicitly attributed
object-based illusory speech and is available to Sorcerers; use
`cast ventriloquate <visible object here> <message>`.
The earlier eight reconstructed class abilities and subsequent combat/stance
repairs are retained. Named hybrid registry/previews are now included;
playable hybrid conversion and inheritance remain disabled.

## Reconstructed rules

| Spell | Effect |
| --- | --- |
| Armor | Timed armor +20. Positive armor improves player avoidance in Genesis. |
| Bless | Timed hitroll +2 and saving throw -2; both modifiers are removed together. |
| Change Sex | Temporary change to a different valid sex value (0–2), reversed on removal. |
| Create Water | Adds up to four times the clamped level in water units, within drink-container capacity. Refuses mixing with a nonempty non-water liquid and preserves poison. |
| Detect Evil | Marks evil NPCs in room displays; does not assign morality to players. |
| Detect Hidden | Allows ordinary hidden-character visibility without bypassing other visibility checks. |
| Detect Magic | Marks magical objects, including spell consumables, in object displays. |
| Detect Poison | Reports poison on supported targeted items. With a self-use potion/pill, checks the drinker's poison status and directly carried food/drink containers instead of rejecting a missing object target. |
| Dispel Evil | Damages only evil NPCs: clamped level to twice that level, halved on a successful save. Rechecks room and combat safety. |
| Giant Strength | Timed derived Strength +1, subject to existing stat limits; leaves stored base stats unchanged. |
| Pass Door | Timed passage through ordinary closed doors, not pass-proof barriers. Existing movement, privacy and dimensional checks remain in place. |
| Protection | Reduces damage from evil NPC attackers by 20%, rounded using integer arithmetic. No player-alignment assumptions. |
| Refresh | Restores 5–25 mana, capped at maximum mana. Movement points no longer exist in this engine. |
| Shield | Timed armor +20. This item/NPC spell is distinct from the class `shield`/`mindblock` command. |
| Stone Skin | Timed armor +30. |
| Weaken | Timed derived Strength -1, or -2 at level 50+, with existing stat floors, saves, player magic resistance, same-room and combat-safety checks. No self-target or repeated stacking. |
| Word of Recall | Recalls a player to their existing home only after travel checks; never creates or silently substitutes a destination. |
| Know Alignment | Reports a visible same-room NPC's prototype alignment, clamped to -1000…1000: evil at -350 or below, good at 350 or above, otherwise neutral. Refuses player targets. |
| Frenzy | A timed battle-rage effect granting hitroll +1 and damroll +1. Does not initiate attacks or alter Werewolf/Vampire rage state. |
| Darkblessing | Only evil NPCs qualify: timed hitroll +1, damroll +1 and armor +10. Refuses all player targets and non-evil NPCs. NPC avoidance is level-based and ignores this armor stat; no NPC avoidance bonus is claimed. |

For timed effects, duration is `clamp(2 + clamp(level, 1, 100) / 5, 2, 12)`
game ticks. Recasting the same effect does not stack or refresh it. Distinct
effects can combine. Effect removal reverses modifiers; Pass Door owns a
named affect rather than an equipment flag, so removal cannot erase an
independent item-provided Pass Door bit.

The former character-alignment system is absent. NPC prototype alignment
already present in area files is now retained, clamped to -1000…1000, and
preserved by area saving; an NPC is evil at -350 or below. Newly created NPC
prototypes start neutral. Players are never classified as evil.

Recall refuses combat or a fight timer, sleeping/stunned states, embraces,
ties, curse/web effects, shadow/deep-umbra/ethereal/spirit travel, protected
source/destination rooms, invalid homes and occupied private destinations.
An already-home target does not move.

## Activation and loading fixes

- The green potion's Detect Poison payload now uses the narrowly scoped
  self-screening behavior above. Other object-target spells still require
  an actual object; this is not a generic target-validation bypass.
- `eat` now activates the three spell fields on an `ITEM_PILL` and consumes
  it once. Existing quest-token behavior and ordinary-food policy are
  unchanged. This makes the supplied Change Sex herbs and other spell
  pills usable by players.
- The room loader now matches the area editor's existing 1–8 door-barrier
  encoding, including pick-, bash- and pass-proof combinations. Base door
  properties are retained in reset flags rather than disappearing on an
  area reset. Doors are not newly forced closed, and the engine's disabled
  legacy `D` reset commands are not re-enabled.
- Named character sex effects now apply and reverse their bounded modifier.
  Historically ignored equipment `APPLY_SEX` entries remain ignored.

## Verification

Native and AddressSanitizer/UndefinedBehaviorSanitizer builds use GNU C89,
`-Wall -Werror`, and the existing builtin-declaration compatibility exception.
The focused C probe links the real engine modules; it is not a reimplementation
of the spell calculations:

```sh
make -C src
sh tests/test-combat-system.sh legacy
python3 tests/legacy_spells.py
```

The C probe covers all 20 handlers: bounded/nonstacking modifiers, removal,
derived versus base stats, detection, NPC-only alignment, protection,
independent Pass Door flag ownership, resource/container caps, poison
dispatch, offensive saves/safety, invalid recall and safe Ventriloquate
rejection without an eligible object/message. The separate variants probe
checks actual attributed object speech and control-character rejection.
It checks Know Alignment output, boundary values,
clamping and visibility, plus Frenzy/Darkblessing eligibility, duration caps,
independent removal and dispelling.

The live suite uses disposable characters and temporary area copies. It
loads original mixed, sensing, refresh and travel potions, Geoffrey's custom
sword and Change Sex herbs; verifies live door traversal and blocked wards;
performs real logout/login and dispel removal; and checks protected-room,
fight-timer and plane restrictions. Its additional test rooms do not alter
the supplied area files. Disposable test wands additionally verify Know
Alignment and Darkblessing through the real item-spell dispatch path,
including NPC-only eligibility and duplicate rejection. Disposable Frenzy
potions verify modifiers, real logout/login persistence, dispelling and
natural expiry. Only the disposable saved effect durations are shortened
for the expiry check; no production timing hook is introduced.

Existing regressions remain available:

```sh
python3 tests/class_abilities.py
python3 tests/maintenance_fixes.py
sh tests/test-combat-system.sh
python3 tests/system_fixes.py
python3 tests/smoke_runtime.py
sh tests/test-resolve.sh
sh tests/test-random.sh
```

These cover class routing and the eight reconstructed gifts, prerequisites,
costs, safety, save/load/expiry, earlier nine-finding repairs, combat styles,
weather, Energy Drain, weapon procs, save failures and random-number ranges.
For the final 20-handler release, all five live suites passed on the native
build. The complete live legacy-spell suite also passed on the sanitizer
build, including the three new effects and Frenzy persistence/removal/expiry.
Both linked C probes passed with sanitizers; the native combat probe and the
sanitizer resolve/cost and random-range regressions passed too. The focused
legacy probe also checks a negative saved home before room lookup, avoiding
a negative hash-array index. The first new live run stopped on an incorrect
test expectation for equipping a wand ("hold" instead of the engine's
"clutch"); that assertion was corrected and both live spell runs repeated
successfully. No gameplay change was needed for that test failure.
Sanitizer probes disable leak checking; these results are not a leak audit.

Set `CRYPT_LIBRARY` to the installed system library path if it is not found
automatically. The live suites require Python 3 and an already-built
`src/merc`; their normal/sanitizer mode follows that binary.

## External upgrade precautions

Back up your source, mutable areas, players and backups before replacing
anything. Build on the external host; no compiled server is shipped in the
source archive. Retest representative items, NPC encounters and player saves
in a private copy before exposing the server publicly.

No supplied area definitions or live accounts are rewritten by this batch.
These rules require gameplay/balance review, particularly combined AC buffs,
NPC Protection and area barriers. The source still has legacy networking
and password handling; these fixes are not a complete production-security
audit. The permission/licensing warning in `README.md` still applies.
