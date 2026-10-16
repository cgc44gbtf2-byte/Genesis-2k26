# Book Mage and Chaos Book Mage

These are Sorcerer playstyles, distinct from the sphere-based Mage class.
Chaos is not an additional hybrid-parent identity.

## Start playing

- Normal: `autoclass sorcerer`
- Chaos: `autoclass chaosmage` or `autoclass sorcerer chaos`
- Existing Sorcerers: `sorccreate starter` claims the kit once.

Normal class-entry stat requirements still apply. Each character receives
one free kit; the saved claim survives class resets. A missing object template
or an attempt while fighting/in object form does not consume the claim.

The kit contains an open, five-page primer and an inscription pen. It does
not spend primal. Normal beginners receive at least rank 1 blue, green and
yellow magic; Chaos beginners receive at least rank 1 in all six colors.
Existing higher mastery is not reduced.

| Page | Content | Normal mana cost |
|---|---|---:|
| 1 | Getting started and navigation | — |
| 2 | Spark: targeted damage | 200 |
| 3 | Mend: targeted healing | 400 |
| 4 | Minor Ward: protective armor blessing, two ticks | 1560 |
| 5 | A complete spell-writing recipe and binding instructions | — |

Use `hold primer`, then `read primer`. Use `turn primer 0` to read the index.
For example:

```
turn primer 3
chant self
turn primer 4
chant self
turn primer 2
chant <enemy>
```

Higher armor improves Genesis's avoidance calculation. Minor Ward raises
armor; do not use `reversed` for a protective armor page. Same-color sorcery does not
stack with itself. The ward's native 1560-mana cost may require increasing
your mana pool before you can use it.

## Write a new Spark

The starter pen is already provided. Enter each command separately:

```
remove primer
sorccreate page
hold pen
write page title My Spark
write page line start.damage.spell
write page line message.1: sparks
write page line message.2: You send sparks toward $N.
write page line message.3: $n sends sparks toward $N.
write page line end.yellow
remove pen
cast 'insert page' page primer
hold primer
turn primer 6
chant <enemy>
```

On a new starter book, binding this page appends it as page 6. If you have
already added pages, use the new page number from the index instead.
Ordinary `sorccreate book` and `sorccreate pen` still cost 10 primal each;
creating a blank page is free. `colors` displays mastery; `colors <color>`
buys the next rank using the existing progression.
Book mages also have the harmless utility command
`cast ventriloquate pen <message>`: it creates explicitly illusory object
speech, not a forged player/staff speaker.

## Chaos rules

A Chaos-marked book rolls one of purple, red, blue, green, yellow or black
each time a page is invoked, including chained pages. The caster sees the
result. A new roll may repeat the previous color; this is not a rotation.
The same blessing can therefore be red on one chant and blue on the next.

The page's purpose is retained. A healing page does not unexpectedly attack
its recipient; a Spark remains a targeted attack; an armor blessing still
modifies armor. The rolled color supplies native color mastery, sorcery
effect identity and the native damage/healing mana cost. Damage/healing costs
can vary between 200 and 800 mana. Ward cost remains 1560.

Action pages keep their written summon/travel operation, rather than changing
destinations or targets because of a color roll. Their color roll is announced.
New blank books created by a Chaos Sorcerer are also Chaos-marked. The marker,
pages, color training, playstyle and starter claim persist through saving.

This is different from an ordinary page written with `end.random`: that legacy
script deliberately uses the original color-dependent behavior, which can
change an attack into healing or a single-target effect into an area effect.
Use a fixed written color to define the desired role in a Chaos book.

## Verification

`sh tests/test-combat-system.sh books` exercises the real handlers and effect
hooks under AddressSanitizer and UndefinedBehaviorSanitizer. It checks the
starter inventory, exact normal costs, all three spell effects, actual writing
and binding of the tutorial recipe, same-color nonstacking, preserved mastery,
one-time claims and player-field parsing. Repeated Chaos chants check all six
ward effect identities and that healing/attack roles remain unchanged.

`GENESIS_BOOKS_ONLY=1 python3 tests/class_abilities.py` runs disposable native
server accounts through normal/Chaos selection, existing-player claiming,
reading, chanting, saving and relogging with nested book pages.
