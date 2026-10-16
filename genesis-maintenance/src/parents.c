/* New Genesis code: no upstream numeric class/power flags are imported. */
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "merc.h"
#include "parents.h"

/* Existing standalone support does NOT establish hybrid readiness.
 * No entry becomes ready until catalogue, effects and persistence are tested.
 */
static const GENESIS_PARENT parents[] = {
    {GEN_PARENT_VAMPIRE, "vampire", "Vampire", CLASS_VAMPIRE, 0, 0, 0},
    {GEN_PARENT_WEREWOLF, "werewolf", "Werewolf", CLASS_WEREWOLF, 0, 0, 0},
    {GEN_PARENT_SORCERER, "sorcerer", "Sorcerer", CLASS_SORCERER, 0, 0, 0},
    {GEN_PARENT_DROW, "drow", "Drow", CLASS_DROW, 0, 0, 0},
    {GEN_PARENT_HIGHLANDER, "highlander", "Highlander", CLASS_HIGHLANDER, 0, 0, 0},
    {GEN_PARENT_MAGE, "mage", "Mage", CLASS_MAGE, 0, 0, 0},
    {GEN_PARENT_DRAGON, "dragon", "Dragon", CLASS_DRAGON, 0, 0, 0},
    {GEN_PARENT_NINJA, "ninja", "Ninja", CLASS_NINJA, 0, 0, 0},
    {GEN_PARENT_MONK, "monk", "Monk", CLASS_MONK, 0, 0, 0},
    {GEN_PARENT_ELEMENTAL, "elemental", "Elemental", CLASS_ELEM, 0, 0, 0},
    {GEN_PARENT_ANGEL, "angel", "Angel", CLASS_ANGEL, 0, 0, 0},
    {GEN_PARENT_CYBORG, "cyborg", "Cyborg", CLASS_CYBORG, 0, 0, 0},
    {GEN_PARENT_DEMON, "demon", "Demon", CLASS_DEMON, 0, 0, 0},
    {GEN_PARENT_PHOENIX, "phoenix", "Phoenix", CLASS_PHOENIX, 1, 0, 0},
    /* Independently implemented and tested native standalone kits.
     * Never use ADG's composite Deathknight bits as a Genesis class value.
     */
    {GEN_PARENT_GARGOYLE, "gargoyle", "Gargoyle", CLASS_GARGOYLE, 1, 0, 0},
    {GEN_PARENT_KUEIJIN, "kuei-jin", "Kuei-jin", CLASS_KUEIJIN, 1, 0, 0},
    {GEN_PARENT_DEATHKNIGHT, "deathknight", "Deathknight", CLASS_DEATHKNIGHT, 1, 0, 0},
    {GEN_PARENT_MOOGLE, "moogle", "Moogle", CLASS_MOOGLE, 1, 0, 0},
    {GEN_PARENT_WRAITH, "wraith", "Wraith", CLASS_WRAITH, 1, 0, 0}
};

static const GENESIS_PARENT_POWER powers[] = {
#define P(parent, key, name, kind, sources) \
    {GEN_PARENT_##parent, #key, name, GEN_POWER_##kind, sources, 0},
#define I(parent, key, name, kind, sources) \
    {GEN_PARENT_##parent, #key, name, GEN_POWER_##kind, sources, 1},
#include "parent_powers.def"
#undef P
#undef I
};

static const GENESIS_HYBRID hybrids[] = {
#define H(id, first, second, name) \
    {GEN_HYBRID_ID_BASE+(id), GEN_PARENT_##first, GEN_PARENT_##second, name},
#include "hybrid_roster.def"
#undef H
};

/* ASCII-only exact matching: no ambiguous prefix grants or locale dependency. */
static int same_key(const char *a, const char *b)
{
    unsigned char ca, cb;
    if (a == NULL || b == NULL) return 0;
    while (*a != '\0' && *b != '\0') {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return *a == *b;
}

size_t genesis_parent_count(void)
{
    return sizeof(parents) / sizeof(parents[0]);
}

size_t genesis_hybrid_count(void)
{
    return sizeof(hybrids)/sizeof(hybrids[0]);
}

const GENESIS_HYBRID *genesis_hybrid_at(size_t index)
{
    return index<genesis_hybrid_count() ? &hybrids[index] : NULL;
}

const GENESIS_HYBRID *genesis_hybrid_find(int id)
{
    size_t i;
    for(i=0;i<genesis_hybrid_count();++i)
        if(hybrids[i].id==id) return &hybrids[i];
    return NULL;
}

const GENESIS_HYBRID *genesis_hybrid_named(const char *name)
{
    size_t i;
    for(i=0;i<genesis_hybrid_count();++i)
        if(same_key(hybrids[i].name,name)) return &hybrids[i];
    return NULL;
}

const GENESIS_HYBRID *genesis_hybrid_for_pair(int first,int second)
{
    size_t i;
    int low,high;
    if(!genesis_parent_pair_canonical(first,second,&low,&high)) return NULL;
    for(i=0;i<genesis_hybrid_count();++i)
        if(hybrids[i].first==low && hybrids[i].second==high) return &hybrids[i];
    return NULL;
}

int genesis_hybrid_registry_valid(void)
{
    size_t i,j;
    const GENESIS_HYBRID *h;
    if(genesis_hybrid_count()!=genesis_parent_pair_count()) return 0;
    for(i=0;i<genesis_hybrid_count();++i) {
        h=&hybrids[i];
        if(h->id<=GEN_HYBRID_ID_BASE || h->name==NULL
            || h->name[0]=='\0' || strlen(h->name)>64
            || h->first>=h->second
            || !genesis_parent_pair_valid(h->first,h->second)) return 0;
        for(j=i+1;j<genesis_hybrid_count();++j)
            if(h->id==hybrids[j].id || same_key(h->name,hybrids[j].name)
                || (h->first==hybrids[j].first && h->second==hybrids[j].second))
                return 0;
    }
    /* Distinct valid pairs plus n*(n-1)/2 count implies full coverage. */
    return 1;
}

const GENESIS_PARENT *genesis_parent_at(size_t index)
{
    return index < genesis_parent_count() ? &parents[index] : NULL;
}

const GENESIS_PARENT *genesis_parent_find(int id)
{
    size_t i;
    for (i = 0; i < genesis_parent_count(); ++i)
        if (parents[i].id == id) return &parents[i];
    return NULL;
}

const GENESIS_PARENT *genesis_parent_named(const char *key)
{
    size_t i;
    for (i = 0; i < genesis_parent_count(); ++i)
        if (same_key(parents[i].key, key)) return &parents[i];
    return NULL;
}

const GENESIS_PARENT *genesis_parent_for_class(long legacy_class)
{
    size_t i;
    if (legacy_class == CLASS_NONE) return NULL;
    for (i = 0; i < genesis_parent_count(); ++i)
        if (parents[i].legacy_class == legacy_class) return &parents[i];
    return NULL;
}

size_t genesis_parent_pair_count(void)
{
    size_t n = genesis_parent_count();
    return n * (n - 1) / 2;
}

int genesis_parent_pair_valid(int first, int second)
{
    return first != second && genesis_parent_find(first) != NULL
        && genesis_parent_find(second) != NULL;
}

int genesis_parent_pair_canonical(int first, int second, int *low, int *high)
{
    if (low == NULL || high == NULL || low == high
        || !genesis_parent_pair_valid(first, second)) return 0;
    *low = first < second ? first : second;
    *high = first < second ? second : first;
    return 1;
}

int genesis_parent_ready(int id)
{
    const GENESIS_PARENT *parent = genesis_parent_find(id);
    size_t i, count = genesis_parent_power_count(id);
    const GENESIS_PARENT_POWER *power;
    int gameplay = 0;
    if (parent == NULL || !parent->catalogue_complete || !parent->effects_gated
        || !parent->persistence_verified || count == 0) return 0;
    for (i = 0; i < count; ++i) {
        power = genesis_parent_power_at(id, i);
        if (!power->implemented) return 0;
        if (power->kind != GEN_POWER_SYSTEM) ++gameplay;
    }
    return gameplay != 0;
}

int genesis_parent_pair_ready(int first, int second)
{
    return genesis_parent_pair_valid(first, second)
        && genesis_parent_ready(first) && genesis_parent_ready(second);
}

size_t genesis_parent_power_count(int parent)
{
    size_t i, count = 0;
    for (i = 0; i < sizeof(powers) / sizeof(powers[0]); ++i)
        if (powers[i].parent == parent) ++count;
    return count;
}

const GENESIS_PARENT_POWER *genesis_parent_power_at(int parent, size_t index)
{
    size_t i;
    for (i = 0; i < sizeof(powers) / sizeof(powers[0]); ++i) {
        if (powers[i].parent != parent) continue;
        if (index-- == 0) return &powers[i];
    }
    return NULL;
}

const GENESIS_PARENT_POWER *genesis_parent_power_named(int parent, const char *key)
{
    size_t i;
    for (i = 0; i < sizeof(powers) / sizeof(powers[0]); ++i)
        if (powers[i].parent == parent && same_key(powers[i].key, key))
            return &powers[i];
    return NULL;
}

int genesis_parent_registry_valid(void)
{
    size_t i, j;
    for (i = 0; i < genesis_parent_count(); ++i) {
        if (parents[i].id <= 0 || parents[i].key[0] == '\0'
            || parents[i].name[0] == '\0') return 0;
        for (j = i + 1; j < genesis_parent_count(); ++j)
            if (parents[i].id == parents[j].id
                || same_key(parents[i].key, parents[j].key)
                || (parents[i].legacy_class != 0
                    && parents[i].legacy_class == parents[j].legacy_class))
                return 0;
    }
    for (i = 0; i < sizeof(powers) / sizeof(powers[0]); ++i) {
        if (genesis_parent_find(powers[i].parent) == NULL
            || powers[i].key[0] == '\0' || powers[i].name[0] == '\0'
            || powers[i].kind < GEN_POWER_ACTIVE
            || powers[i].kind > GEN_POWER_SYSTEM) return 0;
        for (j = i + 1; j < sizeof(powers) / sizeof(powers[0]); ++j)
            if (powers[i].parent == powers[j].parent
                && same_key(powers[i].key, powers[j].key)) return 0;
    }
    return genesis_hybrid_registry_valid();
}
