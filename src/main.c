#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "api/tscjson.h"
#include "api/value.h"
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
#include "graphics/nui.h"
#include "graphics/resources.h"
#include "cells/ticking.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// dont worry about this this would never fuck shit up
void *rp_resourceTableGet(tsc_resourcetable *table, const char *id); 

ui_frame *tsc_mainMenu;
ui_frame *tsc_creditsMenu;

typedef struct tsc_mainMenuBtn_t {
    ui_button *play;
    ui_button *settings;
    ui_button *credits;
    ui_button *texturepacks;
    ui_button *quit;
} tsc_mainMenuBtn_t;

tsc_mainMenuBtn_t tsc_mainMenuBtn;

#define TSC_MAINMENU_PARTICLE_COUNT 8192

typedef struct tsc_mainMenuParticle_t {
    tsc_id_t id;
    float r;
    float g;
    float dist;
    float angle;
    float rot;
} tsc_mainMenuParticle_t;

float tsc_randFloat() {
    return (float)rand() / (float)RAND_MAX;
}

tsc_id_t *tsc_main_allCells = NULL;
size_t tsc_main_cellCount = 0;

tsc_mainMenuParticle_t tsc_randomMainMenuParticle(bool respawn) {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    int r = (w < h ? w : h) / 4;
    int m = w > h ? w : h;

    tsc_mainMenuParticle_t particle;
    particle.id = tsc_main_allCells[rand() % tsc_main_cellCount];
    particle.angle = tsc_randFloat() * 2 * PI;
    particle.r = tsc_randFloat() * (float)r / 10;
    particle.g = tsc_randFloat() * (r/particle.r) * 4;
    particle.dist = r + tsc_randFloat() * r * 4;
    if(respawn) {
        particle.dist = m + tsc_randFloat() * r;
    }
    particle.rot = tsc_randFloat() * 2 * PI;
    return particle;
}

float tsc_magicMusicSampleDoNotTouchEver = 0;

void tsc_magicStreamProcessorDoNotUseEver(void *buffer, unsigned int sampleCount) {
    if(tsc_streql(tsc_currentMenu, "game")) return;
    float *samples = buffer;
    if(sampleCount > 0) {
        tsc_magicMusicSampleDoNotTouchEver = 0;
        for(size_t i = 0; i < sampleCount; i++) {
            tsc_magicMusicSampleDoNotTouchEver += fabs(samples[i]);
        }
        tsc_magicMusicSampleDoNotTouchEver /= sampleCount;
    }
}

void tsc_saveEnabledPacks() {
    size_t len = 0;
    while(tsc_indexEnabledResourcePack(len) != NULL) len++;
    tsc_value theList = tsc_array(len);
    for(size_t i = 0; i < len; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(i);
        tsc_value s = tsc_string(pack->id);
        tsc_setIndex(theList, i, s);
        tsc_destroy(s);
    }

    char enabledPath[] = "data/enabled.json";
    tsc_pathfix(enabledPath);
    FILE *f = fopen(enabledPath, "w");
    tsc_buffer buffer = tsc_json_encode(theList, NULL, 0, false);

    fwrite(buffer.mem, sizeof(char), buffer.len, f);

    tsc_saving_deleteBuffer(buffer);
    tsc_destroy(theList);
    fclose(f);
}

void tsc_loadEnabledPacks() {
    char enabledPath[] = "data/enabled.json";
    tsc_pathfix(enabledPath);
    if(!tsc_hasfile(enabledPath)) return;
    char *data = tsc_allocfile(enabledPath, NULL);
    tsc_json_error_t err;
    tsc_value l = tsc_json_decode(data, &err); // if this fails u are an idiot
    if(err.status != TSC_JSON_ERROR_SUCCESS) {
        fprintf(stderr, "ERROR: %s: byte %d\n", tsc_json_error[err.status], err.index);
        exit(1);
    }

    for(size_t i = 0; i < tsc_getLength(l); i++) {
        const char *s = tsc_toString(tsc_index(l, i));

        tsc_resourcepack *pack = tsc_getResourcePack(s);
        for(size_t j = 0; tsc_indexEnabledResourcePack(j) != NULL; j++) {
            if(tsc_indexEnabledResourcePack(j) == pack) {
                pack = NULL;
                break;
            }
        }
        // if NULL, then the user deleted the enabled pack or we got duplicates
        if(pack != NULL) {
            fprintf(stdout, "enabled %s\n", s);
            tsc_enableResourcePack(pack);
        }
    }

    free(data);
}

#define TSC_GRID_SIZE_BUFSIZE 16

int main(int argc, char **argv) {
    srand(time(NULL));

    // Suppress raylib debug messages
    SetTraceLogLevel(LOG_ERROR);

    workers_setupBest();

    tsc_init_builtin_ids();

    char *level = NULL;
    char gridWidth[TSC_GRID_SIZE_BUFSIZE];
    char gridHeight[TSC_GRID_SIZE_BUFSIZE];
    bool gridWidthSel, gridHeightSel = false;
    strcpy(gridWidth, "100");
    strcpy(gridHeight, "100");
    for(int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if(strncmp(arg, "--width=", 8) == 0) {
            char *svalue = arg + 8;
            strncpy(gridWidth, svalue, TSC_GRID_SIZE_BUFSIZE-1);
        } else if(strncmp(arg, "--height=", 9) == 0) {
            char *svalue = arg + 9;
            strncpy(gridHeight, svalue, TSC_GRID_SIZE_BUFSIZE-1);
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
    tsc_creditsMenu = tsc_ui_newFrame();
    ui_frame *debugFrame = tsc_ui_newFrame();


    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "The Sandbox Cell");
    SetWindowMonitor(0);
    SetWindowState(FLAG_MSAA_4X_HINT | FLAG_WINDOW_MAXIMIZED);

    volatile double timeElapsed = 0;

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

    size_t allPackC = 0;
    char buffer[] = "data/resources";
    tsc_pathfix(buffer);
    char **allPacks = tsc_dirfiles(buffer, &allPackC);
    for(size_t i = 0; i < allPackC; i++) {
        if(!tsc_streql(allPacks[i], "default")) {
            tsc_createResourcePack(tsc_strintern(allPacks[i]));
        }
    }
    tsc_freedirfiles(allPacks);

    tsc_loadSettings();

    if(tsc_toBoolean(tsc_getSetting(builtin.settings.fullscreen))) {
        tsc_settingHandler(builtin.settings.fullscreen);
    }

    tsc_loadAllMods();
    
    tsc_enableResourcePack(defaultResourcePack);
    tsc_loadEnabledPacks();
    tsc_setupRendering();

    tsc_mainMenuBtn.play = tsc_ui_newButtonState();
    tsc_mainMenuBtn.quit = tsc_ui_newButtonState();
    tsc_mainMenuBtn.texturepacks = tsc_ui_newButtonState();
    tsc_mainMenuBtn.settings = tsc_ui_newButtonState();
    tsc_mainMenuBtn.credits = tsc_ui_newButtonState();

    int off = 0;
    
    tsc_mainMenuParticle_t mainMenuParticles[TSC_MAINMENU_PARTICLE_COUNT];
    volatile size_t mainMenuParticleCount = 0;
    bool particlesInitialized = false;

    AttachAudioMixedProcessor(tsc_magicStreamProcessorDoNotUseEver);

    double particleHalvingTimer = 0;

    double blackHoleSoundBonus = 0;

    tsc_addCoreSplashes();
    const char *splash = tsc_randomSplash();

    char *creditsContent = tsc_hasfile("CREDITS.txt") ? tsc_allocfile("CREDITS.txt", NULL) : "No CREDITS.txt";
    int creditsLineCount = 1;
    for(size_t i = 0; creditsContent[i] != '\0'; i++) {
        if(creditsContent[i] == '\n') creditsLineCount++;
    }
    const char **creditsLines = malloc(sizeof(const char *) * creditsLineCount);
    const char *creditsLineStart = creditsContent;
    {
        size_t i = 0;
        size_t j = 0;
        while(true) {
            if(creditsContent[i] == '\0') {
                creditsLines[j] = creditsLineStart;
                break;
            }
            if(creditsContent[i] == '\n') {
                creditsContent[i] = '\0';
                creditsLines[j] = creditsLineStart;
                creditsLineStart = creditsContent + i + 1;
                j++;
            }
            i++;
        }
    }

    tsc_main_cellCount = tsc_countCells();
    tsc_main_allCells = malloc(sizeof(const char *) * tsc_main_cellCount);
    tsc_fillCells(tsc_main_allCells);
    // Some cells are bad
    {
        size_t usefulLen = 0;
        for(size_t i = 0; i < tsc_main_cellCount; i++) {
            bool keep = true;
            tsc_id_t id = tsc_main_allCells[i];
            if(id == builtin.empty) keep = false;
            tsc_cell tmp = tsc_cell_create(id, 0);
            size_t flags = tsc_cell_getTableFlags(&tmp);
            if(flags & TSC_FLAGS_PLACEABLE) keep = false;
            if(keep) {
                tsc_main_allCells[usefulLen] = id;
                usefulLen++;
            }
        }
        tsc_main_cellCount = usefulLen;
    }

    float benchTime = 5;
    float benchNum = 0;
    size_t *benchSamples = NULL;
    size_t benchSampleCount = 0;

    typedef struct tsc_benchmark {
        const char *name;
        const char *level;
    } tsc_benchmark;

    size_t benchmarkCount;
    char benchpath[] = "data/benches.txt";
    tsc_pathfix(benchpath);
    char *benchmarkText = tsc_allocfile(benchpath, NULL);
    char **benchmarkLines = tsc_alloclines(benchmarkText, &benchmarkCount);

    tsc_benchmark *benchmarks = calloc(benchmarkCount, sizeof(tsc_benchmark));
    for(size_t i = 0; i < benchmarkCount; i++) {
        const char *line = benchmarkLines[i];
        size_t nameLen = memchr(line, ';', strlen(line)) - (void *)line;
        char *name = malloc(nameLen + 1);
        memcpy(name, line, nameLen);
        name[nameLen] = 0;
        benchmarks[i].name = name;
        benchmarks[i].level = tsc_strdup(line + nameLen + 1);
    }

    tsc_freefile(benchmarkText);
    tsc_freelines(benchmarkLines);

    tsc_nui_frame *testFrame = tsc_nui_newFrame();
    tsc_nui_buttonState *backToMainMenu = tsc_nui_newButton();

    while(!WindowShouldClose()) {
        tsc_areset(&tsc_tmp);
        // mustn't prealloc more than 4MB
        if(tsc_acount(&tsc_tmp) >= 4*1024*1024) {
            tsc_aclear(&tsc_tmp); // TODO: keep some of the older chunks
        }
        
        BeginDrawing();
        ClearBackground(GetColor(tsc_queryOptionalColor("bgColor", 0x171c1fFF)));

        int width = GetScreenWidth();
        int height = GetScreenHeight();
        
        timeElapsed += GetFrameTime();

        if(!particlesInitialized && timeElapsed > 0.2) {
            particlesInitialized = true;
            mainMenuParticleCount = TSC_MAINMENU_PARTICLE_COUNT;
            for(size_t i = 0; i < mainMenuParticleCount; i++) {
                mainMenuParticles[i] = tsc_randomMainMenuParticle(false);
                mainMenuParticles[i].id = tsc_main_allCells[(i / (mainMenuParticleCount / tsc_main_cellCount)) % tsc_main_cellCount];
            }
        }
        blackHoleSoundBonus = tsc_magicMusicSampleDoNotTouchEver;

        particleHalvingTimer -= GetFrameTime();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_drawGrid();
        } else {
            // Dont worry potato PCs we got you... though credits will kill you
            if(GetFPS() < 30 && timeElapsed > 0.5 && particleHalvingTimer <= 0) {
                particleHalvingTimer = 0.5;
                mainMenuParticleCount /= 2;
                for(size_t i = 0; i < mainMenuParticleCount; i++) {
                    double f = (double)i / ((double)mainMenuParticleCount / (double)tsc_main_cellCount);
                    mainMenuParticles[i].id = tsc_main_allCells[(size_t)(f * tsc_main_cellCount) % tsc_main_cellCount];
                }
            }
            // Super epic background
            int r = (width < height ? width : height) / 4;
            int bx = width/2;
            int by = height/2;
            for(size_t i = 0; i < mainMenuParticleCount; i++) {
                tsc_mainMenuParticle_t particle = mainMenuParticles[i];
                Texture t = textures_get(tsc_idToString(particle.id));
                Vector2 pos = {
                    bx + cos(particle.angle) * particle.dist,
                    by + sin(particle.angle) * particle.dist,
                };
                float x = r/particle.dist;
                Color c = WHITE;
                if(x > 1) x = 1;
                c.a = x * 255;
                Vector2 origin = {particle.r/2, particle.r/2};
                DrawTexturePro(t,
                    (Rectangle) {0, 0, t.width, t.height},
                    (Rectangle) {pos.x, pos.y, particle.r, particle.r},
                   origin, particle.rot * 180 / PI, c
                );
            }
            float blackHoleExtra = blackHoleSoundBonus;
            float blackHoleLimit = 1;
            if(blackHoleExtra > blackHoleLimit) blackHoleExtra = blackHoleLimit;
            DrawCircle(bx, by, r * (1 + blackHoleExtra), GetColor(tsc_queryOptionalColor("bhColor", 0x0b0c0dFF))); // BLACK HOLE
        }

        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_pushFrame(tsc_mainMenu);
            int textHeight = 100;
            tsc_ui_row({
                tsc_ui_text("The Sandbox Cell v0.1.0", 50, WHITE);
                #ifdef TSC_TURBO
                tsc_ui_text("turbo", 20, BLUE);
                #endif
            });
            tsc_ui_pad(20, 20);
            tsc_ui_align(0.5, 0, width, textHeight);
            tsc_ui_text(splash, 25, WHITE);
            tsc_ui_pad(10, 70);
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
                tsc_ui_text("Texture Packs", 20, WHITE);
                tsc_ui_pad(10, 10);
                if(tsc_ui_button(tsc_mainMenuBtn.texturepacks) == UI_BUTTON_HOVER) {
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
        } else {
            splash = tsc_randomSplash();
        }
        if(tsc_streql(tsc_currentMenu, "settings")) {
            GuiEnable();
            GuiLoadStyleDefault();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
            }
            size_t titleSize = 128;
            size_t settingSize = 64;
            size_t settingSpacing = 20;
            size_t settingVertOff = 70;
            size_t settingDeadSpace = 50;
            BeginScissorMode(0, settingVertOff, width, height - settingVertOff);
            size_t curY = settingVertOff;
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
                        char *theValue = NULL;
                        asprintf(&theValue, "%.2f", v);
                        GuiSlider(
                            (Rectangle) {settingDeadSpace + settingWidth, curY - off, settingWidth + sliderWidth, settingSize},
                            setting.name, theValue, &v, setting.slider.min, setting.slider.max);
                        v *= 100;
                        v = (int)v;
                        v /= 100;
                        if(old != v) {
                            tsc_setSetting(setting.id, tsc_number(v));
                            if(setting.callback != NULL) {
                                setting.callback(setting.id);
                            }
                        }
                        free(theValue);
                    } else if(setting.kind == TSC_SETTING_INPUT) {
                        int textWidth = width/4;
                        GuiLabel((Rectangle) {settingDeadSpace, curY - off, settingWidth, settingSize}, setting.name);
                        Rectangle textbox = {
                            settingDeadSpace + settingWidth + 10, curY - off,
                            settingWidth + textWidth, settingSize,
                        };
                        int preferredLeftBack = width/10;
                        if(textbox.x + textbox.width + preferredLeftBack > width) {
                            textbox.width = width - preferredLeftBack - textbox.x;
                        }
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

        if(tsc_streql(tsc_currentMenu, "texturepacks")) {
            GuiEnable();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
            }
            float offset = 40;
            float texturePackTitleSize = 100;
            float texturePackAuthorSize = 30;
            const char *defaultTexturePackName = "(Bugged)";
            const char *defaultTexturePackAuthor = "Unknown";

            float curY = 90;
            for(size_t i = 0; tsc_indexResourcePack(i) != NULL; i++) {
                tsc_resourcepack *pack = tsc_indexResourcePack(i);
                bool enabled = false;
                for(size_t j = 0; tsc_indexEnabledResourcePack(j) != NULL; j++) {
                    if(tsc_indexEnabledResourcePack(j) == pack) {
                        enabled = true;
                        break;
                    }
                }
                const char *name = pack->name == NULL ? defaultTexturePackName : pack->name;
                const char *authorSuffix = TextFormat("by %s", pack->author == NULL ? defaultTexturePackAuthor : pack->author);
                int nameWidth = MeasureText(name, texturePackTitleSize);
                GuiSetStyle(DEFAULT, TEXT_SIZE, texturePackTitleSize);
                if(GuiCheckBox((Rectangle) {offset + texturePackTitleSize, curY, texturePackTitleSize, texturePackTitleSize}, name, &enabled)) {
                    if(enabled) {
                        tsc_enableResourcePack(pack);
                    } else if(pack != defaultResourcePack) {
                        tsc_disableResourcePack(pack);
                    }
                }
                // this works for the wrong reason
                Texture *resource = rp_resourceTableGet(pack->textures, builtin.textures.icon);
                if(resource != NULL) {
                    DrawTexturePro(
                        *resource, (Rectangle) {0, 0, resource->width, resource->height},
                        (Rectangle) {offset, curY, texturePackTitleSize, texturePackTitleSize},
                        (Vector2) {0, 0}, 0, WHITE
                    );
                }
                GuiSetStyle(DEFAULT, TEXT_SIZE, texturePackAuthorSize);
                int authorWidth = MeasureText(authorSuffix, texturePackAuthorSize);
                GuiLabel(
                    (Rectangle) {
                        offset + nameWidth + texturePackTitleSize * 2,
                        curY + texturePackTitleSize - texturePackAuthorSize,
                        authorWidth, texturePackAuthorSize
                    },
                    authorSuffix);
                curY += texturePackTitleSize;
            }
        }
        if(tsc_streql(tsc_currentMenu, "credits")) {
            tsc_ui_pushFrame(tsc_creditsMenu);
            tsc_ui_column({
                for(size_t i = 0; i < creditsLineCount; i++) {
                    tsc_ui_text(creditsLines[i], 50, WHITE);
                }
            });
            tsc_ui_align(0.5, 0.5, width, height);
            tsc_ui_render();
            tsc_ui_popFrame();
            GuiEnable();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
                particleHalvingTimer = 2;
            }
        }
        if(tsc_streql(tsc_currentMenu, "play")) {
            GuiEnable();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
                particleHalvingTimer = 2;
            }
            float inputWidth = (float)width/5;
            float inputSpacing = inputWidth/5;
            float inputHeight = inputWidth/3; // 3:1 aspect ratio
            float inputY = (float)height/2-inputHeight;

            GuiSetStyle(DEFAULT,TEXT_SIZE, inputHeight);
            if(GuiTextBox((Rectangle) {(float)width/2-inputWidth-inputSpacing, inputY, inputWidth, inputHeight}, gridWidth, TSC_GRID_SIZE_BUFSIZE, gridWidthSel)) {
                gridWidthSel = !gridWidthSel;
            }
            if(GuiTextBox((Rectangle) {(float)width/2+inputSpacing, inputY, inputWidth, inputHeight}, gridHeight, TSC_GRID_SIZE_BUFSIZE, gridHeightSel)) {
                gridHeightSel = !gridHeightSel;
            }
            GuiSetStyle(DEFAULT,TEXT_SIZE, inputSpacing);
            int xWidth = MeasureText("by", inputSpacing);
            GuiLabel((Rectangle) {(float)(width-xWidth)/2, inputY+(inputHeight-inputSpacing), xWidth, inputSpacing}, "by");


            GuiSetStyle(DEFAULT,TEXT_SIZE, inputHeight);
            int playWidth = MeasureText("Play", inputHeight);
            if(GuiLabelButton((Rectangle) {(float)(width-playWidth)/2, inputY+inputHeight+inputSpacing, playWidth, inputHeight}, "Play")) {
                if(strlen(gridWidth) == 0) {
                    goto invalid;
                }
                if(strlen(gridHeight) == 0) {
                    goto invalid;
                }
                if(strspn(gridWidth, "0123456789") != strlen(gridWidth)) {
                    goto invalid;
                }
                if(strspn(gridHeight, "0123456789") != strlen(gridHeight)) {
                    goto invalid;
                }
                int w = atoi(gridWidth);
                int h = atoi(gridHeight);
                if(w == 0) goto invalid;
                if(h == 0) goto invalid;
                tsc_nukeGrids();
                tsc_grid *grid = tsc_createGrid("main", w, h, NULL, NULL);
                tsc_switchGrid(grid);
                if(level != NULL) {
                    // For Windows users, where command prompt getting a 60kb command would explode
                    char *contents = tsc_allocfile(level, NULL);
                    if(contents != NULL) {
                        tsc_saving_decodeWithAny(contents, grid);
                        free(contents);
                    } else {
                        tsc_saving_decodeWithAny(level, grid);
                    }
                    level = NULL;
                }
                tsc_currentMenu = "game";
                tsc_resetRendering();
                tickCount = 0;
                goto valid;

                invalid:
                fprintf(stderr, "Invalid dimensions: %s x %s\n", gridWidth, gridHeight);
                valid:;
            }
        }

        if(tsc_streql(tsc_currentMenu, "nui")) {
            tsc_nui_pushFrame(testFrame);
            tsc_nui_setFontSize(20);
            tsc_nui_setColor(tsc_nui_checkButton(backToMainMenu, TSC_NUI_BUTTON_HOVER) ? 0x00FFFFFF : 0xFFFFFFFF );
            tsc_nui_text("Back");
            tsc_nui_button(backToMainMenu);
            if(tsc_nui_checkButton(backToMainMenu, TSC_NUI_BUTTON_PRESS)) {
                tsc_currentMenu = "main";
            }
            tsc_nui_translate(20, 20);

            float titleSize = 40;
            float textSize = 20;
            float titlePad = 10;

            tsc_nui_setColor(0xFFFFFFFF);
            tsc_nui_setFontSize(40);
            tsc_nui_text("The Sandbox Cell v0.1.0");
            tsc_nui_pad(titlePad, titlePad);
            tsc_nui_aligned(0.5, 0);

            tsc_nui_render();
            tsc_nui_popFrame();
        }

        if(tsc_streql(tsc_currentMenu, "benchmark")) {
            GuiEnable();
            GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
            if(GuiButton((Rectangle) { 20, 20, 100, 50 }, "Back")) {
                tsc_currentMenu = "main";
                particleHalvingTimer = 2;
                free(benchSamples);
                benchSamples = NULL;
            }
            int textSize = 50;
            GuiSetStyle(DEFAULT, TEXT_SIZE, textSize);

            float minTime = 2;
            float maxTime = 60;

            int t = MeasureText("Duration: ", textSize);
            GuiSlider((Rectangle){20 + t, 80, 300, 50}, "Duration: ", tsc_tsprintf("%d", (int)benchTime), &benchTime, minTime, maxTime);
            benchTime = (int)benchTime;

            t = MeasureText("Benchmark: ", textSize);
            GuiSlider((Rectangle){20 + t, 100 + textSize, 300, 50}, "Benchmark: ", benchmarks[(int)benchNum].name, &benchNum, 0, benchmarkCount-1);
            benchNum = (int)benchNum;

            if(GuiLabelButton((Rectangle) {50, 120 + textSize * 2, 100, 50}, "Run")) {
                double startTime = tsc_clock();
                double lastSample = startTime;
                size_t sampleIdx = 0;
                float sampleInterval = 0.1;
                benchSampleCount = 0;
                benchSamples = realloc(benchSamples, sizeof(size_t) * benchSampleCount);

                tsc_nukeGrids();
                tsc_grid *grid = tsc_createGrid("main", 100, 100, NULL, NULL);
                tsc_saving_decodeWithAny(benchmarks[(int)benchNum].level, grid);
                tsc_switchGrid(grid);

                size_t tickCount = 0;
                while(true) {
                    double now = tsc_clock();
                    tsc_subtick_run();
                    tickCount++;
                    if((now - lastSample) >= sampleInterval) {
                        benchSampleCount++;
                        benchSamples = realloc(benchSamples, sizeof(size_t) * benchSampleCount);
                        benchSamples[sampleIdx] = tickCount/(now - lastSample); // this is because samples are meant to be in TPS
                        sampleIdx++;
                        tickCount = 0;
                        lastSample = tsc_clock();
                    }
                    if(now - startTime >= benchTime) break;
                }
                particleHalvingTimer = benchTime + 2;
                printf("Collected %lu samples\n", benchSampleCount);
            }

            if(benchSamples != NULL) {
                float graphWidth = width*0.7;
                float graphHeight = (float)height/2;
                size_t maxTPS = 0;
                for(size_t i = 0; i < benchSampleCount; i++) {
                    size_t tps = benchSamples[i];
                    if(maxTPS < tps) maxTPS = tps;
                }
                float pointSize = 2;
                int yAxisNumberCount = 10;
                float graphX = 200;
                float graphY = 140 + textSize * 3;

                DrawRectangleLines(graphX, graphY, graphWidth, graphHeight, GRAY);

                for(int i = 0; i < yAxisNumberCount; i++) {
                    int num = tsc_mapNumber(i, 0, yAxisNumberCount - 1, maxTPS, 0);
                    int y = tsc_mapNumber(i, 0, yAxisNumberCount - 1, 0, graphHeight);
                    const char *s = tsc_tsprintf("%d", num);
                    int x = -MeasureText(s, textSize) - 10;
                    DrawText(s, graphX + x, graphY + y, textSize, WHITE);
                    DrawLine(graphX, graphY + y, graphX + graphWidth, graphY + y, GRAY);
                }

                int lx, ly;
                for(size_t i = 0; i < benchSampleCount; i++) {
                    size_t tps = benchSamples[i];
                    int x = tsc_mapNumber(i, 0, benchSampleCount - 1, pointSize, graphWidth - pointSize);
                    int y = tsc_mapNumber(tps, 0, maxTPS, graphHeight, 0);

                    if(i > 0) {
                        size_t oldTPS = benchSamples[i-1];
                        Color color = YELLOW;
                        if(tps > oldTPS) color = GREEN;
                        if(tps < oldTPS) color = RED;
                        DrawLine(graphX + lx, graphY + ly, graphX + x, graphY + y, color);
                    }
                    lx = x;
                    ly = y;
                }

                for(size_t i = 0; i < benchSampleCount; i++) {
                    size_t tps = benchSamples[i];
                    int x = tsc_mapNumber(i, 0, benchSampleCount - 1, pointSize, graphWidth - pointSize);
                    int y = tsc_mapNumber(tps, 0, maxTPS, graphHeight, 0);

                    DrawCircle(graphX + x, graphY + y, pointSize, BLUE);
                }
            }
        }

        if(tsc_toBoolean(tsc_getSetting(builtin.settings.debugMode))) {
            tsc_ui_pushFrame(debugFrame);
            tsc_ui_pushColumn();
            {
                int textSize = 20;
                int fps = GetFPS();
                tsc_ui_text(TextFormat("FPS: %d", fps), textSize, fps <= 30 ? RED : GREEN);
                tsc_ui_text(TextFormat("Thread Count: %d", workers_amount()), textSize, WHITE);
                tsc_ui_text(TextFormat("Loaded Cells: %lu", tsc_countCells()), textSize, WHITE);
                tsc_ui_text(TextFormat("Temporary Arena Usage: %lu / %lu", tsc_aused(&tsc_tmp), tsc_acount(&tsc_tmp)), textSize, WHITE);
                if(tsc_streql(tsc_currentMenu, "game")) {
                    tsc_ui_text(TextFormat("Tick Time: %.3fs / %.2fs", tickTime, tickDelay), textSize, tickTime > tickDelay && tickDelay > 0 ? RED : WHITE);
                    Color tpsColor = GREEN;
                    if(gameTPS < 10) tpsColor = YELLOW;
                    if(gameTPS < 5) tpsColor = RED;
                    tsc_ui_text(TextFormat("TPS: %lu", gameTPS), textSize, tpsColor);
                    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
                    int unitc = sizeof(units) / sizeof(units[0]);
                    int unit = 0;
                    size_t area = currentGrid->width * currentGrid->height;
                    double amount = area * sizeof(tsc_cell) * 2 + area * tsc_optSize() + currentGrid->chunkwidth * currentGrid->chunkheight;
                    if(initialCode != NULL) amount += strlen((const char *)initialCode) + 1;
                    while(amount >= 1000 && unit < unitc) {
                        unit++;
                        amount /= 1000;
                    }
                    tsc_ui_text(TextFormat("Grid Memory: %.2lf %s", amount, units[unit]), textSize, WHITE);
                }
            };
            tsc_ui_finishColumn();
            tsc_ui_pad(30, 30);
            tsc_ui_box(GetColor(tsc_queryOptionalColor("cellbarColor", 0x00000055)));
            tsc_ui_translate(10, 10);
            tsc_ui_render();
            tsc_ui_popFrame();
        }

        EndDrawing();

        if(tsc_streql(tsc_currentMenu, "game")) {
            tsc_handleRenderInputs();
        } else {
            double delta = GetFrameTime();
            if(!IsWindowFocused()) {
                delta /= 2;
            }
            double multiplier = 1;
            if(IsKeyDown(KEY_COMMA)) {
                multiplier *= 2;
            }
            if(IsKeyDown(KEY_PERIOD)) {
                multiplier *= 4;
            }
            if(IsKeyDown(KEY_SLASH)) {
                multiplier *= 16;
            }
            if(IsKeyDown(KEY_LEFT_SHIFT)) {
                multiplier = 1.0/multiplier;
            }
            delta *= multiplier;
            int r = (width < height ? width : height) / 4;
            for(size_t i = 0; i < mainMenuParticleCount; i++) {
                tsc_mainMenuParticle_t particle = mainMenuParticles[i];
                float x = 0.5*r/(particle.dist-r);
                float y = x * sqrt(x);
                particle.dist -= particle.g * delta;
                particle.angle += y * delta;
                particle.rot += y * 2 * PI * delta;
                if(particle.dist < (float)r-particle.r) {
                    tsc_id_t oldID = particle.id;
                    particle = tsc_randomMainMenuParticle(true);
                    particle.id = oldID; // phoenix told me something something GPU hates texture swapping
                }
                mainMenuParticles[i] = particle;
            }
        }
        double delta = GetFrameTime();
        if(tsc_streql(tsc_currentMenu, "main")) {
            tsc_ui_bringBackFrame(tsc_mainMenu);
            tsc_ui_update(delta);
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.play) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "play";
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.settings) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "settings";
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.texturepacks) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "texturepacks";
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.credits) == UI_BUTTON_PRESS) {
                tsc_currentMenu = "credits";
            }
            if(tsc_ui_checkbutton(tsc_mainMenuBtn.quit) == UI_BUTTON_PRESS) {
                if(rand() % 50) {
                    break;
                }
                tsc_ui_checkbutton(NULL); // segfault hehe
            }
            tsc_ui_popFrame();
            if(IsKeyPressed(KEY_B)) {
                tsc_currentMenu = "benchmark";
            }
            if(IsKeyPressed(KEY_N)) {
                tsc_currentMenu = "nui";
            }
        }
        if(tsc_streql(tsc_currentMenu, "nui")) {
            tsc_nui_bringBackFrame(testFrame);
            tsc_nui_update();
            tsc_nui_popFrame();
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
        
    tsc_aclear(&tsc_tmp);

    tsc_saveEnabledPacks();
    tsc_storeSettings();
    CloseWindow();
    CloseAudioDevice();

    return EXIT_SUCCESS;
}
