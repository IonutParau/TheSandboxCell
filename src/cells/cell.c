#include "grid.h"
#include "../utils.h"
#include "../graphics/resources.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

tsc_cell_id_pool_t builtin;

void tsc_init_builtin_ids() {
    builtin.push = tsc_strintern("push");
    builtin.slide = tsc_strintern("slide");
    builtin.mover = tsc_strintern("mover");
    builtin.trash = tsc_strintern("trash");
    builtin.enemy = tsc_strintern("enemy");
    builtin.generator = tsc_strintern("generator");
    builtin.placeable = tsc_strintern("place");
    builtin.rotator_cw = tsc_strintern("rotator_cw");
    builtin.rotator_ccw = tsc_strintern("rotator_ccw");
    builtin.empty = tsc_strintern("empty");
    builtin.wall = tsc_strintern("wall");

    builtin.textures.icon = tsc_strintern("icon");
    builtin.textures.copy = tsc_strintern("copy");
    builtin.textures.cut = tsc_strintern("cut");
    builtin.textures.del = tsc_strintern("delete");
    builtin.textures.setinitial = tsc_strintern("setinitial");
    builtin.textures.restoreinitial = tsc_strintern("restoreinitial");

    builtin.audio.destroy = tsc_strintern("destroy");
    builtin.audio.explosion = tsc_strintern("explosion");
}

tsc_cell __attribute__((hot)) tsc_cell_create(const char *id, char rot) {
    tsc_cell cell;
    cell.id = id;
    cell.texture = NULL;
    cell.rot = rot % 4;
    cell.data = NULL;
    cell.updated = false;
    cell.celltable = NULL;
    cell.flags = 0;
    // -1 means no interpolation please
    cell.lx = -1;
    cell.ly = -1;
    cell.addedRot = 0;
    return cell;
}

tsc_cell __attribute__((hot)) tsc_cell_clone(tsc_cell *cell) {
    tsc_cell copy = *cell;
    if(cell->data == NULL) return copy;

    copy.data = malloc(sizeof(tsc_cellreg));
    copy.data->len = cell->data->len;
    copy.data->keys = malloc(sizeof(const char *) * copy.data->len);
    memcpy(copy.data->keys, cell->data->keys, sizeof(const char *) * copy.data->len);
    copy.data->values = malloc(sizeof(const char *) * copy.data->len);
    for(size_t i = 0; i < cell->data->len; i++) {
        copy.data->values[i] = tsc_strdup(cell->data->values[i]);
    }
    return copy;
}

void __attribute__((hot)) tsc_cell_destroy(tsc_cell cell) {
    if(cell.data != NULL) {
        for(size_t i = 0; i < cell.data->len; i++) {
            free(cell.data->values[i]);
        }
        free(cell.data->keys);
        free(cell.data->values);
        free(cell.data);
    }
}

const char *tsc_cell_get(const tsc_cell *cell, const char *key) {
    if(cell->data == NULL) return NULL;
    for(size_t i = 0; i < cell->data->len; i++) {
        if(tsc_streql(cell->data->keys[i], key)) {
            return cell->data->values[i];
        }
    }
    return NULL;
}

const char *tsc_cell_nthKey(const tsc_cell *cell, size_t idx) {
    if(cell->data == NULL) return NULL;
    if(idx >= cell->data->len) return NULL;
    return cell->data->keys[idx];
}

void tsc_cell_set(tsc_cell *cell, const char *key, const char *value) {
    if(value == NULL) {
        if(cell->data == NULL) return;
        // Remove
        size_t j = 0;
        for(size_t i = 0; i < cell->data->len; i++) {
            if(tsc_streql(cell->data->keys[i], key)) {
                free(cell->data->values[i]);
                continue;
            }
            cell->data->keys[j] = cell->data->keys[i];
            cell->data->values[j] = cell->data->values[i];
            j++;
        }
        if(j != cell->data->len) {
            // Something was removed
            cell->data->len = j;
            cell->data->keys = realloc(cell->data->keys, sizeof(const char *) * j);
            cell->data->values = realloc(cell->data->values, sizeof(const char *) * j);
        }
        return;
    }
    if(cell->data == NULL) {
        cell->data = malloc(sizeof(tsc_cellreg));
        cell->data->len = 0;
        cell->data->keys = NULL;
        cell->data->values = NULL;
    }
    for(size_t i = 0; i < cell->data->len; i++) {
        if(tsc_streql(cell->data->keys[i], key)) {
            free(cell->data->values[i]);
            cell->data->values[i] = tsc_strdup(value);
            return;
        }
    }
    size_t idx = cell->data->len++;
    cell->data->keys = realloc(cell->data->keys, sizeof(const char *) * cell->data->len);
    cell->data->keys[idx] = tsc_strintern(key);
    cell->data->values = realloc(cell->data->values, sizeof(const char *) * cell->data->len);
    cell->data->values[idx] = tsc_strdup(value);
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

tsc_celltable *tsc_cell_newTable(const char *id) {
    tsc_celltable *table = malloc(sizeof(tsc_celltable));
    tsc_celltable empty = {NULL, NULL, NULL};
    *table = empty;
    size_t idx = cell_table_arr.tablec++;
    cell_table_arr.tables = realloc(cell_table_arr.tables, sizeof(tsc_celltable *) * cell_table_arr.tablec);
    cell_table_arr.ids = realloc(cell_table_arr.ids, sizeof(const char *) * cell_table_arr.tablec);
    cell_table_arr.tables[idx] = table;
    cell_table_arr.ids[idx] = id;
    return table;
}

// HIGH priority for optimization
// While this is "technically" not thread-safe, you can use it as if it is.
tsc_celltable *tsc_cell_getTable(tsc_cell *cell) {
    // Locally cached VTable (id should never be assigned to so this is fine)
    if(cell->celltable != NULL) return cell->celltable;

    for(size_t i = 0; i < cell_table_arr.tablec; i++) {
        if(cell_table_arr.ids[i] == cell->id) {
            tsc_celltable *table = cell_table_arr.tables[i];
            cell->celltable = table;
            return table;
        }
    }

    // Table not found.
    return NULL;
}

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    if(cell->id == builtin.wall) return 0;
    if(cell->id == builtin.slide) return  dir % 2 == cell->rot % 2;

    tsc_celltable *celltable = tsc_cell_getTable(cell);
    if(celltable == NULL) return 1;
    if(celltable->canMove == NULL) return 1;
    return celltable->canMove(grid, cell, x, y, dir, forceType, force, celltable->payload);
}

float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force) {
    if(cell->id == builtin.mover && tsc_streql(forceType, "push")) {
        if(cell->rot == dir) return 1;
        if((cell->rot + 2) % 4 == dir) return -1;

        return 0;
    }

    return 0;
}

int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir) {
    // Can't generate air
    if(cell->id == builtin.empty) return 0;
    return 1;
}

int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating) {
    if(cell->id == builtin.trash) return 1;
    if(cell->id == builtin.enemy) return 1;
    return 0;
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
}

int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {
    return 0;
}

void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy) {

}

char *tsc_cell_signal(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy) {
    tsc_celltable *table = tsc_cell_getTable(cell);
    if(table == NULL) return NULL;
    if(table->signal == NULL) return NULL;
    return table->signal(cell, x, y, protocol, data, sender, sx, sy, table->payload);
}
