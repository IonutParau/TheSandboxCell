// MIT License
// 
// Copyright 2024 Pârău Ionuț Alexandru
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifdef __cplusplus
extern "C" {
#endif
#ifndef TSC_GRID_H
#define TSC_GRID_H

#include <stddef.h>
#include <stdbool.h>

typedef struct tsc_cellreg {
    const char **keys;
    char **values;
    size_t len;
} tsc_cellreg;

typedef struct tsc_cell {
    const char *id;
    const char *texture;
    tsc_cellreg *data;
    struct tsc_celltable *celltable;
    size_t flags;
    int lx;
    int ly;
    char rot;
    signed char addedRot;
    bool updated;
} tsc_cell;

typedef struct tsc_grid tsc_grid;

#define TSC_FLAGS_PLACEABLE 1

// Stores function pointers and payload for everything we might need from a modded cell.
// Can be used in place of ID comparison (technically) but it's not much of a benefit.
typedef struct tsc_celltable {
    void *payload;
    void (*update)(tsc_cell *cell, int x, int y, int ux, int uy, void *payload);
    int (*canMove)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, void *payload);
    char *(*signal)(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy, void *payload);
    size_t flags;
    float (*getBias)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, void *payload);
    int (*canGenerate)(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir, void *payload);
    int (*isTrash)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating, void *payload);
    void (*onTrash)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating, void *payload);
    int (*isAcid)(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy, void *payload);
    void (*onAcid)(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy, void *payload);
} tsc_celltable;

tsc_celltable *tsc_cell_newTable(const char *id);
tsc_celltable *tsc_cell_getTable(tsc_cell *cell);
size_t tsc_cell_getTableFlags(tsc_cell *cell);

typedef struct tsc_texture_id_pool_t {
    const char *icon;
    const char *copy;
    const char *cut;
    const char *del;
    const char *setinitial;
    const char *restoreinitial;
} tsc_texture_id_pool_t;

typedef struct tsc_audio_id_pool_t {
    const char *destroy;
    const char *explosion;
    const char *move;
} tsc_audio_id_pool_t;

typedef struct tsc_optimization_id_pool_t {
    size_t gens[4];
} tsc_optimization_id_pool_t;

typedef struct tsc_setting_id_pool_t {
    const char *vsync;
    const char *fullscreen;
    const char *threadCount;
    const char *savingFormat;
    const char *musicVolume;
    const char *sfxVolume;
    const char *unfocusedVolume;
    const char *updateDelay;
    const char *mtpf;
} tsc_setting_id_pool_t;

typedef struct tsc_id_pool_t {
    const char *empty;
    const char *placeable;
    const char *mover;
    const char *generator;
    const char *push;
    const char *slide;
    const char *rotator_cw;
    const char *rotator_ccw;
    const char *enemy;
    const char *trash;
    const char *wall;
    tsc_texture_id_pool_t textures;
    tsc_audio_id_pool_t audio;
    tsc_optimization_id_pool_t optimizations;
    tsc_setting_id_pool_t settings;
} tsc_cell_id_pool_t;

extern tsc_cell_id_pool_t builtin;


tsc_cell tsc_cell_create(const char *id, char rot);
tsc_cell tsc_cell_clone(tsc_cell *cell);
void tsc_cell_swap(tsc_cell *a, tsc_cell *b);
void tsc_cell_destroy(tsc_cell cell);
const char *tsc_cell_get(const tsc_cell *cell, const char *key);
const char *tsc_cell_nthKey(const tsc_cell *cell, size_t idx);
void tsc_cell_set(tsc_cell *cell, const char *key, const char *value);
void tsc_cell_rotate(tsc_cell *cell, signed char amount);

typedef struct tsc_grid {
    tsc_cell *cells;
    tsc_cell *bgs;
    int width;
    int height;
    const char *title;
    const char *desc;
    size_t refc;
    bool *chunkdata;
    int chunkwidth;
    int chunkheight;
    char *optData;
} tsc_grid;

typedef struct tsc_gridStorage {
    const char **id;
    tsc_grid **grid;
    size_t len;
} tsc_gridStorage;

extern tsc_grid *currentGrid;
extern int tsc_maxSliceSize;

tsc_grid *tsc_getGrid(const char *name);
tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description);
void tsc_retainGrid(tsc_grid *grid);
void tsc_deleteGrid(tsc_grid *grid);
void tsc_switchGrid(tsc_grid *grid);
void tsc_copyGrid(tsc_grid *dest, tsc_grid *src);
void tsc_clearGrid(tsc_grid *grid, int width, int height);
void tsc_nukeGrids();

size_t tsc_allocOptimization(const char *id);
size_t tsc_findOptimization(const char *trueID);
size_t tsc_defineEffect(const char *id);
size_t tsc_findEffect(const char *trueID);

size_t tsc_optSize();
size_t tsc_effectSize();

tsc_cell *tsc_grid_get(tsc_grid *grid, int x, int y);
void tsc_grid_set(tsc_grid *grid, int x, int y, tsc_cell *cell);
tsc_cell *tsc_grid_background(tsc_grid *grid, int x, int y);
void tsc_grid_setBackground(tsc_grid *grid, int x, int y, tsc_cell *cell);
int tsc_grid_frontX(int x, char dir);
int tsc_grid_frontY(int y, char dir);
int tsc_grid_shiftX(int x, char dir, int amount);
int tsc_grid_shiftY(int y, char dir, int amount);
void tsc_grid_enableChunk(tsc_grid *grid, int x, int y);
void tsc_grid_disableChunk(tsc_grid *grid, int x, int y);
bool tsc_grid_checkChunk(tsc_grid *grid, int x, int y);
bool tsc_grid_checkRow(tsc_grid *grid, int y);
bool tsc_grid_checkColumn(tsc_grid *grid, int x);
bool tsc_grid_checkOptimization(tsc_grid *grid, int x, int y, size_t optimization);
void tsc_grid_setOptimization(tsc_grid *grid, int x, int y, size_t optimization, bool enabled);

// Cell interactions

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force);
float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force);
int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir);
int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating);
void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, tsc_cell *eating);
int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy);
void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, char dir, const char *forceType, double force, tsc_cell *dissolving, int dx, int dy);
char *tsc_cell_signal(tsc_cell *cell, int x, int y, const char *protocol, const char *data, tsc_cell *sender, int sx, int sy);

// Returns how many cells were pushed.
int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);
// Returns how many cells were pulled.
int tsc_grid_pull(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);
// Returns how many cells were grabbed.
int tsc_grid_grab(tsc_grid *grid, int x, int y, char dir, char side, double force, tsc_cell *replacement);
// Returns whether the cell was nudged.
bool tsc_grid_nudge(tsc_grid *grid, int x, int y, char dir, tsc_cell *replacement);

#endif
#ifndef TSC_SUBTICKS_H
#define TSC_SUBTICKS_H

#include <stddef.h>
#include <stdbool.h>

#define TSC_SUBMODE_TICKED 0
#define TSC_SUBMODE_TRACKED 1
#define TSC_SUBMODE_NEIGHBOUR 2
#define TSC_SUBMODE_CUSTOM 3

typedef struct tsc_subtick_custom_order {
    int order;
    int rotc;
    int *rots;
} tsc_subtick_custom_order;

typedef struct tsc_subtick_t {
    const char **ids;
    size_t idc;
    const char *name;
    union {
        tsc_subtick_custom_order **customOrder;
    };
    double priority;
    char mode;
    char parallel;
    char spacing;
} tsc_subtick_t;

typedef struct tsc_updateinfo_t {
    tsc_subtick_t *subtick;
    int x;
    char rot;
} tsc_updateinfo_t;

typedef struct tsc_subtick_manager_t {
    struct tsc_subtick_t *subs;
    size_t subc;
} tsc_subtick_manager_t;

extern tsc_subtick_manager_t subticks;

tsc_subtick_t *tsc_subtick_add(tsc_subtick_t subtick);
void tsc_subtick_addCell(tsc_subtick_t *subtick, const char *id);
tsc_subtick_t *tsc_subtick_find(const char *name);
tsc_subtick_t *tsc_subtick_addTicked(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addTracked(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addNeighbour(const char *name, double priority, char spacing, bool parallel);
tsc_subtick_t *tsc_subtick_addCustom(const char *name, double priority, char spacing, bool parallel, tsc_subtick_custom_order *orders, size_t orderc);
void tsc_subtick_addCore();
void tsc_subtick_run();

#endif
#ifndef TSC_SAVING_H
#define TSC_SAVING_H

#include <stddef.h>

typedef struct tsc_buffer {
    char *mem;
    size_t len;
    size_t cap;
} tsc_buffer;

// Legacy alias
typedef tsc_buffer tsc_saving_buffer;

tsc_buffer tsc_saving_newBuffer(const char *initial);
tsc_buffer tsc_saving_newBufferCapacity(const char *initial, size_t capacity);
void tsc_saving_deleteBuffer(tsc_buffer buffer);
void tsc_saving_write(tsc_buffer *buffer, char ch);
void tsc_saving_writeStr(tsc_buffer *buffer, const char *str);
void __attribute__((format (printf, 2, 3))) tsc_saving_writeFormat(tsc_buffer *buffer, const char *fmt, ...);
void tsc_saving_writeBytes(tsc_buffer *buffer, const char *mem, size_t count);

typedef int tsc_saving_encoder(tsc_buffer *buffer, tsc_grid *grid);
typedef void tsc_saving_decoder(const char *code, tsc_grid *grid);

typedef struct tsc_saving_format {
    const char *name;
    const char *header;
    tsc_saving_encoder *encode;
    tsc_saving_decoder *decode;
} tsc_saving_format;

int tsc_saving_encodeWith(tsc_buffer *buffer, tsc_grid *grid, const char *name);
void tsc_saving_encodeWithSmallest(tsc_buffer *buffer, tsc_grid *grid);
void tsc_saving_decodeWith(const char *code, tsc_grid *grid, const char *name);
const char *tsc_saving_identify(const char *code);
void tsc_saving_decodeWithAny(const char *code, tsc_grid *grid);

void tsc_saving_register(tsc_saving_format format);
void tsc_saving_registerCore();

#endif
#ifndef TSC_RESOURCES_H
#define TSC_RESOURCES_H

#include <raylib.h>
#include <stddef.h>


typedef struct tsc_resourcepack
tsc_resourcepack;

// This is highly important shit
// If something isn't found, it is yoinked from here.
// If something isn't found here, then you'll likely get segfaults (or errors).
extern tsc_resourcepack *defaultResourcePack;
tsc_resourcepack *tsc_getResourcePack(const char *id);


const char *tsc_textures_load(tsc_resourcepack *pack, const char *id, const char *file);
const char *tsc_sound_load(tsc_resourcepack *pack, const char *id, const char *file);

void tsc_sound_play(const char *id);


#endif
#ifndef TSC_UI_H
#define TSC_UI_H

#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>

#define WIDTH(x) (((double)(x) / 100.0) * GetScreenWidth())
#define HEIGHT(y) (((double)(y) / 100.0) * GetScreenHeight())

extern const char *tsc_currentMenu;

typedef struct ui_frame ui_frame;

#define UI_BUTTON_CLICK 1
#define UI_BUTTON_PRESS 2
#define UI_BUTTON_LONGPRESS 3
#define UI_BUTTON_RIGHTCLICK 4
#define UI_BUTTON_RIGHTPRESS 5
#define UI_BUTTON_RIGHTLONGPRESS 6
#define UI_BUTTON_HOVER 7

typedef struct ui_button {
    float pressTime;
    float longPressTime;
    Color color;
    int shrinking;
    bool wasClicked;
    bool clicked;
    bool rightClick;
    bool hovered;
} ui_button;

#define UI_INPUT_DEFAULT 0
// Ideal for sizes. Only allows 0123456789
#define UI_INPUT_SIZE 1
// Like SIZE, but it allows a single .
#define UI_INPUT_FLOAT 2
#define UI_INPUT_SIGNED 4

typedef struct ui_input {
    char *text;
    size_t textlen;
    const char *placeholder;
    size_t maxlen;
    size_t flags;
} ui_input;

typedef struct ui_slider {
    double value;
    double min;
    double max;
    size_t segments;
} ui_slider;

typedef struct ui_scrollable {
    RenderTexture texture;
    int width;
    int height;
    int offx;
    int offy;
} ui_scrollable;

ui_frame *tsc_ui_newFrame();
void tsc_ui_destroyFrame(ui_frame *frame);
void tsc_ui_pushFrame(ui_frame *frame);
// like tsc_ui_pushFrame, but it does not "clear" what was before.
// This is useful if building the UI is slow, as you can do so conditionally.
// TSC_UI is meant to be a mix of immediate-mode and traditional component tree UIs, and this function is that bridge.
void tsc_ui_bringBackFrame(ui_frame *frame);
void tsc_ui_reset();
void tsc_ui_popFrame();

// Do everything
void tsc_ui_update(double delta);
void tsc_ui_render();
int tsc_ui_absorbedPointer(int x, int y);

// Containers
void tsc_ui_pushRow();
void tsc_ui_finishRow();
void tsc_ui_pushColumn();
void tsc_ui_finishColumn();
void tsc_ui_pushStack();
void tsc_ui_finishStack();

// States
ui_button *tsc_ui_newButtonState();
ui_input *tsc_ui_newInputState(const char *placeholder, size_t maxlen, size_t flags);
ui_slider *tsc_ui_newSliderState(double min, double max);
ui_scrollable *tsc_ui_newScrollableState();

void tsc_ui_clearButtonState(ui_button *button);
void tsc_ui_clearInputState(ui_input *input);
void tsc_ui_clearSliderState(ui_slider *slider);
void tsc_ui_clearScrollableState(ui_scrollable *scrollable);

// Stateful components
int tsc_ui_button(ui_button *state);
int tsc_ui_checkbutton(ui_button *state);
const char *tsc_ui_input(ui_input *state, int width, int height);
double tsc_ui_slider(ui_slider *state, int width, int height, int thickness);

// Stateless components

void tsc_ui_image(const char *id, int width, int height);
void tsc_ui_text(const char *text, int size, Color color);
void tsc_ui_space(int amount);

// Stateless components with a child

void tsc_ui_scrollable(int width, int height);
void tsc_ui_translate(int x, int y);
void tsc_ui_pad(int x, int y);
void tsc_ui_align(float x, float y, int width, int height);
void tsc_ui_center(int width, int height);
void tsc_ui_box(Color background);
void tsc_ui_tooltip(const char *name, int nameSize, const char *description, int descSize, int maxLineLen);

// Macros
#define tsc_ui_row(body) do {tsc_ui_pushRow(); body; tsc_ui_finishRow();} while(0)
#define tsc_ui_column(body) do {tsc_ui_pushColumn(); body; tsc_ui_finishColumn();} while(0)
#define tsc_ui_stack(body) do {tsc_ui_pushStack(); body; tsc_ui_finishStack();} while(0)

#endif
#ifndef TSC_THREAD_WORKERS
#define TSC_THREAD_WORKERS

#include <stddef.h>
#include <stdbool.h>

typedef void (worker_task_t)(void *data);

void workers_addTask(worker_task_t *task, void *data);
void workers_waitForTasks(worker_task_t *task, void **dataArr, size_t len);
void workers_waitForTasksFlat(worker_task_t *task, void *dataArr, size_t dataSize, size_t len);
int workers_amount();
void workers_setAmount(int newCount);
bool workers_isDisabled();

#endif
#ifndef TSC_UTILS
#define TSC_UTILS

#include <stddef.h>
#include <stdbool.h>

// Based off https://stackoverflow.com/questions/5919996/how-to-detect-reliably-mac-os-x-ios-linux-windows-in-c-preprocessor
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
        #define TSC_WINDOWS
   #else
        #error "Windows 32-bit is not supported"
   #endif
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        #error "iPhone Emulators are not supported"
    #elif TARGET_OS_MACCATALYST
        // I guess?
        #define TSC_MACOS
    #elif TARGET_OS_IPHONE
        #error "iPhone are not supported"
    #elif TARGET_OS_MAC
        #define TSC_MACOS
    #else
        #error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    #error "Android is not supported"
#elif __linux__
    #define TSC_LINUX
#endif

#if __unix__ // all unices not caught above
    // Unix
    #define TSC_UNIX
    #define TSC_POSIX
#elif defined(_POSIX_VERSION)
    // POSIX
    #define TSC_POSIX
#endif

// x64 gives us 64 bit pointers, but in virtual memory, they become 48-bit pointers.
// 12 bits for a 4kb index within a page, 4 indexes 9 bits each which tell the CPU how to traverse the page table.
// The remaining 16 bits must be empty in case the 5th index is ever implemented, but it can be used to store extra info.
#if defined(__x86_64) || defined(_M_X64)
    #define TSC_PTR_HAS_SHORT 1
#else
    #define TSC_PTR_HAS_SHORT 0
#endif

const char *tsc_strintern(const char *str);
int tsc_streql(const char *a, const char *b);
char *tsc_strdup(const char *str);
char *tsc_strcata(const char *a, const char *b);
unsigned long tsc_strhash(const char *str);

void tsc_memswap(void *a, void *b, size_t len);

// Replaces the / with \ on Windows
void tsc_pathfix(char *path);
const char *tsc_pathfixi(const char *path);
char tsc_pathsep();

char *tsc_allocfile(const char *path, size_t *len);
void tsc_freefile(char *memory);
int tsc_hasfile(const char *path);
// You must free path. The extension comes out of that memory.
// It replaces the first . if any with a null terminator.
// NULL if there is no extension.
const char *tsc_fextension(char *path);

char **tsc_dirfiles(const char *path, size_t *len);
void tsc_freedirfiles(char **dirfiles);

void *tsc_malloc(size_t len);
void *tsc_realloc(void *buffer, size_t len);
void tsc_free(void *buffer);

bool tsc_getBit(char *num, size_t bit);
void tsc_setBit(char *num, size_t bit, bool value);

unsigned char tsc_getUnusedPointerByte(void *pointer);
void *tsc_getPointerWithoutByte(void *pointer);
void *tsc_setUnusedPointerByte(void *pointer, unsigned char byte);
// x64 only
unsigned short tsc_getUnusedPointerShort(void *pointer);
void *tsc_getPointerWithoutShort(void *pointer);
// x64 only
void *tsc_setUnusedPointerShort(void *pointer, unsigned short byte);

#ifndef TSC_POSIX

int asprintf(char **s, const char *fmt, ...);

#endif

#endif
#ifndef TSC_VALUE_H
#define TSC_VALUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Signal values

#define TSC_VALUE_NULL 0
#define TSC_VALUE_INT 1
#define TSC_VALUE_NUMBER 2
#define TSC_VALUE_BOOL 3
#define TSC_VALUE_STRING 4
#define TSC_VALUE_CSTRING 5
#define TSC_VALUE_ARRAY 6
#define TSC_VALUE_OBJECT 7
#define TSC_VALUE_CELLPTR 8
#define TSC_VALUE_OWNEDCELL 9

typedef struct tsc_value tsc_value;

typedef struct tsc_string_t {
    size_t refc;
    char *memory;
    size_t len;
} tsc_string_t;

typedef struct tsc_array_t {
    size_t refc;
    tsc_value *values;
    size_t valuec;
} tsc_array_t;

typedef struct tsc_object_t {
    size_t refc;
    tsc_value *values;
    char **keys;
    size_t len;
} tsc_object_t;

typedef struct tsc_ownedcell_t {
    size_t refc;
    tsc_cell cell;
} tsc_ownedcell_t;

typedef struct tsc_value {
    size_t tag;
    union {
        int64_t integer;
        double number;
        bool boolean;
        tsc_string_t *string;
        const char *cstring;
        tsc_array_t *array;
        tsc_object_t *object;
        tsc_cell *cellptr;
        tsc_ownedcell_t *ownedcell;
    };
} tsc_value;

tsc_value tsc_null();
tsc_value tsc_int(int64_t integer);
tsc_value tsc_number(double number);
tsc_value tsc_boolean(bool boolean);
tsc_value tsc_string(const char *str);
tsc_value tsc_lstring(const char *str, size_t len);
tsc_value tsc_cstring(const char *str);
tsc_value tsc_array(size_t len);
tsc_value tsc_object();
tsc_value tsc_cellPtr(tsc_cell *cell);
tsc_value tsc_ownedCell(tsc_cell *cell);
void tsc_retain(tsc_value value);
void tsc_destroy(tsc_value value);

void tsc_ensureArgs(tsc_value args, int min);
void tsc_varArgs(tsc_value args, int min);
void tsc_append(tsc_value list, tsc_value value);

tsc_value tsc_index(tsc_value list, size_t index);
void tsc_setIndex(tsc_value list, size_t index, tsc_value value);
tsc_value tsc_getKey(tsc_value object, const char *key);
void tsc_setKey(tsc_value object, const char *key, tsc_value value);

bool tsc_isNull(tsc_value value);
bool tsc_isInt(tsc_value value);
bool tsc_isNumber(tsc_value value);
bool tsc_isNumerical(tsc_value value);
bool tsc_isBoolean(tsc_value value);
bool tsc_isString(tsc_value value);
bool tsc_isArray(tsc_value value);
bool tsc_isObject(tsc_value value);
bool tsc_isCellPtr(tsc_value cell);
bool tsc_isOwnedCell(tsc_value cell);
bool tsc_isCell(tsc_value cell);

int64_t tsc_toInt(tsc_value value);
double tsc_toNumber(tsc_value value);
bool tsc_toBoolean(tsc_value value);
const char *tsc_toString(tsc_value value);
const char *tsc_toLString(tsc_value value, size_t *len);
size_t tsc_getLength(tsc_value value);
const char *tsc_keyAt(tsc_value object, size_t index);
tsc_cell *tsc_toCell(tsc_value value);

typedef tsc_value (tsc_signal_t)(tsc_value args);

#define TSC_VALUE_UNION 1024
#define TSC_VALUE_OPTIONAL 1025
#define TSC_VALUE_TUPLE 1026

typedef struct tsc_typeinfo_t {
    size_t tag;
    union {
        struct tsc_typeinfo_t *child;
        struct {
            struct tsc_typeinfo_t *children;
            size_t childCount;
        };
        struct {
            const char **keys;
            struct tsc_typeinfo_t *values;
            size_t pairCount;
        };
    };
} tsc_typeinfo_t;

// TODO: define typeInfo
const char *tsc_setupSignal(const char *id, tsc_signal_t *signal, tsc_typeinfo_t *typeInfo);
tsc_typeinfo_t *tsc_getSignalInfo(const char *id);
tsc_signal_t *tsc_getSignal(const char *id);
tsc_value tsc_callSignal(tsc_signal_t *signal, tsc_value *argv, size_t argc);

#endif
// Cell Categories + mod ID stuff.
// This file is very confusingly named, but fuck you

#ifndef TSC_API_H
#define TSC_API_H

#include <stddef.h>
#include <stdbool.h>


typedef struct tsc_mod_t {
    const char *id;
    tsc_value value;
} tsc_mod_t;

void tsc_addSplash(const char *splash, double weight);
const char *tsc_randomSplash();

// Check if mod exists
bool tsc_hasMod(const char *id);
// Check if a mod exists and is loaded
bool tsc_hasLoadedMod(const char *id);

// In case the mod forgets
const char *tsc_currentModID();

typedef struct tsc_cellprofile_t {
    const char *id;
    const char *name;
    const char *desc;
} tsc_cellprofile_t;

const char *tsc_registerCell(const char *id, const char *name, const char *description);

tsc_cellprofile_t *tsc_getProfile(const char *id);

typedef struct tsc_cellbutton {
    void *payload;
    void (*click)(void *payload);
    const char *icon;
    const char *name;
    const char *desc;
} tsc_cellbutton;

#define TSC_CATEGORY_CELL 0
#define TSC_CATEGORY_SUBCATEGORY 1
#define TSC_CATEGORY_BUTTON 2

typedef struct tsc_categoryitem {
    char kind;
    union {
        const char *cellID;
        struct tsc_category *category;
        struct tsc_cellbutton button;
    };
} tsc_categoryitem;

typedef struct tsc_category {
    const char *title;
    const char *description;
    const char *icon;
    tsc_categoryitem *items;
    size_t itemc;
    size_t itemcap;
    struct tsc_category *parent;
    bool open;
    bool usedAsCell;
} tsc_category;

#define TSC_SETTING_TOGGLE 0
#define TSC_SETTING_LIST 1
#define TSC_SETTING_SLIDER 2
#define TSC_SETTING_INPUT 3

typedef void tsc_settingCallback(const char *settingID);


tsc_category *tsc_rootCategory();
tsc_category *tsc_newCategory(const char *title, const char *description, const char *icon);
tsc_category *tsc_newCellGroup(const char *title, const char *description, const char *mainCell);
void tsc_addCategory(tsc_category *category, tsc_category *toAdd);
void tsc_addCell(tsc_category *category, const char *cell);
void tsc_addButton(tsc_category *category, const char *icon, const char *name, const char *description, void (*click)(void *), void *payload);
tsc_category *tsc_getCategory(tsc_category *category, const char *path);


tsc_value tsc_getSetting(const char *settingID);
void tsc_setSetting(const char *settingID, tsc_value v);
const char *tsc_addSettingCategory(const char *settingCategoryID, const char *settingTitle);
const char *tsc_addSetting(const char *settingID, const char *name, const char *categoryID, unsigned char kind, void *data, tsc_settingCallback *callback);


#endif
#ifndef TSC_JSON_H
#define TSC_JSON_H


tsc_buffer tsc_json_encode(tsc_value value, tsc_buffer *err);
tsc_value tsc_json_decode(const char *text, tsc_buffer *err);

#endif
#ifdef __cplusplus
}
#endif
