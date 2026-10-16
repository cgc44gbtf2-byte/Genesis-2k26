# Cyborg — isolated backend development

## Current status

The complete current guide is **CYBORG_KIT.md**: all 60 catalogue entries now
have tested Genesis implementations. The former pending entries have been
promoted after effect-level verification. The sections below are historical
development notes, not the current completion/readiness status.
Standalone Cyborg is playable; hybrid conversion remains disabled.

## Latest status — initial playable adapter

The later NEW_CLASSES_IMPLEMENTATION.md supersedes backend-only status below.
Cyborg now has real autoclass entry, training, 24 component adapters, bounded
nanite storage/factory/repair, taser and same-room railgun attacks, vision,
camouflage and radio. Angel and one Demon also have initial playable kits.
Native and linked sanitizer checks passed. Cyborg remains a hybrid parent,
but full hybrid conversion and all remaining reference abilities are pending.
The following sections retain the earlier backend research and verification
history; statements that gameplay was disabled describe those earlier batches.

Cyborg is intended as a standalone parent class AND a selectable hybrid parent.
This supersedes the earlier hybrid-only requirement. Standalone Cyborg gets
its full kit; hybrids retain the player-chosen 75%-per-parent rule.
This batch does NOT implement a playable Cyborg, combat effects, player
conversion, or standalone autoclass selection. Do not enable an empty class.

## Implemented

`src/cyborg_core.{h,c}` and `cyborg_components.def` provide:

- 24 independently identified components: 15 implants and nine replacement
  slots, splitting left/right arms and legs explicitly.
- Original Vortex executable prerequisite branches and primal/humanity costs.
  Credit: Kale/Jalerit. The Vortex module explicitly permits reuse with credit.
- Dependency-closed permissions, separate from installed components.
  Installation never implicitly grants an unpermitted prerequisite.
- Atomic installation: missing prerequisites, duplicate installation, invalid
  state, insufficient primal or insufficient humanity debit nothing.
- Dependency-safe uninstall, exact humanity restoration, no primal refund
  and no repeated refund for absent components.
- A bounded, versioned text codec that rejects unknown versions, numeric
  overflow, unknown flags, orphan dependencies and inconsistent accounting.
  Failed decoding leaves the destination unchanged.

The component backend uses neither `CLASS_*` values nor `pcdata->powers[]`.
The explicit humanity budget is supplied by a future authorized adapter:
there is no newly invented starting-humanity rule or fixed 100-point limit.
Component permissions are not the final individual-ability selection mask.
Do not calculate the 75% quota from this 24-component table.

The APIs return `CY_OK` (zero) on success, otherwise a named status.
The version-1 codec is now wired to an independent PC_DATA member and an
optional CyborgCore player record. Existing files default to empty state and
empty state writes no extra record. Oversized, binary, unsupported or invalid
records reset only Cyborg state and consume their complete line. Class clearing
also resets the state. Combat/visibility/resource effects remain unconnected.
A future adapter must verify standalone Cyborg or hybrid identity and authoritative
selected powers; a syntactically valid codec record is not authorization.

## Parent inventory

`hybrid cyborg` now previews 60 pending inventory entries across Vortex and
Mindcloud, including management/training entries. This is NOT 60 implemented
powers and NOT a finalized quota denominator. All Cyborg effects remain
marked unported and the parent remains not ready for conversion.

Vortex's factory/radio/chameleon/taser effects share their corresponding
implant identity; command aliases are not extra abilities. Mindcloud aliases
oilspit/stuntubes, terminator/cubeform and upload/shadowwalk are counted once.
Some Mindcloud commands call shared handlers whose internal guards still
require verification. Advertised effects are not assumed to work.

## References

The Vortex:
https://github.com/mudhistoricalsociety/The_Vortex
Inspected snapshot: 6db439378f566f59d7ce89f7b01264d76896d1e7.
The root cyborg.c is not listed in the supplied src/Makefile; inspected headers
lack its original implant/resource definitions. Its executable Railgun
installation branch requires both arms and teleoptics, not the targeting
computer claimed by the introductory comment. The backend follows the branch.

Mindcloud:
https://github.com/smthbh/Mindcloud-2.5-MUD
Inspected snapshot: 084e5d8af642755a775b64d7f1f03d9e7bcffa1b.
Its face/limb/body model is different from Vortex's, with caps 5/5/6.
Its supplied build includes cyborg.c. Command bindings, combat modifiers,
darkness handling and regeneration consumers were inspected. Source bodies
have NOT been copied. The module credits Zaphonite, Vladd and Jobo, and
preserves Mindcloud, Diku and Merc notices. Its bundled Dystopia/GodWars
licenses impose additional conditions, including distribution-permission
wording; do not treat public GitHub availability as proof of permission.
Confirm applicable permissions and preserve required credits/help/license
files before copying/publishing a derived implementation.

Gathering:
https://github.com/cgc44gbtf2-byte/gathering
Inspected snapshot: 289aa46d35f358d0b1ae024e3fecf005a2461176.
The full recursive tree contains README.md only, with no source to import.
Keep this as a pending reference, not a source of invented abilities.

## Remaining effect work

Vortex Repair lacks a maximum-health cap. Taser prints computed damage without
applying that amount through damage(). Snipe moves the shooter along its
path without restoring the starting room and subtracts health directly.
Explosives write object value[4], outside Genesis's four-slot object array.
Resource generation, projectile/owner lifecycle, cloak/sensor consumers,
training and passive implant effects need complete, individually selected
capability gates and runtime tests.

Mindcloud's three successive Stun Tubes hits access the target again after
each attack, and then set raw poison/flaming flags without a named expiry.
Cubeform directly adds/removes modifiers and shares a generic form bit.
Its movement-cost requirements cannot be copied: Genesis has no movement
resource. Its combat/update hooks broadly grant class-wide effects, which
must not grant unselected powers to hybrids.

Player save/load, restoration/cleanup, hybrid quotas/selection, balance and
all live combat/visibility effects remain unfinished.

## Verification

Run the isolated backend sanitizer tests:

    sh tests/test-cyborg-core.sh

All 24 component installation/removal paths, prerequisites, exact costs,
malformed state, resource boundaries, parser bounds and codec round trips
passed under ASan/UBSan with strict compiler warnings.

The full server builds with -Wall -Werror. Runtime smoke, system fixes,
legacy spells and maintenance-fix native suites passed. The class suite
initially expected the old empty Cyborg inventory; that fixture has been
updated to require both reference kits and an unported status. Its final
live retry and the full linked parent ASan/UBSan probe passed, including the
60-entry Cyborg inventory, alias deduplication, prior scope label, blocked
readiness and read-only actor/player-data checks. All five native suites
therefore passed across the runs.

Test setup recovery: the first live attempt lacked the server binary after
the preceding archive cleanup; it was rebuilt. A direct class-suite retry
needed the workspace's absolute CRYPT_LIBRARY path. These were test execution
issues, not verified server defects. The archive must be source-only and
validated after cleaning; it is not a full-class or full-hybrid release.

## Player-state integration batch

The latest code wires the state to real player save/load and resets it on
class clearing. Standalone/library record tests passed, including CRLF,
binary and oversized records and preserving the next player field.
Full linked parser checks passed under ASan/UBSan. They verify real fread_char
round trips, fail-closed malformed-record handling, following-field alignment
and preservation of legacy class/power slots. The deliberate malformed fixture
emits the expected reset warning, not an unexplained runtime error.

Strict native compilation and all five live suites passed. The live class
suite proves real save/logout/login persistence of independent component
state without converting its actor's legacy class, and proves empty old
characters write no extra record. These checks do not verify playable Cyborg
effects, Cyborg training or hybrid conversion: those remain unimplemented.
