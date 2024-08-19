#ifndef TSC_SUBTICKS_H
#define TSC_SUBTICKS_H

#include <stddef.h>

#define TSC_SUBMODE_TICKED 0
#define TSC_SUBMODE_TRACKED 1
#define TSC_SUBMODE_NEIGHBOUR 2

typedef struct tsc_subtick_t {
    const char **ids;
    size_t idc;
    const char *name;
    void *payload;
    double priority;
    char mode;
    char parallel;
    char spacing;
} tsc_subtick_t;

typedef struct tsc_updateinfo_t {
    tsc_subtick_t *subtick;
    int x;
    char rot;
} tsc_updateinfo_t;

typedef struct tsc_subtick_manager_t {
    struct tsc_subtick_t *subs;
    size_t subc;
} tsc_subtick_manager_t;

extern tsc_subtick_manager_t subticks;

void tsc_subtick_add(tsc_subtick_t subtick);
void tsc_subtick_addCell(tsc_subtick_t *subtick, const char *id);
tsc_subtick_t *tsc_subtick_find(const char *name);
void tsc_subtick_addCore();
void tsc_subtick_run();

#endif
