# Skills, spells and fighting stances: focused repair

This is a second maintenance batch after `MAINTENANCE_FIXES.md`. It makes no
deployment changes. Back up player and area files before applying the source
to an external server.

## Changes

| Area | Repair |
| --- | --- |
| Skill-index boundary | `sn_lookup(MAX_SKILL)` and unnamed padding slots now return zero instead of reading past the table. Existing named indices and object spell slots were not renumbered. |
| Weapon spell procs | A weapon only invokes a spell when its encoded skill index is in range, has a handler and takes an offensive character target. An invalid proc is skipped; the normal weapon hit remains. All 33 proc references in the supplied area objects have compatible targets. |
| Saved combat state | On player load, invalid current styles fall back to unarmed, invalid selected/automatic stances fall back to none, and stored style/stance ranks are bounded from 0 to 200. The `skill` command also checks indices before displaying them. Valid saved training is preserved. |
| Defensive stances | Crab, Dragon and Swallow now apply their damage-reduction multiplier even when the attacker uses no stance. Attackers' damage-boosting stance behavior is unchanged. |
| Spell positions | The general `cast` command now enforces each spell's existing `minimum_position` field before charging mana, so a standing-only spell cannot be cast while fighting. Object-activated effects retain their separate invocation rules. |
| Control Weather | `cast 'control weather' better` or `worse` adjusts the global pressure-trend change by 1–3 according to spell level, bounded to -12…12. The normal weather update changes the visible sky later; the spell does not force instant rain or sunshine. Invalid direction is rejected before charging the original 25 mana. |
| Energy Drain | On an eligible same-room target, deals 1–2 times bounded spell level (1–100) in spell damage. It rechecks combat protection before moving resources, including when triggered by a weapon. A successful save halves damage. Otherwise, it removes up to half that level from the target's nonnegative mana and restores at most the caster's missing mana; the transfer neither overdraws a valid balance nor pushes the caster over their normal maximum. It does not steal XP, levels or maximum stats. Self-targeting is refused before paying the original 35 mana. |

The bound on Energy Drain spell level also applies when it fires from an item.
Eight weapon prototypes in the supplied areas invoke Energy Drain, so these
weapons now gain its bounded effect rather than silently doing nothing. This
may change item balance; test those weapons before opening a public server.

## Verification and remaining scope

`tests/test_combat_system.c`, run through `tests/test-combat-system.sh`,
links the real game modules into an AddressSanitizer/UndefinedBehaviorSanitizer
probe. It checks all ten stance selections, advanced prerequisites, autostance,
the defensive damage term, indices, casting restrictions, weather changes and
pre-charge validation, including Energy Drain's target-safety recheck.
`tests/system_fixes.py` exercises player load and
save, both weather directions, live Energy Drain, and weapons with both
valid Energy Drain and invalid stored procs against a disposable local server. Both tests keep
runtime data outside the source tree.

```sh
sh tests/test-combat-system.sh
python3 tests/system_fixes.py
```

The C script creates an isolated instrumented build; the Python test needs
`src/merc` built already. On systems where Python or the compiler cannot
locate libcrypt, point `CRYPT_LIBRARY` at the installed library.

**Historical scope of this batch:** 21 other level-99 legacy spell handlers remained
empty. Although players cannot cast them directly at normal levels, a scan
of the 1,255 object prototypes in `area/area.lst` found 62 spell references
across 47 objects to those handlers (16 distinct spell names). The previous
scan found 65 references before restoring Energy Drain. Removing those item
effects or defining replacements is separate work; they were not silently
re-enabled or given invented mechanics here. This is a focused repair, not a
complete audit of every spell, skill or item in the game.

The 16 item-referenced names are armor, bless, change sex, create water,
detect evil, detect hidden, detect magic, detect poison, dispel evil, giant
strength, pass door, protection, refresh, shield, stone skin and word of
recall. Five further empty level-99 handlers had no references in the supplied
area objects: know alignment, ventriloquate, weaken, frenzy and darkblessing.

**Subsequent maintenance:** `LEGACY_SPELL_RESTORATION.md` supersedes the
deferred-work status above. It reconstructs the 16 item-referenced spells and
the NPC-used Weaken handler, leaving four unreferenced handlers disabled.
The later audit also identified four Dispel Evil weapon procs and Geoffrey's
Giant Strength sword activation, beyond the 62 consumable/wand/staff payload
references counted in this historical inventory.
