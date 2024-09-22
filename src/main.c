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
#include "graphics/ui.h"
#include "cells/ticking.h"

void doShit(void *thing) {
    int *num = thing;
    *num += 1;
}

ui_frame *tsc_mainMenu;
ui_frame *tsc_settingsMenu;

typedef struct tsc_mainMenuBtn_t {
    ui_button *play;
    ui_button *settings;
    ui_button *credits;
    ui_button *manual;
    ui_button *quit;
} tsc_mainMenuBtn_t;

tsc_mainMenuBtn_t tsc_mainMenuBtn;

int main(int argc, char **argv) {
    srand(time(NULL));

    // Suppress raylib debug messages
    //SetTraceLogLevel(LOG_ERROR);

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
        } else if(strcmp(arg, "--mtpf") == 0) {
            multiTickPerFrame = true;
        } else if(strcmp(arg, "--no-mtpf") == 0) {
            multiTickPerFrame = false;
        } else if(strncmp(arg, "--tickDelay=", 12) == 0) {
            tickDelay = atof(arg + 12);
        }
    }
    
    tsc_subtick_addCore();
    tsc_saving_registerCore();
    tsc_loadDefaultCellBar();

    tsc_mainMenu = tsc_ui_newFrame();
    tsc_settingsMenu = tsc_ui_newFrame();

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

    tsc_mainMenuBtn.play = tsc_ui_newButtonState();
    tsc_mainMenuBtn.quit = tsc_ui_newButtonState();
    tsc_mainMenuBtn.manual = tsc_ui_newButtonState();
    tsc_mainMenuBtn.settings = tsc_ui_newButtonState();
    tsc_mainMenuBtn.credits = tsc_ui_newButtonState();

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x171c1fFF));

        int width = GetScreenWidth();
        int height = GetScreenHeight();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_drawGrid();
        }
        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_pushFrame(tsc_mainMenu);
            int textHeight = 100;
            tsc_ui_text("The Sandbox Cell v0.0.1", 50, WHITE);
            tsc_ui_pad(20, 20);
            tsc_ui_align(0.5, 0, width, textHeight);
            tsc_ui_space(width/4);
            tsc_ui_box(GRAY);
            tsc_ui_align(0.5, 0.5, width, height);
            Color buttonHoverColor = GetColor(0x3275A8FF);
            tsc_ui_row({
                tsc_ui_text("Play", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.play) == UI_BUTTON_HOVER) {
                    tsc_ui_box(buttonHoverColor);
                }
                tsc_ui_text("Settings", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.settings) == UI_BUTTON_HOVER) {
                    tsc_ui_box(buttonHoverColor);
                }
                tsc_ui_text("Manual", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.manual) == UI_BUTTON_HOVER) {
                    tsc_ui_box(buttonHoverColor);
                }
                tsc_ui_text("Credits", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.credits) == UI_BUTTON_HOVER) {
                    tsc_ui_box(buttonHoverColor);
                }
                tsc_ui_text("Quit", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.quit) == UI_BUTTON_HOVER) {
                    tsc_ui_box(buttonHoverColor);
                }
            });
            tsc_ui_pad(20, textHeight+20);
            tsc_ui_align(0.5, 0, width, 0);
            tsc_ui_render();
            tsc_ui_popFrame();
        }

        EndDrawing();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_handleRenderInputs();
        }
        double delta = GetFrameTime();
        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_bringBackFrame(tsc_mainMenu);
            tsc_ui_update(delta);
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.play) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "game";
                tsc_resetRendering();
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.quit) == UI_BUTTON_PRESS) {
                if(rand() % 50) {
                    break;
                }
                tsc_ui_checkbutton(NULL); // segfault hehe
            }
            tsc_ui_popFrame();
        }

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
