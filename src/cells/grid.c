#include "grid.h"
#include "../utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

tsc_grid *currentGrid = NULL;
tsc_gridStorage *gridStorage = NULL;

tsc_grid *tsc_getGrid(const char *name) {
    if(gridStorage == NULL) return NULL;
    for(int i = 0; i < gridStorage->len; i++) {
        if(tsc_streql(gridStorage->id[i], name)) {
            return gridStorage->grid[i];
        }
    }
    return NULL;
}

tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description) {
    assert(width > 0);
    assert(height > 0);

    if(gridStorage == NULL) {
        gridStorage = malloc(sizeof(tsc_gridStorage));
        gridStorage->grid = NULL;
        gridStorage->id = NULL;
        gridStorage->len = 0;
    }
    tsc_grid *grid = malloc(sizeof(tsc_grid));
    size_t idx = gridStorage->len++;
    gridStorage->grid = realloc(gridStorage->grid, sizeof(tsc_grid *) * gridStorage->len);
    gridStorage->id = realloc(gridStorage->id, sizeof(const char *) * gridStorage->len);
    gridStorage->grid[idx] = grid;
    gridStorage->id[idx] = tsc_strintern(id);

    grid->width = width;
    grid->height = height;
    grid->title = tsc_strintern(title);
    grid->desc = tsc_strintern(description);
    grid->refc = 1;
    size_t len = width * height;
    grid->cells = malloc(sizeof(tsc_cell) * len);

    for(size_t i = 0; i < len; i++) {
        grid->cells[i] = tsc_cell_create(builtin.empty, 0);
    }

    return grid;
}

void tsc_retainGrid(tsc_grid *grid) {
    grid->refc++;
}

void tsc_deleteGrid(tsc_grid *grid) {
    grid->refc--;
    if(grid->refc > 0) return;
    // Title and description is interned
    for(size_t i = 0; i < grid->width*grid->height; i++) {
        tsc_cell_destroy(grid->cells[i]);
    }
    free(grid->cells);
    free(grid);
}

void tsc_switchGrid(tsc_grid *grid) {
    if(currentGrid != NULL) {
        tsc_deleteGrid(currentGrid);
    }
    currentGrid = grid;
    tsc_retainGrid(currentGrid);
}

void tsc_copyGrid(tsc_grid *dest, tsc_grid *src) {
    if(dest->width == src->width && dest->height == src->height) {
        for(size_t x = 0; x < src->width; x++) {
            for(size_t y = 0; y < src->height; y++) {
                tsc_grid_set(dest, x, y, tsc_grid_get(src, x, y));
            }
        }
        return;
    }
    // Kill grid
    for(size_t x = 0; x < dest->width; x++) {
        for(size_t y = 0; y < dest->height; y++) {
            tsc_cell_destroy(*tsc_grid_get(dest, x, y));
        }
    }
    // Resize and copy
    dest->width = src->width;
    dest->height = src->height;
    dest->cells = realloc(dest->cells, sizeof(tsc_cell) * dest->width * dest->height);
    for(size_t i = 0; i < dest->width * dest->height; i++) {
        dest->cells[i] = tsc_cell_clone(src->cells + i);
    }
}

void tsc_clearGrid(tsc_grid *grid, int width, int height) {
    for(int x = 0; x < grid->width; x++) {
        for(int y = 0; y < grid->height; y++) {
            tsc_cell_destroy(*tsc_grid_get(grid, x, y));
        }
    }
    size_t len = width * height;
    grid->cells = realloc(grid->cells, sizeof(tsc_cell) * len);
    grid->width = width;
    grid->height = height;
    for(size_t i = 0; i < len; i++) {
        grid->cells[i] = tsc_cell_create(builtin.empty, 0);
    }
}

void tsc_nukeGrids() {
    if(gridStorage == NULL) {
        return;
    }
    for(size_t i = 0; i < gridStorage->len; i++) {
        tsc_deleteGrid(gridStorage->grid[i]);
    }
    free(gridStorage->grid);
    free(gridStorage->id);
    free(gridStorage);
    gridStorage = NULL;
    tsc_deleteGrid(currentGrid);
    currentGrid = NULL;
}

tsc_cell *tsc_grid_get(tsc_grid *grid, int x, int y) {
    if(x < 0 || y < 0 || x >= grid->width || y >= grid->height) return NULL;
    return grid->cells + (x + y * grid->width);
}

void tsc_grid_set(tsc_grid *grid, int x, int y, tsc_cell *cell) {
    if(x < 0 || y < 0 || x >= grid->width || y >= grid->height) return;
    tsc_cell copy = tsc_cell_clone(cell);
    tsc_cell *old = tsc_grid_get(grid, x, y);
    tsc_cell_destroy(*old);
    *old = copy;
}

int tsc_grid_frontX(int x, char dir) {
    if(dir == 0) return x + 1;
    if(dir == 2) return x - 1;
    return x;
}

int tsc_grid_frontY(int y, char dir) {
    if(dir == 1) return y + 1;
    if(dir == 3) return y - 1;
    return y;
}

int tsc_grid_shiftX(int x, char dir, int amount) {
    if(dir == 0) return x + amount;
    if(dir == 2) return x - amount;
    return x;
}

int tsc_grid_shiftY(int y, char dir, int amount) {
    if(dir == 1) return y + amount;
    if(dir == 3) return y - amount;
    return y;
}

void tsc_grid_tick(tsc_grid *grid) {
    int rotc = 4;
    int rots[] = {2, 3, 0, 1};

    // Reset
    for(size_t x = 0; x < grid->width; x++) {
        for(size_t y = 0; y < grid->height; y++) {
            tsc_cell *cell = tsc_grid_get(grid, x, y);
            cell->updated = false;
            // Fix rotation in case something fucks it up
            while(cell->rot < 0) cell->rot += 4;
            cell->rot %= 4;
        }
    }
    
    // Generators
    for(int i = 0; i < rotc; i++) {
        for(size_t x = 0; x < grid->width; x++) {
            for(size_t y = 0; y < grid->height; y++) {
                tsc_cell *cell = tsc_grid_get(grid, x, y);
                if(cell->id == builtin.generator && cell->rot == rots[i] && !cell->updated) {
                    cell->updated = true;
                    int bx = tsc_grid_shiftX(x, cell->rot, -1);
                    int by = tsc_grid_shiftY(y, cell->rot, -1);
                    tsc_cell *back = tsc_grid_get(grid, bx, by);
                    if(back == NULL) continue;
                    if(!tsc_cell_canGenerate(grid, back, bx, by, cell, x, y, cell->rot)) continue;
                    int fx = tsc_grid_frontX(x, cell->rot);
                    int fy = tsc_grid_frontY(y, cell->rot);
                    tsc_grid_push(grid, fx, fy, cell->rot, 1, back);
                }
            }
        }
    }

    // Rotators
    for(size_t x = 0; x < grid->width; x++) {
        for(size_t y = 0; y < grid->height; y++) {
            tsc_cell *cell = tsc_grid_get(grid, x, y);
            if(cell->updated) continue;
            int amount = 0;
            if(cell->id == builtin.rotator_cw) amount = 1;
            if(cell->id == builtin.rotator_ccw) amount = 3;
            if(amount == 0) continue;

            tsc_cell *c;
#define DO_ROT(ox, oy) if((c = tsc_grid_get(grid, x + (ox), y + (oy))) != NULL) c->rot = (c->rot + amount) % 4;

            DO_ROT(1, 0);
            DO_ROT(-1, 0);
            DO_ROT(0, 1);
            DO_ROT(0, -1);

#undef DO_ROT

            cell->updated = true;
        }
    }
    
    // Movers 
    for(int i = 0; i < rotc; i++) {
        for(size_t x = 0; x < grid->width; x++) {
            for(size_t y = 0; y < grid->height; y++) {
                tsc_cell *cell = tsc_grid_get(grid, x, y);
                if(cell->id == builtin.mover && cell->rot == rots[i] && !cell->updated) {
                    cell->updated = true;
                    tsc_grid_push(grid, x, y, cell->rot, 0, NULL);
                }
            }
        }
    }
}

int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement) {
    // Beautiful hack
    tsc_cell empty = tsc_cell_create(NULL, 0);
    if(replacement == NULL) {
        empty.id = builtin.empty;
        replacement = &empty;
    }

    int amount = 0;

    int mode = 0;

    int cx = x;
    int cy = y;
    tsc_cell replacecell = tsc_cell_clone(replacement);
    while(true) {
        tsc_cell *cell = tsc_grid_get(grid, cx, cy);
        if(cell == NULL) return 0;
        if(cell->id == builtin.empty) {
            amount++;
            break;
        }
        force += tsc_cell_getBias(grid, cell, cx, cy, dir, "push");
        if(force <= 0) return 0;
        if(!tsc_cell_canMove(grid, cell, cx, cy, dir, "push")) return 0;
        if(tsc_cell_isAcid(grid, cell, cx, cy, dir, "push", replacement)) {
            mode = 1;
            break;
        }
        if(tsc_cell_isTrash(grid, cell, cx, cy, dir, "push", replacement)) {
            mode = 2;
            break;
        }
        replacement = cell;
        cx = tsc_grid_frontX(cx, dir);
        cy = tsc_grid_frontY(cy, dir);
        amount++;
    }

    // Move using tsc_cell_swap
    for(int i = 0; i < amount; i++) {
        tsc_cell_swap(tsc_grid_get(grid, x, y), &replacecell);
        x = tsc_grid_frontX(x, dir);
        y = tsc_grid_frontY(y, dir);
    }

    if(mode == 1) {
        tsc_cell_onAcid(grid, tsc_grid_get(grid, x, y), x, y, dir, "push", &replacecell);     
    }
    if(mode == 2) {
        tsc_cell_onTrash(grid, tsc_grid_get(grid, x, y), x, y, dir, "push", &replacecell);     
    }
    
    tsc_cell_destroy(replacecell);

    // +1 cuz replacement. This also means 0 only happens if pushing failed
    return amount + 1;
}
