/* Independent Cyborg component and resource state. */
#ifndef GENESIS_CYBORG_CORE_H
#define GENESIS_CYBORG_CORE_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Stable component IDs, independent of legacy CLASS_* and powers[] indices.
 * These component permissions are NOT the final 75% ability quota.
 */
enum {
    CY_HEART, CY_LIVER, CY_STOMACH, CY_NEUROPROCESSOR, CY_BATTERY,
    CY_NANITE_FACTORY, CY_LEFT_ARM, CY_RIGHT_ARM, CY_LEFT_LEG, CY_RIGHT_LEG,
    CY_EYES, CY_EARS, CY_ADRENAL_BOOST, CY_BOOSTERWARE, CY_BOOSTMASTER,
    CY_WOLVERS, CY_CHAMELEON, CY_MUSCLE, CY_BONELACE, CY_RADIOSPLICE,
    CY_TARGETCOMP, CY_TELEOPTICS, CY_RAILGUN, CY_TASER, CY_COMPONENT_COUNT
};
#define CY_COMPONENT_MASK UINT32_C(0x00ffffff)
#define CY_CORE_RECORD_SIZE 128

typedef struct cyborg_component {
    int id;
    const char *key;
    const char *name;
    uint32_t requires;
    long primal;
    long humanity;
} CYBORG_COMPONENT;

typedef struct cyborg_core {
    uint32_t permitted;
    uint32_t installed;
    long humanity_limit;
    long humanity;
} CYBORG_CORE;

typedef struct cyborg_runtime {
    long nanites; /* bounded 0..10000 */
    long rifle;   /* trained 0..12 */
    long factory; /* explicit on/off; requires installed factory */
} CYBORG_RUNTIME;

/* Independent Mindcloud-style progression and Vortex ammunition. Versioned
 * separately: older CyborgCore/Runtime records retain their exact layouts.
 */
typedef struct cyborg_systems {
    long face;       /* 0..5 */
    long limbs;      /* 0..5 */
    long body;       /* 0..6; requires limbs>=1 */
    long demolition; /* 0..12 */
    long bombs;      /* 0..20 */
    long armed;      /* 0..1; requires a bomb */
    long bionic;     /* explicit training currency, 0..10000000 */
} CYBORG_SYSTEMS;

enum {
    CY_OK, CY_INVALID, CY_NOT_SELECTED, CY_ALREADY_INSTALLED,
    CY_MISSING_DEPENDENCY, CY_NO_PRIMAL, CY_NO_HUMANITY,
    CY_NOT_INSTALLED, CY_DEPENDENT_INSTALLED
};

uint32_t cyborg_component_bit(int id);
const CYBORG_COMPONENT *cyborg_component_at(size_t index);
const CYBORG_COMPONENT *cyborg_component_named(const char *key);
int cyborg_component_registry_valid(void);
int cyborg_core_valid(const CYBORG_CORE *state);
/* Caller must authorize standalone Cyborg identity or verified hybrid selection.
 * Budget is explicit; the standalone adapter currently supplies 100.
 */
int cyborg_core_init(CYBORG_CORE *state, uint32_t permitted, long humanity);
int cyborg_core_install(CYBORG_CORE *state, int id, long *primal);
int cyborg_core_uninstall(CYBORG_CORE *state, int id);
/* Versioned codec and bounded player-record helpers. No power authorization. */
int cyborg_core_encode(const CYBORG_CORE *state, char *out, size_t size);
int cyborg_core_decode(CYBORG_CORE *state, const char *record);
int cyborg_core_write_player(const CYBORG_CORE *state, FILE *fp);
/* Called after fread_word consumed CyborgCore. Consumes exactly its line.
 * Invalid records clear only Cyborg state, rather than carrying old grants.
 */
int cyborg_core_read_player(CYBORG_CORE *state, FILE *fp);
int cyborg_runtime_valid(const CYBORG_RUNTIME *state);
int cyborg_runtime_write_player(const CYBORG_RUNTIME *state, FILE *fp);
int cyborg_runtime_read_player(CYBORG_RUNTIME *state, FILE *fp);
int cyborg_systems_valid(const CYBORG_SYSTEMS *state);
int cyborg_systems_write_player(const CYBORG_SYSTEMS *state, FILE *fp);
int cyborg_systems_read_player(CYBORG_SYSTEMS *state, FILE *fp);
#endif
