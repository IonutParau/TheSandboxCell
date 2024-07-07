#include <stdio.h>
#include <stdlib.h>
#include "cells/grid.h"
#include "cells/subticks.h"
#include "saving/saving.h"
#include "threads/workers.h"
#include <raylib.h>
#include "graphics/resources.h"
#include "graphics/rendering.h"
#include "utils.h"

void doShit(void *thing) {
    int *num = thing;
    *num += 1;
}

int main() {
    // Suppress raylib debug messages
    SetTraceLogLevel(LOG_ERROR);

    workers_setupBest();

    tsc_init_builtin_ids();

    tsc_grid *grid = tsc_createGrid("main", 100, 100, NULL, NULL);
    tsc_switchGrid(grid);
    tsc_grid *initial = tsc_createGrid("initial", grid->width, grid->height, NULL, NULL);
    tsc_copyGrid(initial, grid);

    tsc_cell mover = tsc_cell_create(builtin.mover, 0);
    tsc_grid_set(grid, 0, 0, &mover);
    tsc_cell_destroy(mover);

    tsc_subtick_addCore();
    tsc_saving_registerCore();
    
    InitWindow(800, 600, "The Sandbox Cell");
    SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetWindowMonitor(0);
    SetWindowIcon(LoadImage("textures/base.png"));
   
    const char *defaultRP = tsc_strintern("default");
    tsc_createResourcePack(defaultRP);
    defaultResourcePack = tsc_getResourcePack(defaultRP);
    if(defaultResourcePack == NULL) {
        fprintf(stderr, "Default texture pack is missing.\n");
        return 1;
    }
    tsc_enableResourcePack(defaultResourcePack);
    tsc_setupRendering();

    SetTargetFPS(60);

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x171c1fFF));

        tsc_drawGrid();

        EndDrawing();

        tsc_handleRenderInputs();
    }

    CloseWindow();

    return EXIT_SUCCESS;
}
