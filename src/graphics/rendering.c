#include "rendering.h"
#include "../saving/saving.h"
#include "resources.h"
#include "../utils.h"
#include <limits.h>
#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "../cells/ticking.h"
#include "../api/api.h"
#include "ui.h"
#include <time.h>
#include <math.h>

typedef struct camera_t {
    double x, y, cellSize, speed;
} camera_t;

static camera_t renderingCamera;
static Shader renderingRepeatingShader;
static unsigned int renderingRepeatingScaleLoc;
static RenderTexture renderingCellTexture;
static int renderingCellBrushSize;
static const char *renderingCellBrushId = NULL;
static ui_frame *renderingGameUI;
static tsc_categorybutton *renderingCellButtons = NULL;
static double renderingApproximationSize = 4;
static double trueApproximationSize;
static double tsc_sizeOptimizedByApproximation;
static float tsc_zoomScrollTotal = 0;
static float tsc_brushScrollBuf = 0;
static int tsc_guidelineMode = 0;

typedef struct selection_t {
    int sx;
    int sy;
    int ex;
    int ey;
} selection_t;

typedef struct selection_btn_t {
    ui_button *copy;
    ui_button *cut;
    ui_button *del;
} selection_btn_t;

typedef struct gridclip_t {
    tsc_cell *cells;
    int width;
    int height;
} gridclip_t;

static gridclip_t renderingGridClipboard;
static selection_t renderingSelection;
static selection_btn_t renderingSelectionButtons;
static bool renderingIsSelecting = false;
static bool renderingIsDragging = false;
static bool renderingIsPasting = false;
static bool renderingBlockPlacing = false;
static bool tsc_isResizingGrid = false;
static char tsc_sideResized = 4;
static int tsc_sideExtension = 0;

static selection_t tsc_fixSelection(selection_t sel) {
    selection_t fixed = sel;
    if(fixed.ex < fixed.sx) {
        tsc_memswap(&fixed.ex, &fixed.sx, sizeof(int));
    }
    if(fixed.ey < fixed.sy) {
        tsc_memswap(&fixed.ey, &fixed.sy, sizeof(int));
    }
    if(fixed.sx < 0) fixed.sx = 0;
    if(fixed.sy < 0) fixed.sy = 0;
    if(fixed.ex >= currentGrid->width) fixed.ex = currentGrid->width-1;
    if(fixed.ey >= currentGrid->height) fixed.ey = currentGrid->height-1;
    return fixed;
}

const char *currentId = NULL;
char currentRot = 0;
static int brushSize = 0;

static tsc_categorybutton *tsc_createCellButtons(tsc_category *category) {
    tsc_categorybutton *buttons = malloc(sizeof(tsc_categorybutton) * category->itemc);
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY) {
            buttons[i].category = tsc_ui_newButtonState();
            buttons[i].items = tsc_createCellButtons(category->items[i].category);
        } else if(category->items[i].kind == TSC_CATEGORY_BUTTON) {
            buttons[i].button = tsc_ui_newButtonState();
        } else {
            buttons[i].cell = tsc_ui_newButtonState();
        }
    }
    return buttons;
}

static void tsc_clearGridClipboard() {
    int l = renderingGridClipboard.width * renderingGridClipboard.height;
    for(int i = 0; i < l; i++) {
        tsc_cell_destroy(renderingGridClipboard.cells[i]);
    }
    free(renderingGridClipboard.cells);
    renderingGridClipboard.cells = NULL;
    renderingGridClipboard.width = 0;
    renderingGridClipboard.height = 0;
}

static void tsc_centerCamera() {
    if(currentGrid == NULL) {
        renderingCamera.x = 0;
        renderingCamera.y = 0;
        return;
    }
    double w = GetScreenWidth();
    double h = GetScreenHeight();
    int cx = currentGrid->width/2;
    int cy = currentGrid->height/2;
    renderingCamera.x = cx * renderingCamera.cellSize - w/2;
    renderingCamera.y = cy * renderingCamera.cellSize - h/2;
}

void tsc_resetRendering() {
    brushSize = 0;
    renderingCamera.cellSize = 32;
    renderingCamera.speed = 200;
    tsc_centerCamera();
    currentId = builtin.mover;
    currentRot = 0;
    isInitial = true;
    renderingCellBrushSize = 0;
    renderingCellBrushId = NULL;
    renderingIsSelecting = false;
    renderingIsPasting = false;
    tsc_isResizingGrid = false;
    tsc_sideResized = 4;
    tsc_sideExtension = 0;
    tsc_zoomScrollTotal = 0;
    tsc_brushScrollBuf = 0;
    trueApproximationSize = renderingApproximationSize;
    tsc_sizeOptimizedByApproximation = 0;
    tsc_ui_clearButtonState(renderingSelectionButtons.copy);
    tsc_ui_clearButtonState(renderingSelectionButtons.cut);
    tsc_ui_clearButtonState(renderingSelectionButtons.del);
    tsc_clearGridClipboard();
}

void tsc_setupRendering() {
    renderingRepeatingShader = LoadShader(NULL, tsc_pathfixi("data/shaders/repeating.glsl"));
    renderingRepeatingScaleLoc = GetShaderLocation(renderingRepeatingShader, "scale");
    renderingCellTexture = LoadRenderTexture(1, 1);
    renderingGameUI = tsc_ui_newFrame();
    renderingSelectionButtons.copy = tsc_ui_newButtonState();
    renderingSelectionButtons.cut = tsc_ui_newButtonState();
    renderingSelectionButtons.del = tsc_ui_newButtonState();
    renderingGridClipboard.cells = NULL;
    renderingGridClipboard.width = 0;
    renderingGridClipboard.height = 0;

    // Why is this in setupRendering?
    // I have no idea
    // This codebase has existed for so little yet is so spaghetti
    tsc_setupUpdateThread();

    if(tickDelay != 0) {
        gameTPS = 1.0 / tickDelay;
    } else {
        gameTPS = 0;
    }

    renderingCellButtons = tsc_createCellButtons(tsc_rootCategory());
    tsc_resetRendering();
}

static float tsc_updateInterp(float a, float b) {
#ifdef TSC_TURBO
    return b;
#endif
    float time = tickTime;
    if(tickDelay == 0) return b;
    if(isGamePaused) return b;
    if(a == -1) return b;
    if(time > tickDelay) return b;
    if(isGameTicking) return a;
    float t = time / tickDelay;
    return a + (b - a) * t;
}

static float tsc_rotInterp(char rot, signed char added) {
    float last = ((signed char)rot) - added;
    float now = rot;
    return tsc_updateInterp(last, now);
}

static void tsc_drawCell(tsc_cell *cell, int x, int y, double opacity, int gridRepeat, bool forceRectangle) {
    if(cell->id == builtin.empty && cell->texture == NULL) return;
    Texture texture = textures_get(cell->texture == NULL ? cell->id : cell->texture);
    double size = renderingCamera.cellSize * gridRepeat;
    Vector2 origin = {size / 2, size / 2};
    Rectangle src = {0, 0, texture.width, texture.height};
    bool isRect = renderingCamera.cellSize < trueApproximationSize || forceRectangle;
    float ix = isRect ? x : tsc_updateInterp(cell->lx, x);
    float iy = isRect ? y : tsc_updateInterp(cell->ly, y);
    float irot = isRect ? cell->rot : tsc_rotInterp(cell->rot, cell->addedRot);
    Rectangle dest = {ix * renderingCamera.cellSize - renderingCamera.x + origin.x,
        iy * renderingCamera.cellSize - renderingCamera.y + origin.y,
        size,
        size};
    Color color = WHITE;
    color.a = opacity * 255;
    if(isRect) {
        Color approx = textures_getApproximation(cell->texture == NULL ? cell->id : cell->texture);
        //approx = ColorAlphaBlend(approx, approx, color);
        approx.a = color.a;
        Vector2 origin = {size / 2, size / 2};
        // My precious little hack
        // Makes sense if you think about it... but I never think
        //dest.x += origin.x;
        //dest.y += origin.y;
        DrawRectanglePro(dest, origin, 0, approx);
        return;
    }
    // Basic cells get super optimized rendering
    if(gridRepeat > 1) {
        float repeat[] = {gridRepeat, gridRepeat, opacity};
        SetShaderValue(renderingRepeatingShader, renderingRepeatingScaleLoc, repeat, SHADER_UNIFORM_VEC3);
        //BeginTextureMode(renderingCellTexture);
        //ClearBackground(BLANK);
        BeginBlendMode(BLEND_ALPHA);
        BeginShaderMode(renderingRepeatingShader);
        //dest.x = origin.x;
        //dest.y = origin.y;
    }
    // weird hack
    DrawTexturePro(texture, src, dest, origin, irot * 90, color);
    if(gridRepeat > 1) {
        EndShaderMode();
        // Weird hack science can not explain
        // DrawTexture(renderingCellTexture.texture, -renderingCamera.x + ix * renderingCamera.cellSize,
        //         -renderingCamera.y + iy * renderingCamera.cellSize, color);
        EndBlendMode();
    }
}

static int tsc_cellScreenX(int screenX) {
    double x = screenX;
    x += renderingCamera.x;
    x /= renderingCamera.cellSize;
    return x;
}

static int tsc_cellScreenY(int screenY) {
    double y = screenY;
    y += renderingCamera.y;
    y /= renderingCamera.cellSize;
    return y;
}

static void tsc_buildCellbar(tsc_category *category, tsc_categorybutton *buttons, int cellButton, int padding, int y) {
    int height = GetScreenHeight();
    int rowHeight = cellButton + padding * 2;
    bool hasOpen = false;
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY && category->items[i].category->open) {
            hasOpen = true;
        }
    }
    tsc_ui_row({
        for(size_t i = 0; i < category->itemc; i++) {
            if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY) {
                tsc_category *sub = category->items[i].category;
                tsc_ui_image(sub->icon, cellButton, cellButton);
                if(tsc_ui_button(buttons[i].category) == UI_BUTTON_HOVER || sub->open) {
                    tsc_ui_translate(0, -10);
                }
                if(!sub->open && !hasOpen) tsc_ui_tooltip(sub->title, 30, sub->description, 20, 100);
                tsc_ui_pad(padding, padding);
            } else if(category->items[i].kind == TSC_CATEGORY_BUTTON) {
                tsc_cellbutton btn = category->items[i].button;
                tsc_ui_image(btn.icon, cellButton, cellButton);
                if(tsc_ui_button(buttons[i].button) == UI_BUTTON_HOVER) {
                    tsc_ui_translate(0, -10);
                }
                if(!hasOpen) tsc_ui_tooltip(btn.name, 30, btn.desc, 20, 100);
                tsc_ui_pad(padding, padding);
            } else {
                tsc_ui_image(category->items[i].cellID, cellButton, cellButton);
                if(tsc_ui_button(buttons[i].cell) == UI_BUTTON_HOVER) {
                    tsc_ui_translate(0, -10);
                }
                if(!hasOpen) {
                    tsc_cellprofile_t *profile = tsc_getProfile(category->items[i].cellID);
                    if(profile != NULL) {
                        tsc_ui_tooltip(profile->name, 30, profile->desc, 20, 100);
                    }
                }
                tsc_ui_pad(padding, padding);
            }
        }
    });
    tsc_ui_translate(0, height - rowHeight - y);
    if(hasOpen) for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY && category->items[i].category->open) {
            tsc_buildCellbar(category->items[i].category, buttons[i].items, cellButton, padding, y + rowHeight);
        }
    }
}

static void tsc_updateCellbar(tsc_category *category, tsc_categorybutton *buttons) {
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY) {
            if(category->items[i].category->usedAsCell) {
                char state = tsc_ui_checkbutton(buttons[i].category);
                if(state == UI_BUTTON_PRESS) {
                    currentId = category->items[i].category->icon;
                    tsc_closeCategory(category->items[i].category);
                } else if(state == UI_BUTTON_RIGHTPRESS) {
                    if(category->items[i].category->open) {
                        tsc_closeCategory(category->items[i].category);
                    } else {
                        tsc_openCategory(category->items[i].category);
                    }
                }
            } else {
                if(tsc_ui_checkbutton(buttons[i].category) == UI_BUTTON_PRESS) {
                    if(category->items[i].category->open) {
                        tsc_closeCategory(category->items[i].category);
                    } else {
                        tsc_openCategory(category->items[i].category);
                    }
                }
            }
            tsc_updateCellbar(category->items[i].category, buttons[i].items);
        } else if(category->items[i].kind == TSC_CATEGORY_BUTTON) {
            if(tsc_ui_checkbutton(buttons[i].button) == UI_BUTTON_PRESS) {
                category->items[i].button.click(category->items[i].button.payload);
            }
        } else {
            if(tsc_ui_checkbutton(buttons[i].cell) == UI_BUTTON_PRESS) {
                currentId = category->items[i].cellID;
            }
        }
    }
}


void tsc_drawGrid() {
    Vector2 emptyOrigin = {0, 0};
    Rectangle emptyDest = {-renderingCamera.x, -renderingCamera.y, renderingCamera.cellSize * currentGrid->width, renderingCamera.cellSize * currentGrid->height};
    if(renderingCamera.cellSize < trueApproximationSize) {
        if(GetFPS() > 60 && renderingCamera.cellSize > tsc_sizeOptimizedByApproximation) {
            trueApproximationSize /= 2;
            tsc_sizeOptimizedByApproximation = renderingCamera.cellSize;
        }
        Color approx = textures_getApproximation(builtin.empty);
        DrawRectanglePro(emptyDest, emptyOrigin, 0, approx);
    } else {
        if(GetFPS() < 30) {
            trueApproximationSize *= 2;
        }
        Texture empty = textures_get(builtin.empty);
        float emptyScale[3] = {currentGrid->width, currentGrid->height, 1};
        SetShaderValue(renderingRepeatingShader, renderingRepeatingScaleLoc, emptyScale, SHADER_UNIFORM_VEC3);
        BeginShaderMode(renderingRepeatingShader);
        {
            Rectangle emptySrc = {0, 0, empty.width, empty.height};
            DrawTexturePro(empty, emptySrc, emptyDest, emptyOrigin, 0, WHITE);
        }
        EndShaderMode();
    }

    int sx = tsc_cellScreenX(0);
    if(sx < 0) sx = 0;
    int sy = tsc_cellScreenY(0);
    if(sy < 0) sy = 0;
    int ex = tsc_cellScreenX(GetScreenWidth());
    if(ex >= currentGrid->width) ex = currentGrid->width-1;
    int ey = tsc_cellScreenY(GetScreenHeight());
    sx -= sx % tsc_gridChunkSize;
    sy -= sy % tsc_gridChunkSize;
    if(ey >= currentGrid->height) ey = currentGrid->height-1;
    if(ex < sx) ex = sx;
    if(ey < sy) ey = sy;

    int maxRenderCount = 32768;
    int skipLevel = 1;
    int renderCount = (ex - sx + 1) * (ey - sy + 1);
    while(renderCount > maxRenderCount) {
        skipLevel *= 2;
        renderCount /= 4;
    }
    if(renderingCamera.cellSize >= trueApproximationSize) {
        skipLevel = 1;
    }

    for(size_t y = sy; y <= ey; y = (y+skipLevel) - y % skipLevel) {
        for(size_t x = sx; x <= ex; x = (x+skipLevel) - x % skipLevel) {
            if(!tsc_grid_checkChunk(currentGrid, x, y)) {
                continue;
            }
            int repeat = skipLevel;
            //if(x + repeat >= currentGrid->width) repeat = currentGrid->width - x;
            //if(y + repeat >= currentGrid->height) repeat = currentGrid->height - y;
            tsc_cell *bg = tsc_grid_background(currentGrid, x, y);
            if(bg == NULL) break;
            tsc_drawCell(bg, x, y, 1, repeat, repeat > 1);
        }
    }

    for(size_t y = sy; y <= ey; y = (y+skipLevel) - y % skipLevel) {
        for(size_t x = sx; x <= ex; x = (x+skipLevel) - x % skipLevel) {
            if(!tsc_grid_checkChunk(currentGrid, x, y)) {
                continue;
            }
            int repeat = skipLevel;
            if(x + repeat > currentGrid->width) repeat = currentGrid->width - x;
            if(y + repeat > currentGrid->height) repeat = currentGrid->height - y;
            tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
            if(cell == NULL) break;
            tsc_drawCell(cell, x, y, 1, repeat, repeat > 1);
        }
    }

    int selx;
    int sely;
    int selw;
    int selh;
    float selthickness = renderingCamera.cellSize / 4;
    float selrowpad = renderingCamera.cellSize;
    float selbtnsize = renderingCamera.cellSize * 2;
    float selbtnpad = selbtnsize / 4;

    if(renderingIsSelecting) {
        selection_t fixedSel = tsc_fixSelection(renderingSelection);
        int width = fixedSel.ex - fixedSel.sx + 1;
        int height = fixedSel.ey - fixedSel.sy + 1;

        if(selbtnsize < 32) {
            selbtnsize = 32;
            selbtnpad = 8;
        }

        selx = fixedSel.sx * renderingCamera.cellSize - renderingCamera.x;
        sely = fixedSel.sy * renderingCamera.cellSize - renderingCamera.y;
        selw = width * renderingCamera.cellSize;
        selh = height * renderingCamera.cellSize;

        int units = 2 * 3 + 2;
        selbtnsize = (float)selh / units;
        selbtnpad = selbtnsize / 4;

        Rectangle rect = {selx, sely, selw, selh};

        DrawRectangleLinesEx(rect, selthickness, WHITE);
        DrawRectangle(selx, sely, selw, selh, GetColor(0x00000066));
        float sizeHeight = (float)GetScreenHeight()/40;
        DrawText(TextFormat("%d x %d\n", width, height), selx, sely - sizeHeight, sizeHeight, WHITE);
    } else if(renderingIsPasting) {
        int mx = tsc_cellMouseX();
        int my = tsc_cellMouseY();
        for(int x = 0; x < renderingGridClipboard.width; x++) {
            for(int y = 0; y < renderingGridClipboard.height; y++) {
                int i = y * renderingGridClipboard.width + x;
                tsc_drawCell(&renderingGridClipboard.cells[i], mx + x, my + y, 0.5, 1, false);
            }
        }
    }
    if((!renderingIsSelecting || tsc_guidelineMode != 0) && !renderingIsPasting) {
        int cmx = tsc_cellMouseX();
        int cmy = tsc_cellMouseY();
        cmx = cmx - brushSize;
        cmy = cmy - brushSize;
        tsc_cell cell = tsc_cell_create(currentId, currentRot);
        cell.lx = cmx;
        cell.ly = cmy;
        cell.addedRot = 0;
        tsc_drawCell(&cell, cmx, cmy, 0.5, brushSize*2+1, false);
    }

    if(tsc_guidelineMode != 0) {
        float sx = 0;
        float sy = 0;
        float ex = currentGrid->width;
        float ey = currentGrid->height;

        if(renderingIsSelecting) {
            sx = renderingSelection.sx;
            sy = renderingSelection.sy;
            ex = renderingSelection.ex+1;
            ey = renderingSelection.ey+1;
        }

        sx = sx * renderingCamera.cellSize - renderingCamera.x;
        sy = sy * renderingCamera.cellSize - renderingCamera.y;
        ex = ex * renderingCamera.cellSize - renderingCamera.x;
        ey = ey * renderingCamera.cellSize - renderingCamera.y;

        if(tsc_guidelineMode == 1) {
            float mx = sx+(ex-sx)/2;
            float my = sy+(ey-sy)/2;

            DrawLine(sx, my, ex, my, YELLOW);
            DrawLine(mx, sy, mx, ey, BLUE);
        }
        if(tsc_guidelineMode == 2) {
            DrawLine(sx, sy, ex, ey, YELLOW);
            DrawLine(ex, sy, sx, ey, BLUE);
        }
        if(tsc_guidelineMode == 3) {
            float mx = sx+(ex-sx)/2;
            float my = sy+(ey-sy)/2;

            DrawLine(sx, my, ex, my, YELLOW);
            DrawLine(mx, sy, mx, ey, BLUE);
            DrawLine(sx, sy, ex, ey, RED);
            DrawLine(ex, sy, sx, ey, ORANGE);
        }
        if(tsc_guidelineMode == 4) {
            float mx = sx+(ex-sx)/2;
            float my = sy+(ey-sy)/2;
            float w = ex - mx;
            float h = ey - my;

            float xDiag = cos(PI/4)*w;
            float yDiag = sin(PI/4)*h;

            DrawLine(sx, my, ex, my, YELLOW);
            DrawLine(mx, sy, mx, ey, BLUE);
            DrawLine(mx-xDiag, my-yDiag, mx+xDiag, my+yDiag, RED);
            DrawLine(mx+xDiag, my-yDiag, mx-xDiag, my+yDiag, ORANGE);
            DrawEllipseLines(mx, my, w, h, GREEN);
        }
    }

    static char buffer[256];

    // Debug stuff
    // +0.01 because 10ms is less time than it takes for the brain to render a frame, let alone notice anything.
    if(tickTime > (tickDelay + 0.01) && isGameTicking && tickDelay > 0) {
        snprintf(buffer, 256, "Tick is taking %.3fs longer than normal. Please wait.", tickTime - tickDelay);
        DrawText(buffer, 10, 40, 20, RED);
    }
    if(tickDelay == 0) {
        snprintf(buffer, 256, "TPS: %lu", gameTPS);
        Color color = GREEN;
        if(gameTPS < 10) {
            color = YELLOW; // still good but nothing to flex about
        }
        if(gameTPS < 4) {
            color = RED; // substandard
        }
        DrawText(buffer, 10, 40, 20, color);
    }

    if(tickCount > 0) {
        snprintf(buffer, 256, "Tick Count: %lu", tickCount);
        int tickCountSize = 20;
        int tickCountWidth = MeasureText(buffer, tickCountSize);
        DrawText(buffer, GetScreenWidth() - tickCountWidth - 10, 10, tickCountSize, WHITE);
    }

    DrawFPS(10, 10);

    tsc_ui_pushFrame(renderingGameUI);
    if(renderingIsSelecting && !renderingIsDragging) {
        float minselbtnsize = (float)GetScreenWidth()/40;
        if(selbtnsize < minselbtnsize) {
            selbtnsize = minselbtnsize;
        }
        tsc_ui_row({
            tsc_ui_image(builtin.textures.copy, selbtnsize, selbtnsize);
            tsc_ui_button(renderingSelectionButtons.copy);
            tsc_ui_pad(selbtnpad, selbtnpad);
            tsc_ui_image(builtin.textures.cut, selbtnsize, selbtnsize);
            tsc_ui_button(renderingSelectionButtons.cut);
            tsc_ui_pad(selbtnpad, selbtnpad);
            tsc_ui_image(builtin.textures.del, selbtnsize, selbtnsize);
            tsc_ui_button(renderingSelectionButtons.del);
            tsc_ui_pad(selbtnpad, selbtnpad);
        });
        tsc_ui_translate(selx, sely + selh + (selrowpad < selbtnsize ? selrowpad : selbtnsize));
    }
    int height = GetScreenHeight();
    int padding = 10;
    int cellButton = 60;
    int rowHeight = cellButton + padding * 2;
    if(!tsc_isResizingGrid) {
        // Cellbar background
        tsc_ui_space(GetScreenWidth());
        tsc_ui_box(GetColor(tsc_queryOptionalColor("cellbarColor", 0x00000055)));
        tsc_ui_translate(0, height - rowHeight);
        tsc_buildCellbar(tsc_rootCategory(), renderingCellButtons, cellButton, padding, 0);
        tsc_ui_render();
    }
    tsc_ui_popFrame();
}

int tsc_cellMouseX() {
    double x = GetMouseX();
    x += renderingCamera.x;
    if(x < 0) x -= renderingCamera.cellSize; // accounts for rounding error
    x /= renderingCamera.cellSize;
    return x;
}

int tsc_cellMouseY() {
    double y = GetMouseY();
    y += renderingCamera.y;
    if(y < 0) y -= renderingCamera.cellSize; // accounts for rounding error
    y /= renderingCamera.cellSize;
    return y;
}

static void tsc_setZoomLevel(double cellSize) {
    float mx = GetMouseX();
    float my = GetMouseY();
    float scale = cellSize / renderingCamera.cellSize;
    renderingCamera.cellSize = cellSize;
    renderingCamera.x += mx;
    renderingCamera.y += my;
    renderingCamera.x *= scale;
    renderingCamera.y *= scale;
    renderingCamera.x -= mx;
    renderingCamera.y -= my;
    renderingCellBrushSize = 0;
}

static void tsc_handleCellPlace() {
    if(isGameTicking && !multiTickPerFrame) return;
    long x = tsc_cellMouseX();
    long y = tsc_cellMouseY();

    tsc_cell current = tsc_cell_create(currentId, currentRot);
    tsc_cell nothing = tsc_cell_create(builtin.empty, 0);

    size_t currentFlags = tsc_cell_getTableFlags(&current);

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        for(int ox = -brushSize; ox <= brushSize; ox++) {
            for(int oy = -brushSize; oy <= brushSize; oy++) {
                if(currentFlags & TSC_FLAGS_PLACEABLE) {
                    tsc_grid_setBackground(currentGrid, x + ox, y + oy, &current);
                } else {
                    tsc_grid_set(currentGrid, x + ox, y + oy, &current);
                }
            }
        }
    }

    if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        for(int ox = -brushSize; ox <= brushSize; ox++) {
            for(int oy = -brushSize; oy <= brushSize; oy++) {
                if(currentFlags & TSC_FLAGS_PLACEABLE) {
                    tsc_grid_setBackground(currentGrid, x + ox, y + oy, &nothing);
                } else {
                    tsc_grid_set(currentGrid, x + ox, y + oy, &nothing);
                }
            }
        }
    }
}

static void tsc_copySelection() {
    renderingIsSelecting = false;
    renderingIsDragging = false;
    tsc_clearGridClipboard();
    selection_t fixed = tsc_fixSelection(renderingSelection);
    int width = fixed.ex - fixed.sx + 1;
    int height = fixed.ey - fixed.sy + 1;
    renderingGridClipboard.width = width;
    renderingGridClipboard.height = height;
    renderingGridClipboard.cells = malloc(sizeof(tsc_cell) * width * height);
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int i = y * width + x;
            renderingGridClipboard.cells[i] = tsc_cell_clone(tsc_grid_get(currentGrid, fixed.sx + x, fixed.sy + y));
        }
    }
    renderingIsPasting = true;
}

static void tsc_cutSelection() {
    renderingIsSelecting = false;
    renderingIsDragging = false;
    selection_t fixed = tsc_fixSelection(renderingSelection);
    int width = fixed.ex - fixed.sx + 1;
    int height = fixed.ey - fixed.sy + 1;
    renderingGridClipboard.width = width;
    renderingGridClipboard.height = height;
    renderingGridClipboard.cells = malloc(sizeof(tsc_cell) * width * height);
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int i = y * width + x;
            renderingGridClipboard.cells[i] = tsc_cell_clone(tsc_grid_get(currentGrid, fixed.sx + x, fixed.sy + y));
            tsc_cell empty = tsc_cell_create(builtin.empty, 0);
            tsc_grid_set(currentGrid, fixed.sx + x, fixed.sy + y, &empty);
        }
    }
    renderingIsPasting = true;
}

static void tsc_deleteSelection() {
    renderingIsSelecting = false;
    renderingIsDragging = false;
    selection_t fixed = tsc_fixSelection(renderingSelection);
    int width = fixed.ex - fixed.sx + 1;
    int height = fixed.ey - fixed.sy + 1;
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            tsc_cell empty = tsc_cell_create(builtin.empty, 0);
            tsc_grid_set(currentGrid, fixed.sx + x, fixed.sy + y, &empty);
        }
    }
}

void tsc_pasteGridClipboard() {
    if(renderingGridClipboard.width == 0) return;
    renderingIsSelecting = false;
    renderingIsDragging = false;
    renderingIsPasting = true;
}


void tsc_handleRenderInputs() {
    float delta = GetFrameTime();

    tsc_ui_bringBackFrame(renderingGameUI);
    tsc_ui_update(delta);
    tsc_updateCellbar(tsc_rootCategory(), renderingCellButtons);
    int absorbed = tsc_ui_absorbedPointer(GetMouseX(), GetMouseY());
    tsc_ui_popFrame();

    double speed = renderingCamera.speed;
    if(IsKeyDown(KEY_LEFT_SHIFT)) {
        speed *= 2;
    }
    if(IsKeyDown(KEY_LEFT_CONTROL)) {
        if(IsKeyPressed(KEY_A)) {
            renderingIsSelecting = true;
            renderingSelection.sx = 0;
            renderingSelection.sy = 0;
            renderingSelection.ex = currentGrid->width-1;
            renderingSelection.ey = currentGrid->height-1;
        }
        if(IsKeyPressed(KEY_C)) {
            tsc_copySelection();
        }
        if(IsKeyPressed(KEY_X)) {
            tsc_cutSelection();
        }
        if(IsKeyPressed(KEY_V) && renderingIsSelecting) {
            renderingIsSelecting = false;
            renderingIsPasting = true;
        }
    } else {
        if(IsKeyDown(KEY_W)) {
            renderingCamera.y -= speed * delta;
        }
        if(IsKeyDown(KEY_S)) {
            renderingCamera.y += speed * delta;
        }
        if(IsKeyDown(KEY_A)) {
            renderingCamera.x -= speed * delta;
        }
        if(IsKeyDown(KEY_D)) {
            renderingCamera.x += speed * delta;
        }
    }

    if(IsKeyPressed(KEY_G)) {
        int maxGuidelines = 5;
        tsc_guidelineMode++;
        tsc_guidelineMode %= maxGuidelines;
    }

    if(IsKeyPressed(KEY_DELETE)) {
        if(renderingIsSelecting) {
            tsc_deleteSelection();
        }
    }

    if(IsKeyPressed(KEY_HOME)) {
        tsc_centerCamera();
    }
   
    if(IsKeyPressed(KEY_ESCAPE)) {
        if(renderingIsPasting) {
            renderingIsPasting = false;
        } else if(renderingIsSelecting && !renderingIsDragging) {
            renderingIsSelecting = false;
            renderingIsDragging = false;
        } else if(!isGameTicking && isGamePaused) {
            tsc_currentMenu = "main";
            tsc_nukeGrids();
            return; // game is done.
        }
    }

    if(IsKeyPressed(KEY_Q)) {
        if(renderingIsPasting) {
            int width = renderingGridClipboard.width;
            int height = renderingGridClipboard.height;
            tsc_cell *copy = malloc(sizeof(tsc_cell) * width * height);

            for(int x = 0; x < width; x++) {
                for(int y = 0; y < height; y++) {
                    int o = x * height + y;
                    int i = y * width + (width - x - 1);
                    copy[o] = renderingGridClipboard.cells[i];
                    copy[o].rot += 3;
                    copy[o].rot %= 4;
                }
            }

            free(renderingGridClipboard.cells);
            renderingGridClipboard.cells = copy;

            renderingGridClipboard.width = height;
            renderingGridClipboard.height = width;
        } else {
            currentRot--;
            while(currentRot < 0) currentRot += 4;
        }
    }
    if(IsKeyPressed(KEY_E)) {
        if(renderingIsPasting) {
            int width = renderingGridClipboard.width;
            int height = renderingGridClipboard.height;
            tsc_cell *copy = malloc(sizeof(tsc_cell) * width * height);

            for(int x = 0; x < width; x++) {
                for(int y = 0; y < height; y++) {
                    int o = x * height + y;
                    int i = (height - y - 1) * width + x;
                    copy[o] = renderingGridClipboard.cells[i];
                    copy[o].rot += 1;
                    copy[o].rot %= 4;
                }
            }

            free(renderingGridClipboard.cells);
            renderingGridClipboard.cells = copy;

            renderingGridClipboard.width = height;
            renderingGridClipboard.height = width;
        } else {
            currentRot++;
            currentRot %= 4;
        }
    }

    if(IsKeyPressed(KEY_L) && !isGameTicking) {
        const char *clipboard = GetClipboardText();
        if(clipboard != NULL) {
            tsc_saving_decodeWithAny(clipboard, currentGrid);
            isInitial = true;
            tickCount = 0;
        }
    }

#define CELL_KEY(key1, key2, cell) do {if(IsKeyPressed(key1) || (ch == key2)) currentId = cell;} while(0)

    while(true) {
        char ch = GetCharPressed();
        if(ch == 0) break;
        CELL_KEY(KEY_KP_0, '0', builtin.placeable);
        CELL_KEY(KEY_KP_1, '1', builtin.generator);
        CELL_KEY(KEY_KP_2, '2', builtin.rotator_cw);
        CELL_KEY(KEY_KP_3, '3', builtin.rotator_ccw);
        CELL_KEY(KEY_KP_4, '4', builtin.mover);
        CELL_KEY(KEY_KP_5, '5', builtin.push);
        CELL_KEY(KEY_KP_6, '6', builtin.slide);
        CELL_KEY(KEY_KP_7, '7', builtin.enemy);
        CELL_KEY(KEY_KP_8, '8', builtin.trash);
        CELL_KEY(KEY_KP_9, '9', builtin.wall);
    }

#undef CELL_KEY

    if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && !renderingIsPasting) {
        renderingIsSelecting = true;
        renderingIsDragging = true;
        long mx = tsc_cellMouseX();
        long my = tsc_cellMouseY();
        renderingSelection.sx = mx;
        renderingSelection.sy = my;
        renderingSelection.ex = mx;
        renderingSelection.ey = my;
    }

    if(renderingIsDragging) {
        renderingSelection.ex = tsc_cellMouseX();
        renderingSelection.ey = tsc_cellMouseY();
        if(renderingSelection.ex < 0) renderingSelection.ex = 0;
        if(renderingSelection.ey < 0) renderingSelection.ey = 0;
        if(renderingSelection.ex >= currentGrid->width) renderingSelection.ex = currentGrid->width-1;
        if(renderingSelection.ey >= currentGrid->height) renderingSelection.ey = currentGrid->height-1;
    }


    if(IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE)) {
        renderingIsDragging = false;
    }

    if(!absorbed && ((!renderingIsSelecting && !renderingIsPasting) || tsc_guidelineMode != 0) && !renderingBlockPlacing && !tsc_isResizingGrid)
        tsc_handleCellPlace();

    if(!renderingIsSelecting && !renderingIsPasting) {
        if(IsKeyPressed(KEY_LEFT_ALT)) {
            tsc_isResizingGrid = !tsc_isResizingGrid;
        }
    }

    if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        renderingBlockPlacing = false;
    }

    if(renderingIsPasting && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        renderingIsPasting = false;
        renderingBlockPlacing = true;
        int mx = tsc_cellMouseX();
        int my = tsc_cellMouseY();
        for(int x = 0; x < renderingGridClipboard.width; x++) {
            for(int y = 0; y < renderingGridClipboard.height; y++) {
                int i = y * renderingGridClipboard.width + x;
                if(renderingGridClipboard.cells[i].id == builtin.empty) continue;
                tsc_grid_set(currentGrid, mx + x, my + y, &renderingGridClipboard.cells[i]);
            }
        }
    }

    if(tsc_ui_checkbutton(renderingSelectionButtons.copy) == UI_BUTTON_PRESS) {
        printf("Copied selection\n");
        tsc_ui_clearButtonState(renderingSelectionButtons.copy);
        tsc_copySelection();
    }
    if(IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_CONTROL)) {
        if(renderingIsSelecting) tsc_copySelection();
    }
    if(tsc_ui_checkbutton(renderingSelectionButtons.cut) == UI_BUTTON_PRESS) {
        printf("Cut selection\n");
        tsc_ui_clearButtonState(renderingSelectionButtons.cut);
        tsc_clearGridClipboard();
        tsc_cutSelection();
    }
    if(IsKeyPressed(KEY_X) && IsKeyDown(KEY_LEFT_CONTROL)) {
        if(renderingIsSelecting) tsc_cutSelection();
    }
    if(tsc_ui_checkbutton(renderingSelectionButtons.del) == UI_BUTTON_PRESS) {
        printf("Deleted selection\n");
        tsc_ui_clearButtonState(renderingSelectionButtons.del);
        tsc_clearGridClipboard();
        tsc_deleteSelection();
    }
    if(IsKeyPressed(KEY_BACKSPACE) && IsKeyDown(KEY_LEFT_CONTROL)) {
        if(renderingIsSelecting) tsc_deleteSelection();
    }

    if(renderingGridClipboard.width != 0 && IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_CONTROL)) {
        renderingIsPasting = true;
    }

    if(renderingIsPasting && IsKeyPressed(KEY_ESCAPE)) {
        renderingIsPasting = false;
    }

    float mouseWheel = GetMouseWheelMove();
    if(IsKeyDown(KEY_LEFT_CONTROL)) {
        tsc_brushScrollBuf += mouseWheel;
        int amount = 32.0 / renderingCamera.cellSize;
        if(amount < 1) amount = 1;
        while(tsc_brushScrollBuf >= 1.5) {
            tsc_brushScrollBuf -= 1.5;
            brushSize += amount;
        }
        while(tsc_brushScrollBuf <= -1.5) {
            tsc_brushScrollBuf += 1.5;
            if(brushSize >= amount) brushSize -= amount;
            else brushSize = 0;
        }
    } else {
        tsc_zoomScrollTotal += mouseWheel;
        tsc_setZoomLevel(32.0 * (pow(2, tsc_zoomScrollTotal / 1.5)));
    }

    if(IsKeyPressed(KEY_SPACE) && !renderingIsPasting && !renderingIsSelecting && !renderingIsDragging && !tsc_isResizingGrid) {
        isGamePaused = !isGamePaused;
    }

    if(IsKeyPressed(KEY_R)) {
        if(isGamePaused && !isGameTicking) {
            tsc_copyGrid(currentGrid, tsc_getGrid("initial"));
            isInitial = true;
            tickCount = 0;
        }
    }

    float tickTimeScale = 1;
    if(IsKeyDown(KEY_LEFT_SHIFT)) {
        if(IsKeyDown(KEY_COMMA)) {
            tickTimeScale /= 2;
        }
        if(IsKeyDown(KEY_PERIOD)) {
            tickTimeScale /= 4;
        }
        if(IsKeyDown(KEY_SLASH)) {
            tickTimeScale /= 16;
        }
    } else {
        if(IsKeyDown(KEY_COMMA)) {
            tickTimeScale *= 2;
        }
        if(IsKeyDown(KEY_PERIOD)) {
            tickTimeScale *= 4;
        }
        if(IsKeyDown(KEY_SLASH)) {
            tickTimeScale *= 16;
        }
    }

    if(!isGamePaused || isGameTicking) tickTime += delta * tickTimeScale;

    if((IsKeyPressed(KEY_F) || IsKeyPressedRepeat(KEY_F)) && !isGameTicking) {
        onlyOneTick = true;
        if(!multiTickPerFrame) {
            tsc_signalUpdateShouldHappen();
        }
    }

    if(!isGamePaused && !multiTickPerFrame) {
        if(tickTime >= tickDelay && !isGameTicking) {
            tsc_signalUpdateShouldHappen();
        }
    }
}
