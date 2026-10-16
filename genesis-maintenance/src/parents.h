/*
 * Parent identities and catalogue metadata for hybrid development.
 * Parent IDs are NOT CLASS_* bit values. Never assign them to ch->class.
 * Append identities; do not renumber persisted identifiers.
 */
#ifndef GENESIS_PARENTS_H
#define GENESIS_PARENTS_H

#include <stddef.h>

#define GEN_PARENT_VAMPIRE    1001
#define GEN_PARENT_WEREWOLF   1002
#define GEN_PARENT_SORCERER   1003
#define GEN_PARENT_DROW       1004
#define GEN_PARENT_HIGHLANDER 1005
#define GEN_PARENT_MAGE       1006
#define GEN_PARENT_DRAGON     1007
#define GEN_PARENT_NINJA      1008
#define GEN_PARENT_MONK       1009
#define GEN_PARENT_ELEMENTAL  1010
#define GEN_PARENT_ANGEL      1011
#define GEN_PARENT_CYBORG     1012
#define GEN_PARENT_DEMON      1013
#define GEN_PARENT_PHOENIX    1014
#define GEN_PARENT_GARGOYLE   1015
#define GEN_PARENT_KUEIJIN    1016
#define GEN_PARENT_DEATHKNIGHT 1017
#define GEN_PARENT_MOOGLE     1018
#define GEN_PARENT_WRAITH     1019

#define GEN_POWER_ACTIVE  1
#define GEN_POWER_PASSIVE 2
#define GEN_POWER_SYSTEM  3

typedef struct genesis_parent {
    int id;
    const char *key;
    const char *name;
    long legacy_class;
    int catalogue_complete;
    int effects_gated;
    int persistence_verified;
} GENESIS_PARENT;

typedef struct genesis_parent_power {
    int parent;
    /* Stable, parent-scoped key; neither command aliases nor legacy flags. */
    const char *key;
    const char *name;
    int kind;
    const char *sources;
    int implemented;
} GENESIS_PARENT_POWER;

/* Registry namespace only. Never assign these IDs to ch->class.
 * Both parents use stable GEN_PARENT_* IDs, not CLASS_* bit values.
 * This metadata does not make a hybrid playable or migrate a player.
 */
#define GEN_HYBRID_ID_BASE 200000
typedef struct genesis_hybrid {
    int id;
    int first;
    int second;
    const char *name;
} GENESIS_HYBRID;

size_t genesis_hybrid_count(void);
const GENESIS_HYBRID *genesis_hybrid_at(size_t index);
const GENESIS_HYBRID *genesis_hybrid_find(int id);
const GENESIS_HYBRID *genesis_hybrid_named(const char *name);
const GENESIS_HYBRID *genesis_hybrid_for_pair(int first,int second);
int genesis_hybrid_registry_valid(void);

size_t genesis_parent_count(void);
const GENESIS_PARENT *genesis_parent_at(size_t index);
const GENESIS_PARENT *genesis_parent_find(int id);
const GENESIS_PARENT *genesis_parent_named(const char *key);
const GENESIS_PARENT *genesis_parent_for_class(long legacy_class);
size_t genesis_parent_pair_count(void);
int genesis_parent_pair_valid(int first, int second);
int genesis_parent_ready(int id);
int genesis_parent_pair_ready(int first, int second);
int genesis_parent_pair_canonical(int first, int second, int *low, int *high);
size_t genesis_parent_power_count(int parent);
const GENESIS_PARENT_POWER *genesis_parent_power_at(int parent, size_t index);
const GENESIS_PARENT_POWER *genesis_parent_power_named(int parent, const char *key);
int genesis_parent_registry_valid(void);

#endif
