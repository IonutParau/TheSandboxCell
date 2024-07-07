#include "rendering.h"
#include "../cells/subticks.h"
#include "../saving/saving.h"
#include "resources.h"
#include "../utils.h"
#include <raylib.h>
#include <stdio.h>
#include <stdbool.h>
#include "../cells/ticking.h"
#include <time.h>

typedef struct camera_t {
    double x, y, cellSize, speed;
} camera_t;

static camera_t renderingCamera;
static Shader renderingRepeatingShader;
static unsigned int renderingRepeatingScaleLoc;
static Texture renderingEmpty;
static RenderTexture renderingCellTexture;
static int renderingCellBrushSize;

const char *currentId = NULL;
char currentRot = 0;
static int brushSize = 0;

void tsc_setupRendering() {
    renderingCamera.x = 0;
    renderingCamera.y = 0;
    renderingCamera.cellSize = 32;
    renderingCamera.speed = 200;
    renderingRepeatingShader = LoadShader(NULL, "shaders/repeating.glsl");
    renderingRepeatingScaleLoc = GetShaderLocation(renderingRepeatingShader, "scale");
    renderingEmpty = textures_get(builtin.empty);
    currentId = tsc_strintern("mover");
    currentRot = 0;
    renderingCellTexture = LoadRenderTexture(1, 1);
    renderingCellBrushSize = 0;

    // Why is this in setupRendering?
    // To fix bugs
    tsc_setupUpdateThread();

    if(tickDelay != 0) {
        gameTPS = 1.0 / tickDelay;
    } else {
        gameTPS = 0;
    }
}

void tsc_drawCell(tsc_cell *cell, int x, int y, double opacity, int gridRepeat) {
    if(cell->id == builtin.empty && cell->texture == NULL) return;
    Texture texture = textures_get(cell->texture == NULL ? cell->id : cell->texture);
    double size = renderingCamera.cellSize * (2 * gridRepeat + 1);
    Vector2 origin = {size / 2, size / 2};
    Rectangle src = {0, 0, texture.width, texture.height};
    Rectangle dest = {x * renderingCamera.cellSize - renderingCamera.x + origin.x - renderingCamera.cellSize * gridRepeat,
        y * renderingCamera.cellSize - renderingCamera.y + origin.y - renderingCamera.cellSize * gridRepeat,
        size,
        size};
    Color color = WHITE;
    color.a = opacity * 255;
    // Basic cells get super optimized rendering
    if(gridRepeat > 0) {
        // We need a render texture that is big enough
        if(renderingCellBrushSize != gridRepeat) {
            UnloadRenderTexture(renderingCellTexture);
            renderingCellTexture = LoadRenderTexture(size, size);
            renderingCellBrushSize = gridRepeat;
        }
        float repeat[] = {gridRepeat * 2 + 1, gridRepeat * 2 + 1};
        SetShaderValue(renderingRepeatingShader, renderingRepeatingScaleLoc, repeat, SHADER_UNIFORM_VEC2);
        BeginTextureMode(renderingCellTexture);
        BeginShaderMode(renderingRepeatingShader);
        dest.x = origin.x;
        dest.y = origin.y;
    }
    DrawTexturePro(texture, src, dest, origin, cell->rot * 90, color);
    if(gridRepeat > 0) {
        EndShaderMode();
        EndTextureMode();
        // Weird hack science can not explain
        DrawTexture(renderingCellTexture.texture, -renderingCamera.x + (x - gridRepeat) * renderingCamera.cellSize,
                -renderingCamera.y + (y - gridRepeat) * renderingCamera.cellSize, color);
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

void tsc_drawGrid() {
    Texture empty = renderingEmpty;
    float emptyScale[2] = {currentGrid->width, currentGrid->height};
    SetShaderValue(renderingRepeatingShader, renderingRepeatingScaleLoc, emptyScale, SHADER_UNIFORM_VEC2);
    BeginShaderMode(renderingRepeatingShader);
    {
        Rectangle emptySrc = {0, 0, empty.width, empty.height};
        Rectangle emptyDest = {-renderingCamera.x, -renderingCamera.y, renderingCamera.cellSize * currentGrid->width, renderingCamera.cellSize * currentGrid->height};
        Vector2 emptyOrigin = {0, 0};
        DrawTexturePro(empty, emptySrc, emptyDest, emptyOrigin, 0, WHITE);
    }
    EndShaderMode();

    int sx = tsc_cellScreenX(0);
    if(sx < 0) sx = 0;
    int sy = tsc_cellScreenY(0);
    if(sy < 0) sy = 0;
    int ex = tsc_cellScreenX(GetScreenWidth());
    if(ex >= currentGrid->width) ex = currentGrid->width-1;
    int ey = tsc_cellScreenY(GetScreenHeight());
    if(ey >= currentGrid->height) ey = currentGrid->height-1;

    for(size_t x = sx; x <= ex; x++) {
        for(size_t y = sy; y <= ey; y++) {
            tsc_cell *cell = tsc_grid_get(currentGrid, x, y);
            if(cell == NULL) break;
            tsc_drawCell(cell, x, y, 1, 0);
        }
    }

    int cmx = tsc_cellMouseX();
    int cmy = tsc_cellMouseY();
    tsc_cell cell = tsc_cell_create(currentId, currentRot);
    tsc_drawCell(&cell, cmx, cmy, 0.5, brushSize);

    static char buffer[256];

    // Debug stuff
    // +0.01 because 10ms is less time than it takes for the brain to render a frame, let alone notice anything.
    if(tickTime > (tickDelay + 0.01) && isGameTicking && tickDelay > 0) {
        snprintf(buffer, 256, "Tick is taking %.3fs longer than normal. Please wait.", tickTime - tickDelay);
        DrawText(buffer, 10, 40, 20, RED);
    }
    if(tickDelay == 0) {
        snprintf(buffer, 256, "TPS: %lu\n", gameTPS);
        Color color = GREEN;
        if(gameTPS < 10) {
            color = YELLOW; // still good but nothing to flex about
        }
        if(gameTPS < 4) {
            color = RED; // substandard
        }
        DrawText(buffer, 10, 40, 20, color);
    }

    DrawFPS(10, 10);
}

int tsc_cellMouseX() {
    double x = GetMouseX();
    x += renderingCamera.x;
    x /= renderingCamera.cellSize;
    return x;
}

int tsc_cellMouseY() {
    double y = GetMouseY();
    y += renderingCamera.y;
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
    if(isGameTicking || !isGamePaused) return;
    long x = tsc_cellMouseX();
    long y = tsc_cellMouseY();

    if(x < 0 || y < 0 || x >= currentGrid->width || y >= currentGrid->height) return;

    if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        tsc_cell cell = tsc_cell_create(currentId, currentRot);
        for(int ox = -brushSize; ox <= brushSize; ox++) {
            for(int oy = -brushSize; oy <= brushSize; oy++) {
                tsc_grid_set(currentGrid, x + ox, y + oy, &cell);
            }
        }
    }
    
    if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        tsc_cell cell = tsc_cell_create(builtin.empty, 0);
        for(int ox = -brushSize; ox <= brushSize; ox++) {
            for(int oy = -brushSize; oy <= brushSize; oy++) {
                tsc_grid_set(currentGrid, x + ox, y + oy, &cell);
            }
        }
    }
}

void tsc_handleRenderInputs() {
    float delta = GetFrameTime();
    double speed = renderingCamera.speed;
    if(IsKeyDown(KEY_LEFT_SHIFT)) {
        speed *= 2;
    }
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

    if(IsKeyPressed(KEY_Q)) {
        currentRot--;
        while(currentRot < 0) currentRot += 4;
        printf("rot %d\n", (int)currentRot);
    }
    if(IsKeyPressed(KEY_E)) {
        currentRot++;
        currentRot %= 4;
        printf("rot %d\n", (int)currentRot);
    }

    if(IsKeyPressed(KEY_L)) {
        const char *clipboard = GetClipboardText();
        printf("Loading %s\n", clipboard);
        tsc_saving_decodeWithAny(clipboard, currentGrid);
    }

#define CELL_KEY(key, cell) if(IsKeyPressed(key)) currentId = cell

    CELL_KEY(KEY_KP_0, builtin.placeable);
    CELL_KEY(KEY_KP_1, builtin.generator);
    CELL_KEY(KEY_KP_2, builtin.rotator_cw);
    CELL_KEY(KEY_KP_3, builtin.rotator_ccw);
    CELL_KEY(KEY_KP_4, builtin.mover);
    CELL_KEY(KEY_KP_5, builtin.push);
    CELL_KEY(KEY_KP_6, builtin.slide);
    CELL_KEY(KEY_KP_7, builtin.enemy);
    CELL_KEY(KEY_KP_8, builtin.trash);
    CELL_KEY(KEY_KP_9, builtin.wall);

#undef CELL_KEY

    tsc_handleCellPlace();

    float mouseWheel = GetMouseWheelMove();
    if(mouseWheel > 0) {
        if(IsKeyDown(KEY_LEFT_CONTROL)) {
            brushSize++;
        } else {
            tsc_setZoomLevel(renderingCamera.cellSize * 2);
        }
    }
    if(mouseWheel < 0) {
        if(IsKeyDown(KEY_LEFT_CONTROL)) {
            if(brushSize > 0) brushSize--;
        } else {
            tsc_setZoomLevel(renderingCamera.cellSize / 2);
        }
    }

    if(IsKeyPressed(KEY_SPACE)) {
        isGamePaused = !isGamePaused;
    }

    if(!isGamePaused || isGameTicking) tickTime += delta;

    if(!isGamePaused && !multiTickPerFrame) {
        if(tickTime >= tickDelay && !isGameTicking) {
            tickTime = 0;
            tsc_signalUpdateShouldHappen();
        }
    }
}
