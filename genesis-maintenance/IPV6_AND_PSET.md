# IPv6 hosting and staff currency maintenance

## Network listener

The server first opens one IPv6 wildcard listener (`::`) with
`IPV6_V6ONLY=0`. On Linux this accepts IPv6 and IPv4 on the same configured
port. The existing select loop and copyover keep their single listening
descriptor. `PORT=4000 ./run.sh` needs no IP address in the source.

If the IPv6 stack or dual-stack option is unavailable, the server falls back
to an IPv4 wildcard listener and logs that fact. Occupied ports, denied
permissions and resource exhaustion are errors, not reasons to silently
switch families. The listener initializes its socket options and closes
failed sockets; accepted sockets are also closed on setup failures.

Accepted peers use `sockaddr_storage`, not a fixed IPv4 structure.
IPv4-mapped addresses normalize to dotted IPv4. Bans check the numeric
address as well as the reverse hostname, retaining legacy prefix/suffix
matching. Reverse DNS remains synchronous, as in the legacy server; this
change does not claim asynchronous DNS, TLS or a new client protocol.

The supplied replacement address is `2604:a880:400:d1:0:5:331d:4001`, port
`4000`. From an IPv6-capable client:

```sh
telnet -6 2604:a880:400:d1:0:5:331d:4001 4000
```

In a MUD client, enter the IPv6 address as the host and `4000` separately
as the port. A client or access network without IPv6 cannot reach an
IPv6-only public address; it needs IPv6 connectivity or an additional
server IPv4 address/DNS name.

On the host, verify the new binary is running and listening:

```sh
PORT=4000 ./run.sh
# In a second session:
ss -ltnp 'sport = :4000'
```

The Linux host, UFW IPv6 configuration and any provider firewall must allow
inbound TCP 4000 over IPv6. Remove an earlier matching DENY before relying
on an ALLOW rule; keep SSH permitted. This patch does not change the host
firewall, install a service or deploy to the supplied endpoint.

## `pset` balances

`pset` retains its immortal command-table requirement and also checks it
inside the handler. It rejects NPC actors, switched sessions, NPC targets,
and edits to another player of equal or higher trust. Self-edits by an
authorized immortal remain possible. Highest-level `Admin` can edit mortal
balances; this is not a player progression shortcut.

```text
pset Player currencies
pset Player classpoints 50000
pset Player exp 5000000
pset Player primal 500
pset Player bionics 100000
```

Values are **absolute balances**, not additions. `currencies` (or `wallet`)
is read-only and shows the applicable balances and allowed staff-edit
ranges. Invalid numbers, signs, fractions, overflow, trailing arguments
and unavailable class resources do not change the character.

| Field | Common aliases | Staff-edit range / condition |
|---|---|---|
| `exp` | `xp`, `experience` | 0–1,000,000,000 |
| `primal` | `primalpoints` | 0–1,000,000,000 |
| `gold` | `coins`, `money` | 0–1,000,000,000 |
| `classpoints` | `cps`, `cp`, `dp`, `favor`, `favour`, `classcostpoints`, `class_cost_points` | 0–10,000,000; current spendable pool |
| `classpointstotal` | `cptotal`, `cp_total` | 0–10,000,000; lifetime field, edited separately |
| `questpoints` | `quest`, `qp`, `quest_points` | 0–1,000,000 |
| `mana` | — | 0–current maximum mana, at most 1 billion |
| `hit` | `hp`, `health` | 1–current maximum HP, at most 1 billion |
| `blood` | — | 0–current blood-pool capacity |
| `willpower` | `will` | 0–current willpower capacity |
| `ki` | — | 0–300; Ninja |
| `quintessence` | `quint` | 0–10,000; Mage's usable BODY pool |
| `nanites` | — | 0–10,000; Cyborg with installed battery; otherwise only 0 |
| `bionics` | `bionic` | 0–10,000,000; Cyborg |
| `souls` | `soulpower` | 0–1,000,000; Demon |
| `energy` | class-specific aliases below | 0–10,000; matching crafted/lineage parent |
| `humanity` | `freehumanity` | Free Cyborg implant budget; total budget may not exceed 1,000 |
| `humanitycap` | `humanitycapacity` | Total Cyborg implant budget; installed spending–1,000 |
| `bombs` | — | 0–20; Cyborg; disarm before setting an armed inventory to zero |

Class-specific energy aliases are `pomenergy`/`pom` (Moogle), `soulenergy`
(Deathknight), `stoneenergy` (Gargoyle), `chi` (Kuei-jin) and `pathos`
(Wraith). Each rejects the wrong parent class. Corrupt resource state is
rejected rather than silently repairing it or unlocking an ability.

These are staff-editor limits, not a rebalance of training prices or every
natural regeneration limit. No `atm`/bank field is exposed: the legacy
`atm` field is a Mage avoidance parameter, not money. Ability ranks,
mastery, stat caps and quest progress are not currencies.

Current and total class points remain independent: setting `cps` does not
invent lifetime earnings. Setting `humanity` changes total capacity too,
preserving all humanity already spent on installed components. Setting
`humanitycap` preserves that spending and recalculates the free budget.
Installed/permitted masks and learned abilities are never granted or erased.

Positive nanites require a battery because existing passive restoration
clears nanites without one. Staff must not be told an unusable grant will
persist. Training requirements and component installation still apply.

Edits log actor, target, resource and old/new values. They follow the
existing save/autosave rules; `pset` does **not** immediately force a disk
save. Have the player use `save`, or use the existing authorized staff-save
workflow before a planned restart. Never overwrite live player data with
test fixtures or a fresh source checkout.

## Verification

```sh
sh tests/test-combat-system.sh currencies
sh tests/test-combat-system.sh network
sh tests/test-combat-system.sh all
python3 tests/staff_currency_runtime.py
python3 tests/smoke_runtime.py
python3 tests/audit_ability_help.py --strict
```

The linked tests use strict GNU89, `-Wall -Werror`, ASan and UBSan. Currency
tests cover permission/class gates, immutable failed edits, aliases, limits,
read-only queries, strict parsing, battery requirements and preservation
of implant spending. The disposable native server checks real staff
commands and save/relogin persistence across nine class resource families.

Network tests use real sockets for every locally available family,
including greetings, numeric bans and IPv4-mapped peer normalization.
Controlled syscall tests exercise dual-stack binding/options, unavailable
stack fallback, occupied/denied-port rejection and IPv6 peer formatting.
This maintenance environment has an IPv4-only kernel (`EAFNOSUPPORT`);
an actual IPv6 TCP session cannot be certified here. Run the same network
probe on the IPv6-enabled host. Public reachability, provider firewall
settings and production copyover have not been verified or changed.
