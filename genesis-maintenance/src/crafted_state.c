/* Independent, bounded authority for the original Genesis kits. */
#include <stdio.h>
#include <string.h>
#include "crafted_state.h"
#include "reference_state.h"

int crafted_state_valid(const CRAFTED_STATE *s)
{
    if (!s || s->rank < 0 || s->rank > 8 || s->energy < 0 || s->energy > CRAFTED_ENERGY_CAP) return 0;
    if (!s->parent) return !s->rank && !s->energy;
    return s->parent == GEN_PARENT_DEATHKNIGHT || s->parent == GEN_PARENT_MOOGLE;
}

int crafted_state_write(const CRAFTED_STATE *s, FILE *fp)
{
    return fp && crafted_state_valid(s) && fprintf(fp, "CraftedClass 1 %ld %ld %ld\n",
        s->parent, s->rank, s->energy) >= 0;
}

int crafted_state_read(CRAFTED_STATE *s, FILE *fp)
{
    char line[128], *p;
    long v[4];
    int i;
    CRAFTED_STATE next;
    if (!s || !fp) return 0;
    if (!reference_line(fp, line, sizeof(line))) goto bad;
    p = line;
    for (i = 0; i < 4; ++i) {
        v[i] = 0;
        if (*p < '0' || *p > '9') goto bad;
        while (*p >= '0' && *p <= '9') {
            /* Reject before multiplying, including arbitrarily long input. */
            if (v[i] > CRAFTED_ENERGY_CAP / 10) goto bad;
            v[i] = v[i]*10 + (*p++ - '0');
            if (v[i] > CRAFTED_ENERGY_CAP) goto bad;
        }
        if (i < 3 && *p++ != ' ') goto bad;
    }
    if (*p || v[0] != 1) goto bad;
    next.parent = v[1]; next.rank = v[2]; next.energy = v[3];
    if (!crafted_state_valid(&next)) goto bad;
    *s = next; return 1;
bad:
    memset(s, 0, sizeof(*s)); return 0;
}
