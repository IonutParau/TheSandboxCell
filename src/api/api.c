#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "../cells/grid.h"
#include "../utils.h"
#include <raylib.h>
#include "../saving/saving.h"
#include "../graphics/resources.h"
#include "../cells/ticking.h"

typedef struct tsc_splash_t {
    const char *splash;
    double weight;
} tsc_splash_t;

typedef struct tsc_splashes_t {
    tsc_splash_t *arr;
    size_t len;
    double sum;
} tsc_splashes_t;

tsc_splashes_t tsc_splashes = {NULL, 0, 0.0};

void tsc_addSplash(const char *splash, double weight) {
    size_t idx = tsc_splashes.len++;
    tsc_splashes.arr = realloc(tsc_splashes.arr, sizeof(tsc_splash_t) * tsc_splashes.len);
    tsc_splashes.arr[idx].splash = splash;
    tsc_splashes.arr[idx].weight = weight;
    tsc_splashes.sum += weight;
}

void tsc_addCoreSplashes() {
    tsc_addSplash("When in doubt, nuke it", 2);
    tsc_addSplash("Optimized by nerds for nerds", 0.5);
    tsc_addSplash("tim travl", 1);
    tsc_addSplash("indev is crap!!!", 1);
    tsc_addSplash("L I G H T S P E E D", 5);
    tsc_addSplash("Never gonna give you up", 0.01);
#ifdef TSC_TURBO
    tsc_addSplash("Who needs mods when you have speed", 10);
#endif
    tsc_addSplash("Check out Create Mod", 1);
    tsc_addSplash("tpc bad", 0.25);
    tsc_addSplash("cellua bad", 0.25);
    tsc_addSplash("kell buggy", 0.25);
    tsc_addSplash("yo my name is Jeremy", 0.25);
    tsc_addSplash("Pesky Windows", 0.25);
    tsc_addSplash("Photon dev helped", 0.25);
    tsc_addSplash("go suck an egg with your small brain", 0.25);
    tsc_addSplash("PhoenixCM", 0.25);
    tsc_addSplash("Program in C", 0.25);
    tsc_addSplash("No memory leaks ever seen", 0.25);
}

const char *tsc_randomSplash() {
    double x = (double)rand() / (double)(RAND_MAX);
    x *= tsc_splashes.sum;
    for(size_t i = 0; i < tsc_splashes.len; i++) {
        if(x < tsc_splashes.arr[i].weight) {
            return tsc_splashes.arr[i].splash;
        }
        x -= tsc_splashes.arr[i].weight;
    }
    return "Splash screen bugged bruh";
}

const char *currentMod = NULL;

void tsc_loadMod(const char *id) {
    currentMod = id;
    tsc_initMod(id); // Actually initialize it with the modloader
}

const char *tsc_padWithModID(const char *id) {
    static char buffer[1024];
    if(currentMod == NULL) {
        snprintf(buffer, 1024, "%s", id);
    } else {
        snprintf(buffer, 1024, "%s:%s", currentMod, id);
    }
    return buffer;
}

bool tsc_hasMod(const char *id) {
    // TODO: stop lying
    return false;
}

const char *tsc_currentModID() {
    return currentMod;
}

const char *tsc_registerCell(const char *id, const char *name, const char *description) {
    const char *actualID = tsc_strintern(tsc_padWithModID(id));
    return actualID;
}

tsc_category *rootCategory;

tsc_category *tsc_rootCategory() {
    if(rootCategory == NULL) {
        rootCategory = tsc_newCategory("Root", "Why are you looking at the description of root", builtin.trash);
    }
    return rootCategory;
}

tsc_category *tsc_newCategory(const char *title, const char *description, const char *icon) {
    tsc_category *category = malloc(sizeof(tsc_category));
    category->title = title;
    category->description = description;
    category->icon = icon;
    category->itemc = 0;
    category->itemcap = 20;
    category->items = malloc(sizeof(tsc_categoryitem) * category->itemcap);
    category->parent = NULL;
    category->open = false;
    return category;
}

void tsc_addCategory(tsc_category *category, tsc_category *toAdd) {
    if(toAdd->parent != NULL) {
        fprintf(stderr, "Category %s was already in %s, but then was also added to %s", toAdd->title, toAdd->parent->title, category->title);
        exit(1);
    }
    toAdd->parent = category;
    if(category->itemc == category->itemcap) {
        category->itemcap *= 2;
        category->items = realloc(category->items, sizeof(tsc_categoryitem) * category->itemcap);
    }
    category->items[category->itemc].kind = TSC_CATEGORY_SUBCATEGORY;
    category->items[category->itemc].category = toAdd;
    category->itemc++;
}

void tsc_addCell(tsc_category *category, const char *cell) {
    if(category->itemc == category->itemcap) {
        category->itemcap *= 2;
        category->items = realloc(category->items, sizeof(tsc_categoryitem) * category->itemcap);
    }
    category->items[category->itemc].kind = TSC_CATEGORY_CELL;
    category->items[category->itemc].cellID = cell;
    category->itemc++;
}

void tsc_addButton(tsc_category *category, const char *icon, const char *name, const char *desc, void (*click)(void *), void *payload) {
    if(category->itemc == category->itemcap) {
        category->itemcap *= 2;
        category->items = realloc(category->items, sizeof(tsc_categoryitem) * category->itemcap);
    }
    category->items[category->itemc].kind = TSC_CATEGORY_BUTTON;
    category->items[category->itemc].button.click = click;
    category->items[category->itemc].button.payload = payload;
    category->items[category->itemc].button.icon = tsc_strintern(icon);
    category->items[category->itemc].button.name = tsc_strintern(name);
    category->items[category->itemc].button.desc = tsc_strintern(desc);
    category->itemc++;
}

tsc_category *tsc_getCategory(tsc_category *category, const char *path) {
    static char part[256];
    bool final = false;
    for(size_t j = 0; ; j++) {
        part[j] = path[j];
        if(path[j] == '/') {
            part[j] = '\0';
            break;
        }
        if(path[j] == '\0') {
            final = true;
            break;
        }
    }
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind != TSC_CATEGORY_SUBCATEGORY) continue;
        tsc_category *child = category->items[i].category;
        if(tsc_streql(child->title, part)) {
            if(final) return child;
            return tsc_getCategory(child, path + strlen(part) + 1);
        }
    }

    return NULL;
}

void tsc_openCategory(tsc_category *category) {
start:
    if(category == NULL) return;
    if(category->open) return;
    category->open = true;
    category = category->parent;
    goto start;
}

void tsc_closeCategory(tsc_category *category) {
    if(!category->open) return;
    category->open = false;
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].kind == TSC_CATEGORY_SUBCATEGORY) tsc_closeCategory(category->items[i].category);
    }
}

static void tsc_loadButton(void *_) {
    const char *clipboard = GetClipboardText();
    if(clipboard == NULL || strlen(clipboard) == 0) {
        tsc_sound_play(builtin.audio.explosion);
    } else {
        printf("Loading %s\n", clipboard);
        tsc_saving_decodeWithAny(clipboard, currentGrid);
    }
}

static void tsc_saveButton(void *_) {
    tsc_saving_buffer buffer = tsc_saving_newBuffer("");
    tsc_saving_encodeWithSmallest(&buffer, currentGrid);
    if(buffer.len == 0) {
        tsc_sound_play(builtin.audio.explosion);
    }
    SetClipboardText(buffer.mem);
    tsc_saving_deleteBuffer(buffer);
}

static void tsc_saveV3Button(void *_) {
    tsc_saving_buffer buffer = tsc_saving_newBuffer("");
    int success = tsc_saving_encodeWith(&buffer, currentGrid, "V3");
    if(success) {
    SetClipboardText(buffer.mem);
    } else {
        tsc_sound_play(builtin.audio.explosion);
    }
    tsc_saving_deleteBuffer(buffer);
}

static void tsc_restoreInitial(void *_) {
    if(isGameTicking) return;
    tsc_copyGrid(currentGrid, tsc_getGrid("initial"));
    isInitial = true;
}

static void tsc_setInitial(void *_) {
    if(isGameTicking) return;
    tsc_copyGrid(tsc_getGrid("initial"), currentGrid);
    isInitial = true;
}

void tsc_loadDefaultCellBar() {
    tsc_category *root = tsc_rootCategory();

    tsc_category *tools = tsc_newCategory("Tools", "Simple tools and buttons", "icon");
    tsc_addButton(tools, "save", "Save to Clipboard", "Saves the current grid to clipboard using the smallest format", tsc_saveButton, NULL);
    tsc_addButton(tools, "save_v3", "Save to V3", "Saves the current grid using V3, which is supported by a lot more remakes. This can fail.", tsc_saveV3Button, NULL);
    tsc_addButton(tools, "load", "Load from Clipboard", "Load a level code from clipboard", tsc_loadButton, NULL);
    tsc_addButton(tools, "generator", "Set Initial", "Set the current grid state as the initial one", tsc_setInitial, NULL);
    tsc_addButton(tools, "rotator_cw", "Restore Initial", "Restore the initial grid state", tsc_restoreInitial, NULL);

    tsc_addCategory(root, tools);
    tsc_addCell(root, builtin.mover);
    tsc_addCell(root, builtin.generator);
    tsc_addCell(root, builtin.push);
    tsc_addCell(root, builtin.slide);
    tsc_addCell(root, builtin.rotator_cw);
    tsc_addCell(root, builtin.rotator_ccw);
    tsc_addCell(root, builtin.wall);
    tsc_addCell(root, builtin.enemy);
    tsc_addCell(root, builtin.trash);
}
