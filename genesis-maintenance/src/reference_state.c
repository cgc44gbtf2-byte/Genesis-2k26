/* Original Genesis persistence. No reference class bitfields are imported. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "reference_state.h"

int reference_line(FILE *fp, char *out, size_t size)
{
    int c, bad = 0, started = 0;
    size_t n = 0;
    if (!fp || !out || size < 2) return 0;
    while ((c = fgetc(fp)) != EOF && c != '\n') {
        if (!started && (c == ' ' || c == '\t')) continue;
        started = 1;
        if (!c) bad = 1;
        if (n + 1 < size) out[n++] = c; else bad = 1;
    }
    if (n && out[n-1] == '\r') --n;
    out[n] = 0;
    return !bad && !ferror(fp) && n > 0;
}

static int patron_valid(const char *name)
{
    size_t n;
    for (n = 0; n < 13 && name[n]; ++n)
        if (!((name[n] >= 'A' && name[n] <= 'Z') || (name[n] >= 'a' && name[n] <= 'z')))
            return 0;
    return n <= 12;
}

int reference_legacy_valid(const REFERENCE_LEGACY *s)
{
    uint32_t mask;
    int count = 0;
    if (!s) return 0;
    for (mask = s->warps; mask; mask &= mask - 1) ++count;
    return s && s->souls >= 0 && s->souls <= REF_SOUL_CAP
        && s->willing >= 0 && s->willing <= 1
        && !(s->gifts & ~REF_GIFT_MASK) && !(s->warps & ~REF_WARP_MASK)
        && count <= 10 && patron_valid(s->patron);
}

int reference_legacy_write(const REFERENCE_LEGACY *s, FILE *fp)
{
    if (!fp || !reference_legacy_valid(s)) return 0;
    return fprintf(fp, "ParentLegacy 1 %ld %lu %lu %ld %s\n", s->souls,
        (unsigned long)s->gifts, (unsigned long)s->warps, s->willing,
        s->patron[0] ? s->patron : "-") >= 0;
}

int reference_legacy_read(REFERENCE_LEGACY *s, FILE *fp)
{
    char text[128], *end;
    const char *p;
    unsigned long values[5];
    REFERENCE_LEGACY next;
    size_t i, len;
    if (!s || !fp) return 0;
    if (!reference_line(fp, text, sizeof(text))) goto bad;
    p = text;
    for (i = 0; i < 5; ++i) {
        if (*p < '0' || *p > '9') goto bad;
        errno = 0; values[i] = strtoul(p, &end, 10);
        if (errno == ERANGE || p == end || values[i] > LONG_MAX || *end != ' ') goto bad;
        p = end + 1;
    }
    len = strlen(p);
    if (values[0] != 1 || values[2] > REF_GIFT_MASK || values[3] > REF_WARP_MASK
        || len == 0 || len > 12) goto bad;
    memset(&next, 0, sizeof(next));
    next.souls = values[1]; next.gifts = values[2]; next.warps = values[3];
    next.willing = values[4];
    if (strcmp(p, "-")) memcpy(next.patron, p, len + 1);
    if (!reference_legacy_valid(&next)) goto bad;
    *s = next; return 1;
bad:
    memset(s, 0, sizeof(*s)); return 0;
}

int reference_weapon_read(long *flags, long *ticks, FILE *fp)
{
    char line[80], *p;
    long value[3];
    int i;
    if (!flags || !ticks) return 0;
    *flags = *ticks = 0;
    if (!reference_line(fp, line, sizeof(line))) return 0;
    p = line;
    for (i = 0; i < 3; ++i) {
        value[i] = 0;
        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') {
            value[i] = value[i] * 10 + (*p++ - '0');
            if (value[i] > 100) return 0;
        }
        if (i < 2) { if (*p++ != ' ') return 0; }
    }
    if (*p || value[0] != 1 || value[1] < 1 || value[1] > 15
        || value[2] < 1 || value[2] > 12) return 0;
    *flags = value[1]; *ticks = value[2]; return 1;
}
