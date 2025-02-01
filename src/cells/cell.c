#include "grid.h"
#include "../utils.h"
#include "../graphics/resources.h"
#include "../api/api.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

tsc_cell_id_pool_t builtin;

void tsc_init_builtin_ids() {
    builtin.push = tsc_registerCell("push", "Push", "Can be pushed from all directions");
    builtin.slide = tsc_registerCell("slide", "Slide", "Can be pushed horizontally");
    builtin.mover = tsc_registerCell("mover", "Mover", "Moves forward one tile per tick");
    builtin.trash = tsc_registerCell("trash", "Trash", "Deletes anything that moves into it");
    builtin.enemy = tsc_registerCell("enemy", "Enemy", "Like Trash but also dies in the process");
    builtin.generator = tsc_registerCell("generator", "Generator", "Duplicates the cell behind it");
    builtin.placeable = tsc_registerCell("place", "Placeable", "Meant to represent areas the player may modify.\nMostly used in puzzles and vaults.");
    builtin.rotator_cw = tsc_registerCell("rotator_cw", "Rotator CW", "Rotates its adjacent neighbours 90 degrees clockwise per tick.");
    builtin.rotator_ccw = tsc_registerCell("rotator_ccw", "Rotator CCW", "Rotates its adjacent neighbours 90 degrees counter-clockwise per tick.");
    builtin.empty = tsc_registerCell("empty", "Empty", "Literally pure nothingness");
    builtin.wall = tsc_registerCell("wall", "Wall", "Immobile");

    tsc_celltable *placeTable = tsc_cell_newTable(builtin.placeable);
    placeTable->flags = TSC_FLAGS_PLACEABLE;

    builtin.textures.icon = tsc_strintern("icon");
    builtin.textures.copy = tsc_strintern("copy");
    builtin.textures.cut = tsc_strintern("cut");
    builtin.textures.del = tsc_strintern("delete");
    builtin.textures.setinitial = tsc_strintern("setinitial");
    builtin.textures.restoreinitial = tsc_strintern("restoreinitial");

    builtin.audio.destroy = tsc_strintern("destroy");
    builtin.audio.explosion = tsc_strintern("explosion");
    builtin.audio.move = tsc_strintern("move");

    builtin.optimizations.gens[0] = tsc_allocOptimization("gen0");
    builtin.optimizations.gens[1] = tsc_allocOptimization("gen1");
    builtin.optimizations.gens[2] = tsc_allocOptimization("gen2");
    builtin.optimizations.gens[3] = tsc_allocOptimization("gen3");
}

tsc_cell __attribute__((hot)) tsc_cell_create(tsc_id_t id, char rot) {
    tsc_cell cell;
    cell.id = id;
    cell.texture = TSC_NULL_TEXTURE;
    cell.rotData = rot & 0b11;
    cell.updated = false;
    cell.effect = TSC_NULL_EFFECT;
    cell.reg = TSC_NULL_REGISTRY;
    cell.lx = TSC_NULL_LAST;
    cell.ly = TSC_NULL_LAST;
    return cell;
}

tsc_cell __attribute__((hot)) tsc_cell_clone(tsc_cell *cell) {
    tsc_cell copy = *cell;
    // TODO: handle reg edge-case
    return copy;
}

void __attribute__((hot)) tsc_cell_destroy(tsc_cell cell) {
    // TODO: handle reg edge-case
}

void tsc_cell_rotate(tsc_cell *cell, signed char amount) {
    char added = tsc_cell_getAddedRotation(cell);
    char rot = tsc_cell_getRotation(cell);
    rot += amount;
    rot %= 4;
    while(rot < 0) rot += 4;
    added += amount;
    tsc_cell_setRotationData(cell, rot, added);
}

char tsc_cell_getRotation(tsc_cell *cell) {
    return cell->rotData & 0b11;
}
signed char tsc_cell_getAddedRotation(tsc_cell *cell) {
    char added = cell->rotData & 0b11111100;
    added >>= 2;
    // force 2s-complement to work
    if(added & 0b00100000) {
        added |= 0b11000000;
    }
    return added;
}

void tsc_cell_setRotationData(tsc_cell *cell, signed char rot, signed char addedRot) {
    rot %= 4;
    while(rot < 0) rot += 4;
    cell->rotData = (addedRot << 2) | rot;
}

void tsc_cell_swap(tsc_cell *a, tsc_cell *b) {
    tsc_cell c = *a;
    *a = *b;
    *b = c;
}

typedef struct tsc_cell_table_arr {
    tsc_celltable **tables;
    const char **ids;
    size_t tablec;
} tsc_cell_table_arr;

static tsc_cell_table_arr cell_table_arr = {NULL, NULL, 0};
static tsc_celltable tsc_cellTables[TSC_ID_COUNT];
static bool tsc_cellTablesDefined[TSC_ID_COUNT] = {false};

tsc_celltable *tsc_cell_newTable(tsc_id_t id) {
    if(!tsc_cellTablesDefined[id]) {
        tsc_celltable table = {NULL};
        tsc_cellTables[id] = table;
        tsc_cellTablesDefined[id] = true;
    }
    return tsc_cellTables + id;
}

// HIGH priority for optimization
// While this is "technically" not thread-safe, you can use it as if it is.
tsc_celltable *tsc_cell_getTable(tsc_cell *cell) {
    if(tsc_cellTablesDefined[cell->id]) {
        return tsc_cellTables + cell->id;
    }
    // Table not found
    return NULL;
}

size_t tsc_cell_getTableFlags(tsc_cell *cell) {
    tsc_celltable *table = tsc_cell_getTable(cell);
    if(table == NULL) return 0;
    return table->flags;
}

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    char rot = tsc_cell_getRotation(cell);
    if(cell->id == builtin.wall) return 0;
    if(cell->id == builtin.slide) return  dir % 2 == rot % 2;

    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 1;
    if(celltable->canMove == NULL) return 1;
    return celltable->canMove(grid, cell, x, y, dir, forceType, force, celltable->payload);
}

float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    char rot = tsc_cell_getRotation(cell);
    if(cell->id == builtin.mover && tsc_streql(forceType, "push")) {
        if(rot == dir) return 1;
        if((rot + 2) % 4 == dir) return -1;

        return 0;
    }

    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 0;
    if(celltable->getBias == NULL) return 0;
    return celltable->getBias(grid, cell, x, y, dir, forceType, force, celltable->payload);
}

int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir) {
    // Can't generate air
    if(cell->id == builtin.empty) return 0;
    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 1;
    if(celltable->canGenerate == NULL) return 1;
    return celltable->canGenerate(grid, cell, x, y, generator, gx, gy, dir, celltable->payload);
}

int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating) {
    if(cell->id == builtin.trash) return 1;
    if(cell->id == builtin.enemy) return 1;
    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 0;
    if(celltable->isTrash == NULL) return 0;
    return celltable->isTrash(grid, cell, x, y, dir, forceType, force, eating, celltable->payload);
}

void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating) {
    if(cell->id == builtin.enemy) {
        tsc_cell empty = tsc_cell_create(builtin.empty, 0);
        tsc_grid_set(grid, x, y, &empty);
        tsc_sound_play(builtin.audio.explosion);
    }
    if(cell->id == builtin.trash) {
        tsc_sound_play(builtin.audio.destroy);
    }
    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return;
    if(celltable->onTrash == NULL) return;
    return celltable->onTrash(grid, cell, x, y, dir, forceType, force, eating, celltable->payload);
}

int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {
    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 0;
    if(celltable->onAcid == NULL) return 0;
    return celltable->isAcid(grid, cell, dir, forceType, force, dissolving, dx, dy, celltable->payload);
}

void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {
    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return;
    if(celltable->onAcid == NULL) return;
    return celltable->onAcid(grid, cell, dir, forceType, force, dissolving, dx, dy, celltable->payload);
}

char *tsc_cell_signal(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy) {
    tsc_celltable *table = tsc_cell_getTable(cell);
    if(table == NULL) return NULL;
    if(table->signal == NULL) return NULL;
    return table->signal(cell, x, y, protocol, data, sender, sx, sy, table->payload);
}
