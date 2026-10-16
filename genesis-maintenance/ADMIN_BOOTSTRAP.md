# Owner bootstrap: Admin

`relevel` is implemented in `src/kav_wiz.c` in this Genesis version,
not `act_wiz.c`. It now authorizes only the exact player name **Admin**,
case-insensitively. Akuma, Chaos, Dude, Shane and Corruption no longer
receive any special promotion from this command. Prefixes and multi-name
strings do not qualify.

As the logged-in Admin player, type `relevel`. It sets the existing highest
rank `LEV_ST = MAX_LEVEL = 7`, which is also the effective command trust
level in this codebase, and retains the previous top-rank 500,000 maximum/
current HP and mana. It is self-only, not a target-promotion command.
NPCs, switched descriptors and malformed/non-player actors are refused.
Command logging remains `LOG_ALWAYS`.

**Security requirement:** the name itself is the authorization mechanism.
Create and password-protect the Admin character privately before allowing
public registrations. This change does not reserve the name at registration
or provision a password/account for you. Do not expose a new server with an
unclaimed Admin name. Never put its password into source or help files.

This source change does not demote already-promoted accounts, modify live
player files, create an Admin account or deploy the server. Existing staff
ranks are separate persisted state. Save your character normally after use.

Run `sh tests/test-combat-system.sh relevel` for the linked sanitizer probe.
