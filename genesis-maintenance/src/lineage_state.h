#ifndef GENESIS_LINEAGE_STATE_H
#define GENESIS_LINEAGE_STATE_H
#include <stdio.h>
#include "parents.h"
#define LINEAGE_TRACKS 20
#define LINEAGE_ENERGY_CAP 10000L
typedef struct lineage_state {
    long parent, energy, dharma, tracks[LINEAGE_TRACKS];
} LINEAGE_STATE;
int lineage_track_count(long parent);
int lineage_state_valid(const LINEAGE_STATE *s);
int lineage_state_read(LINEAGE_STATE *s, FILE *fp);
int lineage_state_write(const LINEAGE_STATE *s, FILE *fp);
#endif
