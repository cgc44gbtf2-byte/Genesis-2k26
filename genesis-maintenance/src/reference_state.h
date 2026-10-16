#ifndef GENESIS_REFERENCE_STATE_H
#define GENESIS_REFERENCE_STATE_H
#include <stdio.h>
#include <stdint.h>
#define REF_SOUL_CAP 1000000L
#define REF_GIFT_MASK 31UL
#define REF_WARP_MASK UINT32_C(0x3fffffff)
#define REF_QUEST_SOUL_CLAIMED 4194304L
#define REF_QUEST_STAKE 16777216L
typedef struct reference_legacy {
    long souls, willing;
    uint32_t gifts, warps;
    uint64_t grants[2];
    char patron[13];
} REFERENCE_LEGACY;
int reference_legacy_valid(const REFERENCE_LEGACY *state);
int reference_legacy_write(const REFERENCE_LEGACY *state, FILE *fp);
int reference_legacy_read(REFERENCE_LEGACY *state, FILE *fp);
/* Consumes exactly one bounded line, including malformed/oversized payloads. */
int reference_line(FILE *fp, char *out, size_t size);
int reference_weapon_read(long *flags, long *ticks, FILE *fp);
#endif
