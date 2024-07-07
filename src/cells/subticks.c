#include "subticks.h"
#include "grid.h"
#include "../utils.h"
#include "../threads/workers.h"
#include <stdlib.h>
#include <stdio.h>

tsc_subtick_manager_t subticks = {NULL, 0};

static tsc_updateinfo_t *subticks_updateBuffer = NULL;
static size_t subticks_updateLength = 0;

static tsc_updateinfo_t *subticks_getBuffer(size_t amount) {
    if(subticks_updateLength < amount) {
        subticks_updateBuffer = realloc(subticks_updateBuffer, sizeof(tsc_updateinfo_t) * amount);
        subticks_updateLength = amount;
        return subticks_updateBuffer;
    }

    return subticks_updateBuffer;
}

static void tsc_subtick_swap(tsc_subtick_t *a, tsc_subtick_t *b) {
    tsc_subtick_t c = *a;
    *a = *b;
    *b = c;
}

static int tsc_subtick_partition(int low, int high) {
    double pivot = subticks.subs[low].priority;
    int i = low;
    int j = high;

    while(i < j) {
        while(subticks.subs[i].priority <= pivot && i <= high - 1) {
            i++;
        }

        while(subticks.subs[j].priority > pivot && j >= low + 1) {
            j--;
        }
        if(i < j) {
            tsc_subtick_swap(subticks.subs + i, subticks.subs + j);
        }
    }

    tsc_subtick_swap(subticks.subs + low, subticks.subs + j);
    return j;
}

// Warning: Recursive. If there are too many subticks then the sorting function might stackoverflow.
// TODO: use heap stack
static void tsc_subtick_sort(int low, int high) {
    if(low < high) {
        int partition = tsc_subtick_partition(low, high);
        tsc_subtick_sort(low, partition - 1);
        tsc_subtick_sort(partition + 1, high);
    }
}

void tsc_subtick_add(tsc_subtick_t subtick) {
    size_t idx = subticks.subc++;
    subticks.subs = realloc(subticks.subs, sizeof(tsc_subtick_t) * subticks.subc);
    subticks.subs[idx] = subtick;

    tsc_subtick_sort(0, subticks.subc - 1);
}

// Names must be interned because I said so
tsc_subtick_t *tsc_subtick_find(const char *name) {
    for(size_t i = 0; i < subticks.subc; i++) {
        if(subticks.subs[i].name == name) return subticks.subs + i;
    }
    return NULL;
}

static void tsc_subtick_worker(void *data) {
    tsc_updateinfo_t *info = data;

    char mode = info->subtick->mode;

    if(mode == TSC_SUBMODE_TRACKED) {
        char rot = info->rot;
        if(rot == 0) {
            for(int x = currentGrid->width-1; x >= 0; x--) {
                int y = info->x;
                tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
                if(cell == NULL) continue;
                if(cell->rot != rot) continue;
                if(cell->updated) continue;
                for(size_t i = 0; i < info->subtick->idc; i++) {
                    if(info->subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break; // breaks out of id loop.
                        if(table->update == NULL) break;
                        cell->updated = true;
                        table->update(cell, x, y, x, y, table->payload);
                        break;
                    }
                }
            }
        }
        if(rot == 2) {
            for(int x = 0; x < currentGrid->width; x++) {
                int y = info->x;
                tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
                if(cell == NULL) continue;
                if(cell->rot != rot) continue;
                if(cell->updated) continue;
                for(size_t i = 0; i < info->subtick->idc; i++) {
                    if(info->subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break; // breaks out of id loop.
                        if(table->update == NULL) break;
                        cell->updated = true;
                        table->update(cell, x, y, x, y, table->payload);
                        break;
                    }
                }
            }
        }
        if(rot == 1) {
            for(int y = currentGrid->height-1; y >= 0; y--) {
                int x = info->x;
                tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
                if(cell->rot != rot) continue;
                if(cell->updated) continue;
                for(size_t i = 0; i < info->subtick->idc; i++) {
                    if(info->subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break; // breaks out of id loop.
                        if(table->update == NULL) break;
                        cell->updated = true;
                        table->update(cell, x, y, x, y, table->payload);
                        break;
                    }
                }
            }
        }
        if(rot == 3) {
            for(int y = 0; y < currentGrid->height; y++) {
                int x = info->x;
                tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
                if(cell->rot != rot) continue;
                if(cell->updated) continue;
                for(size_t i = 0; i < info->subtick->idc; i++) {
                    if(info->subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break; // breaks out of id loop.
                        if(table->update == NULL) break;
                        cell->updated = true;
                        table->update(cell, x, y, x, y, table->payload);
                        break;
                    }
                }
            }
        }
        return;
    }

    if(mode == TSC_SUBMODE_TICKED) {
        for(int y = 0; y < currentGrid->height; y++) {
            tsc_cell *cell = tsc_grid_get(currentGrid, info->x, y);
            for(size_t i = 0; i < info->subtick->idc; i++) {
                if(info->subtick->ids[i] == cell->id) {
                    tsc_celltable *table = tsc_cell_getTable(cell);
                    if(table == NULL) break; // breaks out of id loop.
                    if(table->update == NULL) break;
                    cell->updated = true;
                    table->update(cell, info->x, y, info->x, y, table->payload);
                    break;
                }
            }
        }
        return;
    }
    
    if(mode == TSC_SUBMODE_NEIGHBOUR) {
        int off[] = {
            -1, 0,
            1, 0,
            0, -1,
            0, 1
        };
        int offc = 4;
        for(int i = 0; i < offc; i++) {
            for(int y = 0; y < currentGrid->height; y++) {
                int x = info->x;
                int cx = x + off[i*2];
                int cy = y + off[i*2+1];
                tsc_cell *cell = tsc_grid_get(currentGrid, cx, cy);
                if(cell == NULL) continue;
                for(size_t i = 0; i < info->subtick->idc; i++) {
                    if(info->subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break; // breaks out of id loop.
                        if(table->update == NULL) break;
                        table->update(cell, cx, cy, x, y, table->payload);
                        break;
                    }
                }
            }
        }
        return;
    }
}

static void tsc_subtick_doMover(struct tsc_cell *cell, int x, int y, int _ux, int _uy, void *_) {
    tsc_grid_push(currentGrid, x, y, cell->rot, 0, NULL);
}

static void tsc_subtick_doGen(struct tsc_cell *cell, int x, int y, int _ux, int _uy, void *_) {
    int bx = tsc_grid_shiftX(x, cell->rot, -1);
    int by = tsc_grid_shiftY(y, cell->rot, -1);
    tsc_cell *back = tsc_grid_get(currentGrid, bx, by);
    if(back == NULL) return;
    if(!tsc_cell_canGenerate(currentGrid, back, bx, by, cell, x, y, cell->rot)) return;
    int fx = tsc_grid_frontX(x, cell->rot);
    int fy = tsc_grid_frontY(y, cell->rot);
    tsc_grid_push(currentGrid, fx, fy, cell->rot, 1, back);
}

static void tsc_subtick_doClockwiseRotator(struct tsc_cell *cell, int x, int y, int ux, int uy, void *_) {
    tsc_cell *toRot = tsc_grid_get(currentGrid, ux, uy);
    if(toRot == NULL) return;
    if(toRot->id == builtin.empty) return;
    toRot->rot++;
    toRot->rot %= 4;
}

static void tsc_subtick_doCounterClockwiseRotator(struct tsc_cell *cell, int x, int y, int ux, int uy, void *_) {
    tsc_cell *toRot = tsc_grid_get(currentGrid, ux, uy);
    if(toRot == NULL) return;
    if(toRot->id == builtin.empty) return;
    toRot->rot += 3;
    toRot->rot %= 4;
}

static void tsc_subtick_do(tsc_subtick_t *subtick) {
    char mode = subtick->mode;
    char parallel = subtick->parallel;
    char spacing = subtick->spacing;

    // If bad, blame Blendy
    char rots[] = {0, 2, 3, 1};
    char rotc = 4;

    if(mode == TSC_SUBMODE_TRACKED) {
        if(parallel) {
            tsc_updateinfo_t *buffer = subticks_getBuffer(currentGrid->width < currentGrid->height ? currentGrid->height : currentGrid->width);

            for(char i = 0; i < rotc; i++) {
                char rot = rots[i];
                if(rot % 2 == 1) {
                    for(size_t x = 0; x < currentGrid->width; x++) {
                        buffer[x].x = x;
                        buffer[x].rot = rot;
                        buffer[x].subtick = subtick;
                    }

                    workers_waitForTasksFlat(&tsc_subtick_worker, buffer, sizeof(tsc_updateinfo_t), currentGrid->width);
                } else {
                    for(size_t y = 0; y < currentGrid->height; y++) {
                        buffer[y].x = y;
                        buffer[y].rot = rot;
                        buffer[y].subtick = subtick;
                    }

                    workers_waitForTasksFlat(&tsc_subtick_worker, buffer, sizeof(tsc_updateinfo_t), currentGrid->height);
                }
            }
            return;
        }
        // TODO: Single-threaded tracked updates
    }
    
    if(mode == TSC_SUBMODE_NEIGHBOUR) {
        if(parallel) {
            // TODO: reuse buffers (possibly buffers stored in grid?)
            tsc_updateinfo_t *buffer = subticks_getBuffer(currentGrid->width);
            for(size_t x = 0; x < currentGrid->width; x++) {
                buffer[x].x = x;
                buffer[x].subtick = subtick;
            }

            workers_waitForTasksFlat(&tsc_subtick_worker, buffer, sizeof(tsc_updateinfo_t), currentGrid->width);
            return;
        }
        // Single-threaded
        for(int x = 0; x < currentGrid->width; x++) {
            for(int y = 0; y < currentGrid->height; y++) {
                int off[] = {
                    -1, 0,
                    1, 0,
                    0, -1,
                    0, 1
                };
                int offc = 4;
                for(int i = 0; i < offc; i++) {
                    int cx = x + off[i*2];
                    int cy = y + off[i*2+1];
                    tsc_cell *cell = tsc_grid_get(currentGrid, cx, cy);
                    if(cell == NULL) continue;
                    if(cell->updated) continue;
                    for(size_t i = 0; i < subtick->idc; i++) {
                        if(subtick->ids[i] == cell->id) {
                            tsc_celltable *table = tsc_cell_getTable(cell);
                            if(table == NULL) break;
                            if(table->update == NULL) break;
                            cell->updated = true;
                            table->update(cell, cx, cy, x, y, table->payload);
                            break;
                        }
                    }
                }
            }
        }
    }

    if(mode == TSC_SUBMODE_TICKED) {
        if(parallel) {
            tsc_updateinfo_t *buffer = subticks_getBuffer(currentGrid->width);
            for(size_t x = 0; x < currentGrid->width; x++) {
                buffer[x].x = x;
                buffer[x].subtick = subtick;
            }

            workers_waitForTasksFlat(&tsc_subtick_worker, buffer, sizeof(tsc_updateinfo_t), currentGrid->width);
            return;
        }
        // Single-threaded
        for(int x = 0; x < currentGrid->width; x++) {
            for(int y = 0; y < currentGrid->height; y++) {
                tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
                if(cell->updated) continue;
                for(size_t i = 0; i < subtick->idc; i++) {
                    if(subtick->ids[i] == cell->id) {
                        tsc_celltable *table = tsc_cell_getTable(cell);
                        if(table == NULL) break;
                        if(table->update == NULL) break;
                        cell->updated = true;
                        table->update(cell, x, y, x, y, table->payload);
                        break;
                    }
                }
            }
        }
    }
}

static void tsc_subtick_reset(void *data) {
    // Super smart optimization
    // If you don't understand, me neither
    size_t x = (size_t)data;

    for(size_t y = 0; y < currentGrid->height; y++) {
        tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
        cell->updated = false;
    }
}

void tsc_subtick_run() {
    // This absolutely evil hack will call the tsc_subtick_reset with a data pointer who's address is the x.
    // I don't care about how horribly unsafe this is or how much better this could be in Rust,
    // this is the fastest way to reset the grid in parallel.
    workers_waitForTasksFlat(&tsc_subtick_reset, 0, 1, currentGrid->width);

    for(size_t i = 0; i < subticks.subc; i++) {
        tsc_subtick_do(subticks.subs + i);
    }
}

void tsc_subtick_addCore() {
    tsc_celltable *mover = tsc_cell_newTable(builtin.mover);
    mover->update = &tsc_subtick_doMover;

    tsc_subtick_t moverSubtick;
    moverSubtick.priority = 3.0;
    moverSubtick.idc = 1;
    moverSubtick.ids = malloc(sizeof(const char *));
    moverSubtick.ids[0] = builtin.mover;
    moverSubtick.name = tsc_strintern("movers");
    moverSubtick.mode = TSC_SUBMODE_TRACKED;
    moverSubtick.spacing = 0;
    moverSubtick.parallel = true;
    tsc_subtick_add(moverSubtick);

    tsc_celltable *generator = tsc_cell_newTable(builtin.generator);
    generator->update = &tsc_subtick_doGen;

    tsc_subtick_t generatorSubtick;
    generatorSubtick.priority = 1.0;
    generatorSubtick.idc = 1;
    generatorSubtick.ids = malloc(sizeof(const char *));
    generatorSubtick.ids[0] = builtin.generator;
    generatorSubtick.name = tsc_strintern("generators");
    generatorSubtick.mode = TSC_SUBMODE_TRACKED;
    generatorSubtick.spacing = 0;
    generatorSubtick.parallel = true;
    tsc_subtick_add(generatorSubtick);

    tsc_celltable *rotcw = tsc_cell_newTable(builtin.rotator_cw);
    rotcw->update = &tsc_subtick_doClockwiseRotator;
    
    tsc_celltable *rotccw = tsc_cell_newTable(builtin.rotator_ccw);
    rotccw->update = &tsc_subtick_doCounterClockwiseRotator;

    tsc_subtick_t rotatorSubtick;
    rotatorSubtick.priority = 2.0;
    rotatorSubtick.idc = 2;
    rotatorSubtick.ids = malloc(sizeof(const char *) * rotatorSubtick.idc);
    rotatorSubtick.ids[0] = builtin.rotator_cw;
    rotatorSubtick.ids[1] = builtin.rotator_ccw;
    rotatorSubtick.name = tsc_strintern("rotators");
    rotatorSubtick.mode = TSC_SUBMODE_NEIGHBOUR;
    rotatorSubtick.spacing = 0;
    rotatorSubtick.parallel = true;
    tsc_subtick_add(rotatorSubtick);
}
