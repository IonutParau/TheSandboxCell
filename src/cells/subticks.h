#ifndef TSC_SUBTICKS_H
#define TSC_SUBTICKS_H

#include <stddef.h>
#include <stdbool.h>

#define TSC_SUBMODE_TICKED 0
#define TSC_SUBMODE_TRACKED 1
#define TSC_SUBMODE_NEIGHBOUR 2
#define TSC_SUBMODE_CUSTOM 3

typedef struct tsc_subtick_custom_order {
    int order;
    int rotc;
    int *rots;
} tsc_subtick_custom_order;

typedef struct tsc_subtick_t {
    const char **ids;
    size_t idc;
    const char *name;
    union {
        tsc_subtick_custom_order **customOrder;
    };
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

tsc_subtick_t *tsc_subtick_add(tsc_subtick_t subtick);
void tsc_subtick_addCell(tsc_subtick_t *subtick, const char *id);
tsc_subtick_t *tsc_subtick_find(const char *name);
tsc_subtick_t *tsc_subtick_addTicked(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addTracked(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addNeighbour(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addCustom(const char *name, double priority, char spacing, bool parallel, tsc_subtick_custom_order *orders, size_t orderc);
void tsc_subtick_addCore();
void tsc_subtick_run();

#endif
