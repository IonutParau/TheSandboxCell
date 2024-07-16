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
    size_t lx;
    size_t ly;
    char rot;
    signed char addedRot;
    bool updated;
} tsc_cell;

typedef struct tsc_grid tsc_grid;

// Stores function pointers and payload for everything we might need from a modded cell.
// Can be used in place of ID comparison (technically) but it's not much of a benefit.
typedef struct tsc_celltable {
    void *payload;
    void (*update)(tsc_cell *cell, int x, int y, int ux, int uy, void *payload);
    int (*canMove)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, void *payload);
} tsc_celltable;

tsc_celltable *tsc_cell_newTable(const char *id);
tsc_celltable *tsc_cell_getTable(tsc_cell *cell);

typedef struct tsc_texture_id_pool_t {
    const char *icon;
} tsc_texture_id_pool_t;

typedef struct tsc_audio_id_pool_t {
    const char *destroy;
    const char *explosion;
} tsc_audio_id_pool_t;

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
void tsc_cell_set(tsc_cell *cell, const char *key, const char *value);
size_t tsc_cell_getFlags(tsc_cell *cell);
void tsc_cell_setFlags(tsc_cell *cell, size_t flags);

typedef struct tsc_grid {
    tsc_cell *cells;
    int width;
    int height;
    const char *title;
    const char *desc;
    size_t refc;
} tsc_grid;

typedef struct tsc_gridStorage {
    const char **id;
    tsc_grid **grid;
    size_t len;
} tsc_gridStorage;

extern tsc_grid *currentGrid;
// hideapi
extern tsc_gridStorage *gridStorage;
// hideapi

tsc_grid *tsc_getGrid(const char *name);
tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description);
void tsc_retainGrid(tsc_grid *grid);
void tsc_deleteGrid(tsc_grid *grid);
void tsc_switchGrid(tsc_grid *grid);
void tsc_copyGrid(tsc_grid *dest, tsc_grid *src);
void tsc_clearGrid(tsc_grid *grid, int width, int height);
void tsc_nukeGrids();

tsc_cell *tsc_grid_get(tsc_grid *grid, int x, int y);
void tsc_grid_set(tsc_grid *grid, int x, int y, tsc_cell *cell);
int tsc_grid_frontX(int x, char dir);
int tsc_grid_frontY(int y, char dir);
int tsc_grid_shiftX(int x, char dir, int amount);
int tsc_grid_shiftY(int y, char dir, int amount);

// Cell interactions 

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType);
float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType);
int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir);
int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *eating);
void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *eating);
int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *dissolving);
void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *dissolving);

// Returns how many cells were pushed.
int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);

#endif
