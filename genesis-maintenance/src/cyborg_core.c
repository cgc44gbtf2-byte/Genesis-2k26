/*
 * New Genesis backend; data/prerequisites adapted from Kale/Jalerit.
 * No upstream state indices, unsafe effect bodies or class flags imported.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "cyborg_core.h"

static const CYBORG_COMPONENT components[] = {
#define B(id) (UINT32_C(1) << CY_##id)
#define C(id, key, name, kind, dependencies, cost, humanity) \
    {CY_##id, #key, name, dependencies, cost, humanity},
#include "cyborg_components.def"
#undef C
#undef B
};

uint32_t cyborg_component_bit(int id)
{
    if (id < 0 || id >= CY_COMPONENT_COUNT) return 0;
    return UINT32_C(1) << id;
}

const CYBORG_COMPONENT *cyborg_component_at(size_t index)
{
    if (index >= CY_COMPONENT_COUNT) return NULL;
    return &components[index];
}

static int same_key(const char *a, const char *b)
{
    unsigned char x, y;
    if (a == NULL || b == NULL) return 0;
    do {
        x = (unsigned char)*a++;
        y = (unsigned char)*b++;
        if (x >= 'A' && x <= 'Z') x += 'a' - 'A';
        if (y >= 'A' && y <= 'Z') y += 'a' - 'A';
        if (x != y) return 0;
    } while (x != 0);
    return 1;
}

const CYBORG_COMPONENT *cyborg_component_named(const char *key)
{
    size_t i;
    for (i = 0; i < CY_COMPONENT_COUNT; ++i)
        if (same_key(key, components[i].key)) return &components[i];
    return NULL;
}

int cyborg_component_registry_valid(void)
{
    size_t i, j;
    const CYBORG_COMPONENT *p;
    if (sizeof(components) / sizeof(components[0]) != CY_COMPONENT_COUNT)
        return 0;
    for (i = 0; i < CY_COMPONENT_COUNT; ++i) {
        p = &components[i];
        /* Dependencies point strictly backwards: no cycles or self grants. */
        if (p->id != (int)i || p->key[0] == 0 || p->name[0] == 0
            || p->primal <= 0 || p->primal > 100
            || p->humanity <= 0 || p->humanity > 100
            || (p->requires & ~(cyborg_component_bit(p->id) - 1)) != 0)
            return 0;
        for (j = 0; j < i; ++j)
            if (same_key(p->key, components[j].key)) return 0;
    }
    return 1;
}

static int closed_mask(uint32_t mask)
{
    size_t i;
    if ((mask & ~CY_COMPONENT_MASK) != 0) return 0;
    for (i = 0; i < CY_COMPONENT_COUNT; ++i)
        if ((mask & cyborg_component_bit((int)i)) != 0
            && (mask & components[i].requires) != components[i].requires)
            return 0;
    return 1;
}

int cyborg_core_valid(const CYBORG_CORE *s)
{
    size_t i;
    long spent = 0;
    if (s == NULL || !closed_mask(s->permitted) || !closed_mask(s->installed)
        || (s->installed & ~s->permitted) != 0
        || s->humanity_limit < 0 || s->humanity < 0
        || s->humanity > s->humanity_limit) return 0;
    for (i = 0; i < CY_COMPONENT_COUNT; ++i)
        if ((s->installed & cyborg_component_bit((int)i)) != 0)
            spent += components[i].humanity;
    return spent <= s->humanity_limit
        && s->humanity == s->humanity_limit - spent;
}

int cyborg_core_init(CYBORG_CORE *s, uint32_t permitted, long humanity)
{
    CYBORG_CORE next;
    if (s == NULL || !closed_mask(permitted) || humanity < 0) return CY_INVALID;
    memset(&next, 0, sizeof(next));
    next.permitted = permitted;
    next.humanity = next.humanity_limit = humanity;
    *s = next;
    return CY_OK;
}

int cyborg_core_install(CYBORG_CORE *s, int id, long *primal)
{
    const CYBORG_COMPONENT *p;
    uint32_t bit = cyborg_component_bit(id);
    if (!cyborg_core_valid(s) || !bit || primal == NULL
        || primal == &s->humanity || primal == &s->humanity_limit
        || *primal < 0) return CY_INVALID;
    p = &components[id];
    if ((s->permitted & bit) == 0) return CY_NOT_SELECTED;
    if ((s->installed & bit) != 0) return CY_ALREADY_INSTALLED;
    if ((s->installed & p->requires) != p->requires) return CY_MISSING_DEPENDENCY;
    if (*primal < p->primal) return CY_NO_PRIMAL;
    if (s->humanity < p->humanity) return CY_NO_HUMANITY;
    /* All checks precede both resource debits and the installation. */
    *primal -= p->primal;
    s->humanity -= p->humanity;
    s->installed |= bit;
    return CY_OK;
}

int cyborg_core_uninstall(CYBORG_CORE *s, int id)
{
    uint32_t bit = cyborg_component_bit(id), remaining;
    if (!cyborg_core_valid(s) || !bit) return CY_INVALID;
    if ((s->installed & bit) == 0) return CY_NOT_INSTALLED;
    remaining = s->installed & ~bit;
    if (!closed_mask(remaining)) return CY_DEPENDENT_INSTALLED;
    s->installed = remaining;
    s->humanity += components[id].humanity;
    /* No primal refund; no repeated refund on a missing component. */
    return CY_OK;
}

int cyborg_core_encode(const CYBORG_CORE *s, char *out, size_t size)
{
    char record[CY_CORE_RECORD_SIZE];
    int n;
    if (!cyborg_core_valid(s) || out == NULL) return CY_INVALID;
    n = snprintf(record, sizeof(record), "CyborgCore 1 %lu %lu %ld %ld",
        (unsigned long)s->permitted, (unsigned long)s->installed,
        s->humanity_limit, s->humanity);
    if (n < 0 || (size_t)n >= sizeof(record) || (size_t)n >= size)
        return CY_INVALID;
    memcpy(out, record, (size_t)n + 1);
    return CY_OK;
}

int cyborg_core_decode(CYBORG_CORE *s, const char *record)
{
    static const char prefix[] = "CyborgCore 1 ";
    CYBORG_CORE next;
    const char *p;
    char *end;
    unsigned long numbers[4];
    size_t i, length;
    if (s == NULL || record == NULL) return CY_INVALID;
    for (length = 0; length < CY_CORE_RECORD_SIZE && record[length]; ++length) {}
    if (length >= CY_CORE_RECORD_SIZE || length < sizeof(prefix) - 1
        || memcmp(record, prefix, sizeof(prefix) - 1) != 0) return CY_INVALID;
    p = record + sizeof(prefix) - 1;
    for (i = 0; i < 4; ++i) {
        if (*p < '0' || *p > '9') return CY_INVALID;
        errno = 0;
        numbers[i] = strtoul(p, &end, 10);
        if (errno == ERANGE || end == p) return CY_INVALID;
        if (i < 3) {
            if (*end != ' ') return CY_INVALID;
            p = end + 1;
        } else if (*end != 0) return CY_INVALID;
    }
    if (numbers[0] > UINT32_MAX || numbers[1] > UINT32_MAX
        || numbers[2] > LONG_MAX || numbers[3] > LONG_MAX) return CY_INVALID;
    memset(&next, 0, sizeof(next));
    next.permitted = (uint32_t)numbers[0];
    next.installed = (uint32_t)numbers[1];
    next.humanity_limit = (long)numbers[2];
    next.humanity = (long)numbers[3];
    if (!cyborg_core_valid(&next)) return CY_INVALID;
    *s = next;
    return CY_OK;
}

int cyborg_core_write_player(const CYBORG_CORE *s, FILE *fp)
{
    char record[CY_CORE_RECORD_SIZE];
    if (fp == NULL || cyborg_core_encode(s, record, sizeof(record)) != CY_OK)
        return CY_INVALID;
    return fprintf(fp, "%s\n", record) < 0 ? CY_INVALID : CY_OK;
}

static int player_line(char *record, const char *prefix, FILE *fp)
{
    size_t initial = strlen(prefix), length = initial;
    int c, bad = 0, started = 0;
    if (fp == NULL) return CY_INVALID;
    memcpy(record, prefix, length);
    /* Continue consuming oversized/binary records so the following player
     * field cannot be interpreted as part of an invalid Cyborg payload.
     */
    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (!started && (c == ' ' || c == '\t')) continue;
        started = 1;
        if (c == 0) bad = 1;
        if (length < CY_CORE_RECORD_SIZE - 1) record[length++] = (char)c;
        else bad = 1;
    }
    if (length > initial && record[length - 1] == '\r') --length;
    record[length] = 0;
    return !bad && !ferror(fp) ? CY_OK : CY_INVALID;
}

int cyborg_core_read_player(CYBORG_CORE *s, FILE *fp)
{
    char record[CY_CORE_RECORD_SIZE];
    if (s == NULL || fp == NULL) return CY_INVALID;
    if (player_line(record, "CyborgCore ", fp) == CY_OK
        && cyborg_core_decode(s, record) == CY_OK)
        return CY_OK;
    memset(s, 0, sizeof(*s));
    return CY_INVALID;
}

int cyborg_runtime_valid(const CYBORG_RUNTIME *s)
{
    return s != NULL && s->nanites >= 0 && s->nanites <= 10000
        && s->rifle >= 0 && s->rifle <= 12
        && (s->factory == 0 || s->factory == 1);
}

int cyborg_runtime_write_player(const CYBORG_RUNTIME *s, FILE *fp)
{
    if (fp == NULL || !cyborg_runtime_valid(s)) return CY_INVALID;
    return fprintf(fp, "CyborgRuntime 1 %ld %ld %ld\n",
                   s->nanites, s->rifle, s->factory) < 0 ? CY_INVALID : CY_OK;
}

int cyborg_runtime_read_player(CYBORG_RUNTIME *s, FILE *fp)
{
    char record[CY_CORE_RECORD_SIZE];
    const char *p;
    char *end;
    unsigned long values[4];
    CYBORG_RUNTIME next;
    size_t i;
    if (s == NULL || fp == NULL) return CY_INVALID;
    if (player_line(record, "CyborgRuntime ", fp) != CY_OK) goto invalid;
    p = record + strlen("CyborgRuntime ");
    for (i = 0; i < 4; ++i) {
        if (*p < '0' || *p > '9') goto invalid;
        errno = 0;
        values[i] = strtoul(p, &end, 10);
        if (errno == ERANGE || end == p || values[i] > LONG_MAX) goto invalid;
        p = end;
        if (i < 3) {
            if (*p++ != ' ') goto invalid;
        } else if (*p != 0) goto invalid;
    }
    if (values[0] != 1) goto invalid;
    memset(&next, 0, sizeof(next));
    next.nanites = (long)values[1];
    next.rifle = (long)values[2];
    next.factory = (long)values[3];
    if (!cyborg_runtime_valid(&next)) goto invalid;
    *s = next;
    return CY_OK;
invalid:
    memset(s, 0, sizeof(*s));
    return CY_INVALID;
}

int cyborg_systems_valid(const CYBORG_SYSTEMS *s)
{
    return s != NULL && s->face >= 0 && s->face <= 5
        && s->limbs >= 0 && s->limbs <= 5 && s->body >= 0 && s->body <= 6
        && (s->body == 0 || s->limbs >= 1)
        && (s->face < 5 || s->body >= 4)
        && s->demolition >= 0 && s->demolition <= 12
        && s->bombs >= 0 && s->bombs <= 20
        && s->armed >= 0 && s->armed <= 1 && (!s->armed || s->bombs > 0)
        && s->bionic >= 0 && s->bionic <= 10000000;
}

int cyborg_systems_write_player(const CYBORG_SYSTEMS *s, FILE *fp)
{
    if (fp == NULL || !cyborg_systems_valid(s)) return CY_INVALID;
    return fprintf(fp, "CyborgSystems 1 %ld %ld %ld %ld %ld %ld %ld\n",
        s->face, s->limbs, s->body, s->demolition, s->bombs, s->armed,
        s->bionic) < 0 ? CY_INVALID : CY_OK;
}

int cyborg_systems_read_player(CYBORG_SYSTEMS *s, FILE *fp)
{
    char record[CY_CORE_RECORD_SIZE], *end;
    const char *p;
    unsigned long values[8];
    CYBORG_SYSTEMS next;
    size_t i;
    if (s == NULL || fp == NULL) return CY_INVALID;
    if (player_line(record, "CyborgSystems ", fp) != CY_OK) goto invalid;
    p = record + strlen("CyborgSystems ");
    for (i = 0; i < 8; ++i) {
        if (*p < '0' || *p > '9') goto invalid;
        errno = 0; values[i] = strtoul(p, &end, 10);
        if (errno == ERANGE || end == p || values[i] > LONG_MAX) goto invalid;
        p = end;
        if (i < 7) { if (*p++ != ' ') goto invalid; }
        else if (*p != 0) goto invalid;
    }
    if (values[0] != 1) goto invalid;
    memset(&next, 0, sizeof(next));
    next.face = values[1]; next.limbs = values[2]; next.body = values[3];
    next.demolition = values[4]; next.bombs = values[5];
    next.armed = values[6]; next.bionic = values[7];
    if (!cyborg_systems_valid(&next)) goto invalid;
    *s = next; return CY_OK;
invalid:
    memset(s, 0, sizeof(*s)); return CY_INVALID;
}
