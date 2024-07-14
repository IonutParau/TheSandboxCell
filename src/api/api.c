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

void tsc_addButton(tsc_category *category, const char *icon, void (*click)(void *), void *payload) {
    if(category->itemc == category->itemcap) {
        category->itemcap *= 2;
        category->items = realloc(category->items, sizeof(tsc_categoryitem) * category->itemcap);
    }
    category->items[category->itemc].kind = TSC_CATEGORY_BUTTON;
    category->items[category->itemc].button.click = click;
    category->items[category->itemc].button.payload = payload;
    category->items[category->itemc].button.icon = tsc_strintern(icon);
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
    if(clipboard == NULL) {
        tsc_sound_play(builtin.audio.explosion);
    } else {
        printf("Loading %s\n", clipboard);
        tsc_saving_decodeWithAny(clipboard, currentGrid);
    }
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
    tsc_addButton(tools, "opengl ftw", tsc_loadButton, NULL);
    tsc_addButton(tools, "generator", tsc_setInitial, NULL);
    tsc_addButton(tools, "rotator_cw", tsc_restoreInitial, NULL);

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
