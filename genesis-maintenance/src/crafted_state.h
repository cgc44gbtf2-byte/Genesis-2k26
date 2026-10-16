#ifndef GENESIS_CRAFTED_STATE_H
#define GENESIS_CRAFTED_STATE_H
#include <stdio.h>
#include "parents.h"
#define CRAFTED_ENERGY_CAP 10000L
typedef struct crafted_state {
    long parent, rank, energy;
} CRAFTED_STATE;
int crafted_state_valid(const CRAFTED_STATE *s);
int crafted_state_read(CRAFTED_STATE *s, FILE *fp);
int crafted_state_write(const CRAFTED_STATE *s, FILE *fp);
#endif
