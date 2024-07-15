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
    size_t lx;
    size_t ly;
    char rot;
    signed char addedRot;
    bool updated;
} tsc_cell;

typedef struct tsc_grid tsc_grid;

// Stores function pointers and payload for everything we might need from a modded cell.
// Can be used in place of ID comparison (technically) but it's not much of a benefit.
typedef struct tsc_celltable {
    void *payload;
    void (*update)(tsc_cell *cell, int x, int y, int ux, int uy, void *payload);
    int (*canMove)(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, void *payload);
} tsc_celltable;

tsc_celltable *tsc_cell_newTable(const char *id);
tsc_celltable *tsc_cell_getTable(tsc_cell *cell);

typedef struct tsc_audio_id_pool_t {
    const char *destroy;
    const char *explosion;
} tsc_audio_id_pool_t;

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
    tsc_audio_id_pool_t audio;
} tsc_cell_id_pool_t;

extern tsc_cell_id_pool_t builtin;


tsc_cell tsc_cell_create(const char *id, char rot);
tsc_cell tsc_cell_clone(tsc_cell *cell);
void tsc_cell_swap(tsc_cell *a, tsc_cell *b);
void tsc_cell_destroy(tsc_cell cell);
const char *tsc_cell_get(const tsc_cell *cell, const char *key);
void tsc_cell_set(tsc_cell *cell, const char *key, const char *value);
size_t tsc_cell_getFlags(tsc_cell *cell);
void tsc_cell_setFlags(tsc_cell *cell, size_t flags);

typedef struct tsc_grid {
    tsc_cell *cells;
    int width;
    int height;
    const char *title;
    const char *desc;
    size_t refc;
} tsc_grid;

typedef struct tsc_gridStorage {
    const char **id;
    tsc_grid **grid;
    size_t len;
} tsc_gridStorage;

extern tsc_grid *currentGrid;

tsc_grid *tsc_getGrid(const char *name);
tsc_grid *tsc_createGrid(const char *id, int width, int height, const char *title, const char *description);
void tsc_retainGrid(tsc_grid *grid);
void tsc_deleteGrid(tsc_grid *grid);
void tsc_switchGrid(tsc_grid *grid);
void tsc_copyGrid(tsc_grid *dest, tsc_grid *src);
void tsc_clearGrid(tsc_grid *grid, int width, int height);
void tsc_nukeGrids();

tsc_cell *tsc_grid_get(tsc_grid *grid, int x, int y);
void tsc_grid_set(tsc_grid *grid, int x, int y, tsc_cell *cell);
int tsc_grid_frontX(int x, char dir);
int tsc_grid_frontY(int y, char dir);
int tsc_grid_shiftX(int x, char dir, int amount);
int tsc_grid_shiftY(int y, char dir, int amount);

// Cell interactions 

int tsc_cell_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType);
float tsc_cell_getBias(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType);
int tsc_cell_canGenerate(tsc_grid *grid, tsc_cell *cell, int x, int y, tsc_cell *generator, int gx, int gy, char dir);
int tsc_cell_isTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *eating);
void tsc_cell_onTrash(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *eating);
int tsc_cell_isAcid(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *dissolving);
void tsc_cell_onAcid(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, tsc_cell *dissolving);

// Returns how many cells were pushed.
int tsc_grid_push(tsc_grid *grid, int x, int y, char dir, double force, tsc_cell *replacement);

#endif
#ifndef TSC_SUBTICKS_H
#define TSC_SUBTICKS_H

#include <stddef.h>

#define TSC_SUBMODE_TICKED 0
#define TSC_SUBMODE_TRACKED 1
#define TSC_SUBMODE_NEIGHBOUR 2

typedef struct tsc_subtick_t {
    const char **ids;
    size_t idc;
    const char *name;
    void *payload;
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

void tsc_subtick_add(tsc_subtick_t subtick);
tsc_subtick_t *tsc_subtick_find(const char *name);
void tsc_subtick_addCore();
void tsc_subtick_run();

#endif
#ifndef TSC_SAVING_H
#define TSC_SAVING_H

#include <stddef.h>

typedef struct tsc_saving_buffer {
    char *mem;
    size_t len;
    size_t cap;
} tsc_saving_buffer;

tsc_saving_buffer tsc_saving_newBuffer(const char *initial);
void tsc_saving_deleteBuffer(tsc_saving_buffer buffer);
void tsc_saving_write(tsc_saving_buffer *buffer, char ch);
void tsc_saving_writeStr(tsc_saving_buffer *buffer, const char *str);
void tsc_saving_writeFormat(tsc_saving_buffer *buffer, const char *fmt, ...);
void tsc_saving_writeBytes(tsc_saving_buffer *buffer, const char *mem, size_t count);

typedef int tsc_saving_encoder(tsc_saving_buffer *buffer, tsc_grid *grid);
typedef void tsc_saving_decoder(const char *code, tsc_grid *grid);

typedef struct tsc_saving_format {
    const char *name;
    const char *header;
    tsc_saving_encoder *encode;
    tsc_saving_decoder *decode;
} tsc_saving_format;

int tsc_saving_encodeWith(tsc_saving_buffer *buffer, tsc_grid *grid, const char *name);
void tsc_saving_encodeWithSmallest(tsc_saving_buffer *buffer, tsc_grid *grid);
void tsc_saving_decodeWith(const char *code, tsc_grid *grid, const char *name);
void tsc_saving_decodeWithAny(const char *code, tsc_grid *grid);

void tsc_saving_register(tsc_saving_format format);
void tsc_saving_registerCore();

#endif
#ifndef TSC_RESOURCES_H
#define TSC_RESOURCES_H

// WIP

#include <raylib.h>
#include <stddef.h>

// These are for the implementation.
// You likely won't care.

typedef struct tsc_resourcearray {
    size_t len;
    const char **ids;
    void *memory;
    size_t itemsize;
} tsc_resourcearray;

typedef struct tsc_resourcetable {
    size_t arrayc;
    tsc_resourcearray *arrays;
    size_t *hashes;
    size_t itemsize;
} tsc_resourcetable;

// Actual API

typedef struct tsc_resourcepack {
    const char *id;
    const char *name;
    const char *description;
    const char *author;
    const char *readme;
    const char *license;
    tsc_resourcetable *textures;
    tsc_resourcetable *audio;
    Font *font;
} tsc_resourcepack;

// This is highly important shit
// If something isn't found, it is yoinked from here.
// If somehting isn't found here, then you'll likely get segfaults (or errors).
extern tsc_resourcepack *defaultResourcePack;
tsc_resourcepack *tsc_getResourcePack(const char *id);


const char *tsc_textures_load(tsc_resourcepack *pack, const char *id, const char *file);
const char *tsc_sound_load(tsc_resourcepack *pack, const char *id, const char *file);
void tsc_music_load(const char *name, const char *file);

void tsc_sound_play(const char *id);


#endif
#ifndef TSC_UI_H
#define TSC_UI_H

#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>

#define WIDTH(x) (((x) / 100.0) * GetScreenWidth())
#define HEIGHT(y) (((y) / 100.0) * GetScreenHeight())

typedef struct ui_frame ui_frame;

#define UI_BUTTON_CLICK 1
#define UI_BUTTON_PRESS 2
#define UI_BUTTON_LONGPRESS 3

typedef struct ui_button {
    float pressTime;
    float longPressTime;
    Color color;
    int shrinking;
    bool wasClicked;
    bool clicked;
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

// Macros
#define tsc_ui_row(body) {tsc_ui_pushRow(); body; tsc_ui_finishRow();}
#define tsc_ui_column(body) {tsc_ui_pushColumn(); body; tsc_ui_finishColumn();}
#define tsc_ui_stack(body) {tsc_ui_pushStack(); body; tsc_ui_finishStack();}

#endif
#ifndef TSC_THREAD_WORKERS
#define TSC_THREAD_WORKERS

#include <stddef.h>

typedef void (worker_task_t)(void *data);

void workers_addTask(worker_task_t *task, void *data);
void workers_waitForTasks(worker_task_t *task, void **dataArr, size_t len);
void workers_waitForTasksFlat(worker_task_t *task, void *dataArr, size_t dataSize, size_t len);
int workers_amount();

#endif
#ifndef TSC_UTILS
#define TSC_UTILS

#include <stddef.h>

const char *tsc_strintern(const char *str);
int tsc_streql(const char *a, const char *b);
char *tsc_strdup(const char *str);
char *tsc_strcata(const char *a, const char *b);
unsigned long tsc_strhash(const char *str);

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

#endif
// Cell Categories + mod ID stuff.
// This file is very confusingly named, but fuck you

#include <stddef.h>
#include <stdbool.h>


void tsc_addSplash(const char *splash, double weight);
const char *tsc_randomSplash();

// Check if mod exists
bool tsc_hasMod(const char *id);

// In case the mod forgets
const char *tsc_currentModID();

const char *tsc_registerCell(const char *id, const char *name, const char *description);

typedef struct tsc_cellbutton {
    void *payload;
    void (*click)(void *payload);
    const char *icon;
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
} tsc_category;

tsc_category *tsc_rootCategory();
tsc_category *tsc_newCategory(const char *title, const char *description, const char *icon);
void tsc_addCategory(tsc_category *category, tsc_category *toAdd);
void tsc_addCell(tsc_category *category, const char *cell);
void tsc_addButton(tsc_category *category, const char *icon, void (*click)(void *), void *payload);
tsc_category *tsc_getCategory(tsc_category *category, const char *path);

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
void tsc_retain(tsc_value value);
void tsc_destroy(tsc_value value);

tsc_value tsc_index(tsc_value list, size_t index);
void tsc_setIndex(tsc_value list, size_t index, tsc_value value);
tsc_value tsc_getKey(tsc_value object, const char *key);
void tsc_setKey(tsc_value object, const char *key, tsc_value value);

bool tsc_isInt(tsc_value value);
bool tsc_isNumber(tsc_value value);
bool tsc_isNumerical(tsc_value value);
bool tsc_isBoolean(tsc_value value);
bool tsc_isString(tsc_value value);
bool tsc_isArray(tsc_value value);
bool tsc_isObject(tsc_value value);

int64_t tsc_toInt(tsc_value value);
double tsc_toNumber(tsc_value value);
bool tsc_toBoolean(tsc_value value);
const char *tsc_toString(tsc_value value);
const char *tsc_toLString(tsc_value value, size_t *len);
size_t tsc_getLength(tsc_value value);
const char *tsc_keyAt(tsc_value object, size_t index);

#endif
#ifdef __cplusplus
}
#endif
