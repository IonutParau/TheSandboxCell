#include "grid.h"
#include "../utils.h"
#include "../api/api.h"
#include "../graphics/resources.h"
#include "../threads/workers.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

tsc_grid *currentGrid = NULL;
tsc_gridStorage *gridStorage = NULL;
size_t tsc_gridChunkSize = 25;

tsc_grid *tsc_getGrid(const char *name) {
    if(gridStorage == NULL) return NULL;
    for(int i = 0; i < gridStorage->len; i++) {
        if(tsc_streql(gridStorage->id[i], name)) {
            return gridStorage->grid[i];
        }
    }
    return NULL;
}
   
typedef struct tsc_grid_init_task_t {
    tsc_grid *grid;
    int y;
} tsc_grid_init_task_t;

static void tsc_initPartOfGrid(tsc_grid_init_task_t *task) {
    size_t off = task->grid->width * task->y;
    // literally just 3 memsets lmao
    memset(task->grid->cells + off, 0, sizeof(tsc_cell) * task->grid->width);
    memset(task->grid->bgs + off, 0, sizeof(tsc_cell) * task->grid->width);
    memset(task->grid->optData + (off) * tsc_optSize(), 0, tsc_optSize() * task->grid->width);
    for(size_t x = 0; x < task->grid->width; x += tsc_gridChunkSize) {
        tsc_grid_disableChunk(task->grid, x, task->y);
    }
}

tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description) {
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

    // that + is to fight integer division rounding
    int chunkWidth = width / tsc_gridChunkSize + 1;
    int chunkHeight = height / tsc_gridChunkSize + 1;

    grid->chunkwidth = chunkWidth;
    grid->chunkheight = chunkHeight;

    grid->chunkdata = malloc(sizeof(bool) * chunkWidth * chunkHeight);

    grid->refc = 1;
    size_t len = width * height;
    grid->cells = malloc(sizeof(tsc_cell) * len);
    grid->bgs = malloc(sizeof(tsc_cell) * len);
    grid->optData = malloc(sizeof(char) * len * tsc_optSize());

    if(len < 100000) {
        for(size_t i = 0; i < len; i++) {
            grid->cells[i] = tsc_cell_create(builtin.empty, 0);
            grid->bgs[i] = tsc_cell_create(builtin.empty, 0);
            memset(grid->optData + i * tsc_optSize(), 0, tsc_optSize());
            tsc_grid_disableChunk(grid, i % grid->width, i / grid->width);
        }
    } else {
        tsc_grid_init_task_t *bullshitTaskBuffer = malloc(sizeof(tsc_grid_init_task_t) * grid->height);
        for(size_t i = 0; i < grid->height; i++) {
            bullshitTaskBuffer[i].grid = grid;
            bullshitTaskBuffer[i].y = i;
        }
        workers_waitForTasksFlat((worker_task_t *)tsc_initPartOfGrid, bullshitTaskBuffer, sizeof(tsc_grid_init_task_t), grid->height);
        free(bullshitTaskBuffer);
    }

    return grid;
}

void tsc_retainGrid(tsc_grid *grid) {
    grid->refc++;
}

static void tsc_clearPartOfGrid(tsc_grid_init_task_t *task) {
    int y = task->y;
    for(int x = 0; x < task->grid->width; x++) {
        tsc_cell_destroy(*tsc_grid_get(task->grid, x, y));
        tsc_cell_destroy(*tsc_grid_background(task->grid, x, y));
    }
}

void tsc_deleteGrid(tsc_grid *grid) {
    grid->refc--;
    if(grid->refc > 0) return;
    // Title and description is interned
    tsc_grid_init_task_t *buffer = malloc(grid->height * sizeof(tsc_grid_init_task_t));
    for(int y = 0; y < grid->height; y++) {
        buffer[y].y = y;
        buffer[y].grid = grid;
    }

    workers_waitForTasksFlat((worker_task_t *)tsc_clearPartOfGrid, buffer, sizeof(tsc_grid_init_task_t), grid->height);

    free(buffer);

    free(grid->cells);
    free(grid->bgs);
    free(grid->chunkdata);
    free(grid);
}

void tsc_switchGrid(tsc_grid *grid) {
    if(currentGrid != NULL) {
        tsc_deleteGrid(currentGrid);
    }
    currentGrid = grid;
    tsc_retainGrid(currentGrid);
}

typedef struct tsc_grid_copy_task_t {
    tsc_grid *src;
    tsc_grid *dest;
    int y;
} tsc_grid_copy_task_t;

static void tsc_copyPartOfGrid(tsc_grid_copy_task_t *task) {
    int y = task->y;
    tsc_grid *src = task->src;
    tsc_grid *dest = task->dest;

    for(int x = 0; x < dest->width; x++) {
        dest->cells[x+y*dest->width] = tsc_cell_clone(tsc_grid_get(src, x, y));
        dest->bgs[x+y*dest->width] = tsc_cell_clone(tsc_grid_background(src, x, y));

        if(tsc_grid_checkChunk(src, x, y)) {
            tsc_grid_enableChunk(dest, x, y);
        }
    }
}

void tsc_copyGrid(tsc_grid *dest, tsc_grid *src) {
    tsc_clearGrid(dest, src->width, src->height);

    int height = dest->height;
    tsc_grid_copy_task_t *buffer = malloc(sizeof(tsc_grid_copy_task_t) * height);
    for(int y = 0; y < height; y++) {
        buffer[y].src = src;
        buffer[y].dest = dest;
        buffer[y].y = y;
    }

    workers_waitForTasksFlat((worker_task_t *)tsc_copyPartOfGrid, buffer, sizeof(tsc_grid_copy_task_t), height);

    free(buffer);
}

void tsc_clearGrid(tsc_grid *grid, int width, int height) {
    {
        // Delete old shit now
        tsc_grid_init_task_t *buffer = malloc(grid->height * sizeof(tsc_grid_init_task_t));
        for(int y = 0; y < grid->height; y++) {
            buffer[y].y = y;
            buffer[y].grid = grid;
        }

        workers_waitForTasksFlat((worker_task_t *)tsc_clearPartOfGrid, buffer, sizeof(tsc_grid_init_task_t), grid->height);

        free(buffer);
    }
    size_t len = width * height;
    grid->cells = realloc(grid->cells, sizeof(tsc_cell) * len);
    grid->bgs = realloc(grid->bgs, sizeof(tsc_cell) * len);
    int chunkWidth = width / tsc_gridChunkSize + 1;
    int chunkHeight = height / tsc_gridChunkSize + 1;
    grid->chunkdata = realloc(grid->chunkdata, sizeof(bool) * chunkWidth * chunkHeight);
    grid->chunkwidth = chunkWidth;
    grid->chunkheight = chunkHeight;
    grid->width = width;
    grid->height = height;
    grid->optData = realloc(grid->optData, sizeof(char) * width * height * tsc_optSize());
    if(len < 100000) {
        for(size_t i = 0; i < len; i++) {
            grid->cells[i] = tsc_cell_create(builtin.empty, 0);
            grid->bgs[i] = tsc_cell_create(builtin.empty, 0);
            memset(grid->optData + i * tsc_optSize(), 0, tsc_optSize());
            tsc_grid_disableChunk(grid, i % grid->width, i / grid->width);
        }
    } else {
        tsc_grid_init_task_t *bullshitTaskBuffer = malloc(sizeof(tsc_grid_init_task_t) * grid->height);
        for(size_t i = 0; i < grid->height; i++) {
            bullshitTaskBuffer[i].grid = grid;
            bullshitTaskBuffer[i].y = i;
        }
        workers_waitForTasksFlat((worker_task_t *)tsc_initPartOfGrid, bullshitTaskBuffer, sizeof(tsc_grid_init_task_t), grid->height);
        free(bullshitTaskBuffer);
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

size_t tsc_optLen = 0;
const char **tsc_optBuf = NULL;

size_t tsc_effectLen = 0;
const char **tsc_effectBuf = NULL;

size_t tsc_allocOptimization(const char *id) {
    const char *trueID = tsc_strintern(tsc_padWithModID(id));
    size_t idx = tsc_optLen++;
    tsc_optBuf = realloc(tsc_optBuf, sizeof(const char *) * tsc_optLen);
    tsc_optBuf[idx] = trueID;
    return idx;
}

size_t tsc_findOptimization(const char *trueID) {
    trueID = tsc_strintern(trueID);
    for(size_t i = 0; i < tsc_optLen; i++) {
        if(tsc_optBuf[i] == trueID) return i;
    }
    return 0;
}

size_t tsc_defineEffect(const char *id) {
    const char *trueID = tsc_strintern(tsc_padWithModID(id));
    size_t idx = tsc_effectLen++;
    tsc_effectBuf = realloc(tsc_effectBuf, sizeof(const char *) * tsc_effectLen);
    tsc_effectBuf[idx] = trueID;
    return idx;
}

size_t tsc_findEffect(const char *trueID) {
    trueID = tsc_strintern(trueID);
    for(size_t i = 0; i < tsc_effectLen; i++) {
        if(tsc_effectBuf[i] == trueID) return i;
    }
    return 0;
}

size_t tsc_optSize() {
    return tsc_optLen / 8 + (tsc_optLen % 8 == 0 ? 0 : 1);
}

size_t tsc_effectSize() {
    return tsc_effectLen / 8 + (tsc_effectLen % 8 == 0 ? 0 : 1);
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
#ifndef TSC_TURBO
    if(copy.lx == TSC_NULL_LAST) copy.lx = x;
    if(copy.ly == TSC_NULL_LAST) copy.ly = y;
#endif
    *old = copy;
    if(copy.id != builtin.empty) {
        tsc_grid_enableChunk(grid, x, y);
    }
}

tsc_cell *tsc_grid_background(tsc_grid *grid, int x, int y) {
    if(x < 0 || y < 0 || x >= grid->width || y >= grid->height) return NULL;
    return grid->bgs + (x + y * grid->width);
}

void tsc_grid_setBackground(tsc_grid *grid, int x, int y, tsc_cell *cell) {
    if(x < 0 || y < 0 || x >= grid->width || y >= grid->height) return;
    tsc_cell copy = tsc_cell_clone(cell);
    tsc_cell *old = tsc_grid_background(grid, x, y);
    tsc_cell_destroy(*old);
#ifndef TSC_TURBO
    if(copy.lx == TSC_NULL_LAST) copy.lx = x;
    if(copy.ly == TSC_NULL_LAST) copy.ly = y;
#endif
    *old = copy;
    if(copy.id != builtin.empty) {
        tsc_grid_enableChunk(grid, x, y);
    }
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

void tsc_grid_enableChunk(tsc_grid *grid, int x, int y) {
    if(tsc_grid_get(grid, x, y) == NULL) return;
    int cx = x / tsc_gridChunkSize;
    int cy = y / tsc_gridChunkSize;
    grid->chunkdata[cy * grid->chunkwidth + cx] = true;
}

void tsc_grid_disableChunk(tsc_grid *grid, int x, int y) {
    if(tsc_grid_get(grid, x, y) == NULL) return;
    int cx = x / tsc_gridChunkSize;
    int cy = y / tsc_gridChunkSize;
    grid->chunkdata[cy * grid->chunkwidth + cx] = false;
}

bool tsc_grid_checkChunk(tsc_grid *grid, int x, int y) {
    if(tsc_grid_get(grid, x, y) == NULL) return false;
    int cx = x / tsc_gridChunkSize;
    int cy = y / tsc_gridChunkSize;
    return grid->chunkdata[cy * grid->chunkwidth + cx];
}

bool tsc_grid_checkRow(tsc_grid *grid, int y) {
    for(int x = 0; x < grid->width; x += tsc_gridChunkSize) {
        if(tsc_grid_checkChunk(grid, x, y)) return true;
    }
    return false;
}

bool tsc_grid_checkColumn(tsc_grid *grid, int x) {
    for(int y = 0; y < grid->height; y += tsc_gridChunkSize) {
        if(tsc_grid_checkChunk(grid, x, y)) return true;
    }
    return false;
}

int __attribute__((optnone)) tsc_grid_chunkOff(int x, int off) {
    // int div handles it
    x -= x % tsc_gridChunkSize;
    int cx = x / tsc_gridChunkSize;
    cx += off;
    return cx * tsc_gridChunkSize;
}

bool tsc_grid_checkOptimization(tsc_grid *grid, int x, int y, size_t optimization) {
    if(tsc_grid_get(grid, x, y) == NULL) return false;
    if(optimization >= tsc_optLen) return false;
    size_t size = tsc_optSize();
    size_t i = x + y * grid->width;
    return tsc_getBit(grid->optData + i * size, optimization);
}

void tsc_grid_setOptimization(tsc_grid *grid, int x, int y, size_t optimization, bool enabled) {
    if(tsc_grid_get(grid, x, y) == NULL) return;
    if(optimization >= tsc_optLen) return;
    size_t size = tsc_optSize();
    size_t i = x + y * grid->width;
    tsc_setBit(grid->optData + i * size, optimization, enabled);
}

int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement) {
    // Beautiful hack
    tsc_cell empty = tsc_cell_create(builtin.empty, 0);
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
        force += tsc_cell_getBias(grid, cell, cx, cy, dir, "push", force);
        if(force <= 0) return 0;
        if(!tsc_cell_canMove(grid, cell, cx, cy, dir, "push", force)) return 0;
        if(tsc_cell_isAcid(grid, replacement, dir, "push", force, cell, cx, cy)) {
            mode = 1;
            break;
        }
        if(tsc_cell_isTrash(grid, cell, cx, cy, dir, "push", force, replacement)) {
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
        tsc_grid_enableChunk(grid, x, y);
        x = tsc_grid_frontX(x, dir);
        y = tsc_grid_frontY(y, dir);
    }

    if(mode == 1) {
        tsc_cell_onAcid(grid, &replacecell, dir, "push", force, tsc_grid_get(grid, x, y), x, y);
    }
    if(mode == 2) {
        tsc_cell_onTrash(grid, tsc_grid_get(grid, x, y), x, y, dir, "push", force, &replacecell);     
    }
    
    tsc_cell_destroy(replacecell);

    tsc_sound_play(builtin.audio.move);

    // +1 cuz replacement. This also means 0 only happens if pushing failed
    return amount + 1;
}

int tsc_grid_pull(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement) {
    int m = 0;

    while(true) {
        tsc_sound_play(builtin.audio.move);
        tsc_cell *current = tsc_grid_get(grid, x, y);
        if(current == NULL) return m;

        if(current->id == builtin.empty) {
            if(replacement != NULL) tsc_grid_set(grid, x, y, replacement);
            return m;
        }

        if(!tsc_cell_canMove(grid, current, x, y, dir, "pull", force)) {
            return m;
        }

        force += tsc_cell_getBias(grid, current, x, y, dir, "pull", force);

        int fx = tsc_grid_frontX(x, dir);
        int fy = tsc_grid_frontY(y, dir);
        tsc_cell *front = tsc_grid_get(grid, fx, fy);
        if(front == NULL) return m;

        if(tsc_cell_isTrash(grid, front, fx, fy, dir, "pull", force, current)) {
           tsc_cell_onTrash(grid, front, fx, fy, dir, "pull", force, current);
        } else if(tsc_cell_isAcid(grid, current, dir, "pull", force, front, fx, fy)) {
            tsc_cell_onAcid(grid, current, dir, "pull", force, front, fx, fy);
        } else if(front->id == builtin.empty) {
            tsc_grid_set(grid, fx, fy, current);
        } else {
            return m;
        }
        
        tsc_cell empty = tsc_cell_create(builtin.empty, 0);
        tsc_grid_set(grid, x, y, &empty);

        x = tsc_grid_shiftX(x, dir, -1);
        y = tsc_grid_shiftY(y, dir, -1);
        m++;
    }
}

int tsc_grid_grab(tsc_grid *grid, int x, int y, char dir, char side, double force, tsc_cell *replacement) {
    return 0;
}

bool tsc_grid_nudge(tsc_grid *grid, int x, int y, char dir, tsc_cell *replacement) {
    return false;
}
