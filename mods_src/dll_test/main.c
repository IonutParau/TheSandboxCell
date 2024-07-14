#include "libtsc.h"
#include <stdio.h>

typedef struct dll_mod_ids_t {
    const char *test;
} dll_mod_ids_t;

dll_mod_ids_t dllmod_ids;

int dllmod_test_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, void *_) {
    return cell->rot == dir;
}

void dllmod_init() {
    dllmod_ids.test = tsc_registerCell("test", "Test", "Just a debug cell.");
    printf("[ DLLMOD ] Loaded test cell as %s\n", dllmod_ids.test);
    // It does not need the actual ID.
    tsc_textures_load(defaultResourcePack, "test", "base.png");

    tsc_celltable *testTable = tsc_cell_newTable(dllmod_ids.test);
    testTable->canMove = dllmod_test_canMove;

    tsc_addCell(tsc_rootCategory(), dllmod_ids.test);
}
