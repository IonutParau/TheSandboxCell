#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cells/grid.h"
#include "cells/subticks.h"
#include "saving/saving.h"
#include "threads/workers.h"
#include <raylib.h>
#include "graphics/resources.h"
#include "graphics/rendering.h"
#include "utils.h"
#include "api/api.h"
#include "api/value.h"
#include "api/tscjson.h"

void doShit(void *thing) {
    int *num = thing;
    *num += 1;
}

int main(int argc, char **argv) {
    srand(time(NULL));

    // Suppress raylib debug messages
    SetTraceLogLevel(LOG_ERROR);

    workers_setupBest();

    tsc_init_builtin_ids();

    char *level = NULL;
    int width, height;
    width = height = 512;
    for(int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if(strncmp(arg, "--width=", 8) == 0) {
            char *svalue = arg + 8;
            width = atoi(svalue);
        } else if(strncmp(arg, "--height=", 9) == 0) {
            char *svalue = arg + 9;
            height = atoi(svalue);
        } else if(strncmp(arg, "--level=", 8) == 0) {
            level = arg + 8;
        }
    }
    
    tsc_subtick_addCore();
    tsc_saving_registerCore();
    tsc_loadDefaultCellBar();

    tsc_grid *grid = tsc_createGrid("main", width, height, NULL, NULL);
    tsc_switchGrid(grid);
    if(level != NULL) {
        tsc_saving_decodeWithAny(level, grid);
    }
    tsc_grid *initial = tsc_createGrid("initial", grid->width, grid->height, NULL, NULL);
    tsc_copyGrid(initial, grid);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "The Sandbox Cell");
    SetWindowState(FLAG_MSAA_4X_HINT);
    SetWindowMonitor(0);

    // L + ratio
    SetExitKey(KEY_NULL);

    InitAudioDevice();
   
    const char *defaultRP = tsc_strintern("default");
    tsc_createResourcePack(defaultRP);
    defaultResourcePack = tsc_getResourcePack(defaultRP);
    if(defaultResourcePack == NULL) {
        fprintf(stderr, "Default texture pack is missing.\n");
        return 1;
    }

    tsc_loadAllMods();
    
    tsc_enableResourcePack(defaultResourcePack);
    tsc_setupRendering();

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x171c1fFF));

        tsc_drawGrid();

        EndDrawing();

        tsc_handleRenderInputs();
        if(IsWindowFocused()) {
            SetMasterVolume(1.0);
        } else {
            SetMasterVolume(0.2);
        }
        tsc_sound_playQueue();
        // This handles all music stuff.
        tsc_music_playOrKeep();
    }

    CloseWindow();
    CloseAudioDevice();

    return EXIT_SUCCESS;
}
