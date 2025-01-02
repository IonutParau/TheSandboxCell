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
#include "graphics/resources.h"
#include "cells/ticking.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

#define TSC_MAINMENU_PARTICLE_COUNT 32768

typedef struct tsc_mainMenuParticle_t {
    const char *id;
    float r;
    float g;
    float dist;
    float angle;
    float rot;
} tsc_mainMenuParticle_t;

float tsc_randFloat() {
    return (float)rand() / RAND_MAX;
}

tsc_mainMenuParticle_t tsc_randomMainMenuParticle(bool respawn) {
    const char *builtinCells[] = {
        builtin.push, builtin.wall, builtin.enemy, builtin.mover,
        builtin.trash, builtin.slide, builtin.generator, builtin.rotator_cw,
        builtin.rotator_ccw,
    };
    size_t builtinCellCount = sizeof(builtinCells) / sizeof(const char *);

    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int r = (w < h ? w : h) / 4;
    int m = w > h ? w : h;

    tsc_mainMenuParticle_t particle;
    particle.id = builtinCells[rand() % builtinCellCount];
    particle.angle = tsc_randFloat() * 2 * PI;
    particle.r = tsc_randFloat() * (float)r / 10;
    particle.g = tsc_randFloat() * (r/particle.r) * 2;
    particle.dist = r + tsc_randFloat() * r * 10;
    if(respawn) {
        particle.dist = m + tsc_randFloat() * r * 3;
    }
    particle.rot += tsc_randFloat() * 2 * PI;
    return particle;
}

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

    tsc_loadSettings();

    if(tsc_toBoolean(tsc_getSetting(builtin.settings.fullscreen))) {
        tsc_settingHandler(builtin.settings.fullscreen);
    }

    tsc_loadAllMods();
    
    tsc_enableResourcePack(defaultResourcePack);
    tsc_setupRendering();

    tsc_mainMenuBtn.play = tsc_ui_newButtonState();
    tsc_mainMenuBtn.quit = tsc_ui_newButtonState();
    tsc_mainMenuBtn.manual = tsc_ui_newButtonState();
    tsc_mainMenuBtn.settings = tsc_ui_newButtonState();
    tsc_mainMenuBtn.credits = tsc_ui_newButtonState();

    int off = 0;
    
    tsc_mainMenuParticle_t mainMenuParticles[TSC_MAINMENU_PARTICLE_COUNT];
    for(size_t i = 0; i < TSC_MAINMENU_PARTICLE_COUNT; i++) {
        mainMenuParticles[i] = tsc_randomMainMenuParticle(false);
    }

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(0x171c1fFF));

        int width = GetScreenWidth();
        int height = GetScreenHeight();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_drawGrid();
        } else {
            // Super epic background
            int r = (width < height ? width : height) / 4;
            int bx = width/2;
            int by = height/2;
            for(size_t i = 0; i < TSC_MAINMENU_PARTICLE_COUNT; i++) {
                tsc_mainMenuParticle_t particle = mainMenuParticles[i];
                Texture t = textures_get(particle.id);
                Vector2 pos = {
                    bx + cos(particle.angle) * particle.dist,
                    by + sin(particle.angle) * particle.dist,
                };
                float scale = particle.r/t.width;
                Color c = WHITE;
                float x = r/particle.dist;
                c.a = x > 1 ? 255 : x * 255;
                Vector2 origin = {particle.r/2, particle.r/2};
                DrawTexturePro(t,
                        (Rectangle) {0, 0, t.width, t.height},
                        (Rectangle) {pos.x + origin.x, pos.y + origin.y, particle.r, particle.r},
                        origin, particle.rot * 180 / PI, c
                    );
            }
            DrawCircle(bx, by, r, BLACK); // BLACK HOLE
        }

        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_pushFrame(tsc_mainMenu);
            int textHeight = 100;
            tsc_ui_text("The Sandbox Cell v0.0.1", 50, WHITE);
            tsc_ui_pad(20, 20);
            tsc_ui_align(0.5, 0, width, textHeight);
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
        if(tsc_streql(tsc_currentMenu, "settings")) {
            GuiEnable();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
                tsc_resetRendering();
            }
            size_t titleSize = 128;
            size_t settingSize = 64;
            size_t settingSpacing = 20;
            size_t settingVertOff = 70;
            size_t settingDeadSpace = 50;
            BeginScissorMode(0, settingVertOff, width, height - settingVertOff);
            size_t curY = settingVertOff;
            Font font = GetFontDefault();
            for(size_t i = 0; i < tsc_settingLen; i++) {
                tsc_settingCategory cat = tsc_settingCategories[i];
                GuiSetStyle(DEFAULT, TEXT_SIZE, titleSize);
                int titleWidth = MeasureText(cat.title, titleSize);
                GuiLabel((Rectangle) {settingDeadSpace, curY - off, titleWidth, titleSize}, cat.title);
                curY += titleSize + settingSpacing;
                for(size_t j = 0; j < cat.settinglen; j++) {
                    tsc_setting setting = cat.settings[j];
                    int settingWidth = MeasureText(setting.name, settingSize);
                    GuiSetStyle(DEFAULT, TEXT_SIZE, settingSize);
                    if(setting.kind == TSC_SETTING_TOGGLE) {
                        bool b = tsc_toBoolean(tsc_getSetting(setting.id));
                        bool old = b;
                        GuiCheckBox((Rectangle) {settingDeadSpace, curY - off, settingSize, settingSize}, setting.name, &b);
                        if(old != b) {
                            tsc_setSetting(setting.id, tsc_boolean(b));
                            if(setting.callback != NULL) {
                                setting.callback(setting.id);
                            }
                        }
                    } else if(setting.kind == TSC_SETTING_SLIDER) {
                        float v = tsc_toNumber(tsc_getSetting(setting.id));
                        float old = v;
                        int sliderWidth = width/4;
                        GuiSlider(
                            (Rectangle) {settingDeadSpace + settingWidth, curY - off, settingWidth + sliderWidth, settingSize},
                            setting.name, NULL, &v, setting.slider.min, setting.slider.max);
                        if(old != v) {
                            tsc_setSetting(setting.id, tsc_number(v));
                            if(setting.callback != NULL) {
                                setting.callback(setting.id);
                            }
                        }
                    } else if(setting.kind == TSC_SETTING_INPUT) {
                        int textWidth = width/4;
                        GuiLabel((Rectangle) {settingDeadSpace, curY - off, settingWidth, settingSize}, setting.name);
                        Rectangle textbox = {
                            settingDeadSpace + settingWidth + 10, curY - off,
                            settingWidth + textWidth, settingSize,
                        };
                        if(GuiTextBox(textbox, setting.string.buffer, setting.string.bufferlen, setting.string.selected)) {
                            cat.settings[j].string.selected = !setting.string.selected;
                            tsc_value s = tsc_string(setting.string.buffer);
                            tsc_setSetting(setting.id, s);
                            tsc_destroy(s);
                            if(setting.callback != NULL) {
                                setting.callback(setting.id);
                            }
                        }
                        if(setting.string.charset != NULL) {
                            size_t l = 0;
                            for(size_t i = 0; setting.string.buffer[i] != '\0'; i++) {
                                bool contained = false;
                                for(size_t j = 0; setting.string.charset[j] != '\0'; j++) {
                                    if(setting.string.charset[j] == setting.string.buffer[i]) contained = true;
                                }
                                if(contained) {
                                   setting.string.buffer[l] = setting.string.buffer[i];
                                   l++;
                                }
                            }
                            setting.string.buffer[l] = '\0';
                        }
                    } else {
                        GuiLabel((Rectangle) {settingDeadSpace, curY - off, settingWidth, settingSize}, setting.name);
                    }
                    curY += settingSize + settingSpacing;
                }
            }
            EndScissorMode();
            size_t maxSize = curY;
            if(maxSize > height) {
                off = GuiScrollBar((Rectangle) {width-20, 0, 20, height}, off, 0, maxSize - height);
            }
        }

        EndDrawing();

        double delta = GetFrameTime();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_handleRenderInputs();
        } else {
            int r = (width < height ? width : height) / 4;
            int bx = width/2;
            int by = height/2;
            for(size_t i = 0; i < TSC_MAINMENU_PARTICLE_COUNT; i++) {
                tsc_mainMenuParticle_t particle = mainMenuParticles[i];
                Texture t = textures_get(particle.id);
                Vector2 pos = {
                    bx + cos(particle.angle) * particle.dist,
                    by + sin(particle.angle) * particle.dist,
                };
                float x = 0.5*r/(particle.dist-r);
                float y = x * sqrt(x);
                particle.dist -= particle.g * delta;
                particle.angle += y * delta;
                particle.rot += y * 2 * PI * delta;
                if(particle.dist < (float)r/2) {
                    particle = tsc_randomMainMenuParticle(true);
                }
                mainMenuParticles[i] = particle;
            }
        }
        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_bringBackFrame(tsc_mainMenu);
            tsc_ui_update(delta);
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.play) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "game";
                tsc_resetRendering();
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.settings) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "settings";
                tsc_resetRendering();
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.manual) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "manual";
                tsc_resetRendering();
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.credits) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "credits";
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
