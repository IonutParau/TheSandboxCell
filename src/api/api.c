#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "../utils.h"

const char *currentMod = NULL;

void tsc_loadMod(const char *id) {
    currentMod = id;
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

tsc_category *rootCategory;

tsc_category *tsc_rootCategory() {
    if(rootCategory == NULL) {
        rootCategory = tsc_newCategory("Root", "Why are you looking at the description of root");
    }
    return rootCategory;
}

tsc_category *tsc_newCategory(const char *title, const char *description) {
    tsc_category *category = malloc(sizeof(tsc_category));
    category->title = title;
    category->description = description;
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
    category->items[category->itemc].isCategory = true;
    category->items[category->itemc].category = toAdd;
    category->itemc++;
}

void tsc_addCell(tsc_category *category, const char *cell) {
    if(category->itemc == category->itemcap) {
        category->itemcap *= 2;
        category->items = realloc(category->items, sizeof(tsc_categoryitem) * category->itemcap);
    }
    category->items[category->itemc].isCategory = false;
    category->items[category->itemc].cellID = cell;
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
        if(!category->items[i].isCategory) continue;
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
    if(category->open) return;
    category->open = true;
    category = category->parent;
    goto start;
}

void tsc_closeCategory(tsc_category *category) {
    if(!category->open) return;
    category->open = false;
    for(size_t i = 0; i < category->itemc; i++) {
        if(category->items[i].isCategory) tsc_closeCategory(category->items[i].category);
    }
}
