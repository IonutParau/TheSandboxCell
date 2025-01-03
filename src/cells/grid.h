#ifndef TSC_GRID_H
#define TSC_GRID_H

#include "subticks.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct tsc_cellreg {
    const char **keys;
    char **values;
    size_t len;
} tsc_cellreg;

typedef struct tsc_cell {
    const char *id;
    const char *texture;
    tsc_cellreg *data;
    struct tsc_celltable *celltable;
    size_t flags;
    int lx;
    int ly;
    char rot;
    signed char addedRot;
    bool updated;
} tsc_cell;

typedef struct tsc_grid tsc_grid;

#define TSC_FLAGS_PLACEABLE 1

// Stores function pointers and payload for everything we might need from a modded cell.
// Can be used in place of ID comparison (technically) but it's not much of a benefit.
typedef struct tsc_celltable {
    void *payload;
    void (*update)(tsc_cell *cell, int x, int y, int ux, int uy, void *payload);
    int (*canMove)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, void *payload);
    char *(*signal)(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy, void *payload);
    size_t flags;
    float (*getBias)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, void *payload);
    int (*canGenerate)(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir, void *payload);
    int (*isTrash)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating, void *payload);
    void (*onTrash)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating, void *payload);
    int (*isAcid)(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy, void *payload);
    void (*onAcid)(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy, void *payload);
} tsc_celltable;

tsc_celltable *tsc_cell_newTable(const char *id);
tsc_celltable *tsc_cell_getTable(tsc_cell *cell);
size_t tsc_cell_getTableFlags(tsc_cell *cell);

typedef struct tsc_texture_id_pool_t {
    const char *icon;
    const char *copy;
    const char *cut;
    const char *del;
    const char *setinitial;
    const char *restoreinitial;
} tsc_texture_id_pool_t;

typedef struct tsc_audio_id_pool_t {
    const char *destroy;
    const char *explosion;
    const char *move;
} tsc_audio_id_pool_t;

typedef struct tsc_optimization_id_pool_t {
    size_t gens[4];
} tsc_optimization_id_pool_t;

typedef struct tsc_setting_id_pool_t {
    const char *vsync;
    const char *fullscreen;
    const char *threadCount;
    const char *savingFormat;
    const char *musicVolume;
    const char *sfxVolume;
    const char *unfocusedVolume;
    const char *updateDelay;
    const char *mtpf;
} tsc_setting_id_pool_t;

typedef struct tsc_id_pool_t {
    const char *empty;
    const char *placeable;
    const char *mover;
    const char *generator;
    const char *push;
    const char *slide;
    const char *rotator_cw;
    const char *rotator_ccw;
    const char *enemy;
    const char *trash;
    const char *wall;
    tsc_texture_id_pool_t textures;
    tsc_audio_id_pool_t audio;
    tsc_optimization_id_pool_t optimizations;
    tsc_setting_id_pool_t settings;
} tsc_cell_id_pool_t;

extern tsc_cell_id_pool_t builtin;

// hideapi
void tsc_init_builtin_ids();
// hideapi

tsc_cell tsc_cell_create(const char *id, char rot);
tsc_cell tsc_cell_clone(tsc_cell *cell);
void tsc_cell_swap(tsc_cell *a, tsc_cell *b);
void tsc_cell_destroy(tsc_cell cell);
const char *tsc_cell_get(const tsc_cell *cell, const char *key);
const char *tsc_cell_nthKey(const tsc_cell *cell, size_t idx);
void tsc_cell_set(tsc_cell *cell, const char *key, const char *value);
void tsc_cell_rotate(tsc_cell *cell, signed char amount);

typedef struct tsc_grid {
    tsc_cell *cells;
    tsc_cell *bgs;
    int width;
    int height;
    const char *title;
    const char *desc;
    size_t refc;
    bool *chunkdata;
    int chunkwidth;
    int chunkheight;
    char *optData;
} tsc_grid;

typedef struct tsc_gridStorage {
    const char **id;
    tsc_grid **grid;
    size_t len;
} tsc_gridStorage;

extern tsc_grid *currentGrid;
extern int tsc_maxSliceSize;
// hideapi
extern tsc_gridStorage *gridStorage;
extern size_t gridChunkSize;
// hideapi

tsc_grid *tsc_getGrid(const char *name);
tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description);
void tsc_retainGrid(tsc_grid *grid);
void tsc_deleteGrid(tsc_grid *grid);
void tsc_switchGrid(tsc_grid *grid);
void tsc_copyGrid(tsc_grid *dest, tsc_grid *src);
void tsc_clearGrid(tsc_grid *grid, int width, int height);
void tsc_nukeGrids();

size_t tsc_allocOptimization(const char *id);
size_t tsc_findOptimization(const char *trueID);
size_t tsc_defineEffect(const char *id);
size_t tsc_findEffect(const char *trueID);

size_t tsc_optSize();
size_t tsc_effectSize();

tsc_cell *tsc_grid_get(tsc_grid *grid, int x, int y);
void tsc_grid_set(tsc_grid *grid, int x, int y, tsc_cell *cell);
tsc_cell *tsc_grid_background(tsc_grid *grid, int x, int y);
void tsc_grid_setBackground(tsc_grid *grid, int x, int y, tsc_cell *cell);
int tsc_grid_frontX(int x, char dir);
int tsc_grid_frontY(int y, char dir);
int tsc_grid_shiftX(int x, char dir, int amount);
int tsc_grid_shiftY(int y, char dir, int amount);
void tsc_grid_enableChunk(tsc_grid *grid, int x, int y);
void tsc_grid_disableChunk(tsc_grid *grid, int x, int y);
bool tsc_grid_checkChunk(tsc_grid *grid, int x, int y);
bool tsc_grid_checkRow(tsc_grid *grid, int y);
bool tsc_grid_checkColumn(tsc_grid *grid, int x);
bool tsc_grid_checkOptimization(tsc_grid *grid, int x, int y, size_t optimization);
void tsc_grid_setOptimization(tsc_grid *grid, int x, int y, size_t optimization, bool enabled);

// Cell interactions

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force);
float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force);
int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir);
int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating);
void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating);
int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy);
void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy);
char *tsc_cell_signal(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy);

// Returns how many cells were pushed.
int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);
// Returns how many cells were pulled.
int tsc_grid_pull(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);
// Returns how many cells were grabbed.
int tsc_grid_grab(tsc_grid *grid, int x, int y, char dir, char side, double force, tsc_cell *replacement);
// Returns whether the cell was nudged.
bool tsc_grid_nudge(tsc_grid *grid, int x, int y, char dir, tsc_cell *replacement);

#endif
