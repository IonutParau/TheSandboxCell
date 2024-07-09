// Cell Categories + mod ID stuff.
// This file is very confusingly named, but fuck you

#include <stddef.h>
#include <stdbool.h>

// hideapi

void tsc_loadMod(const char *id);
const char *tsc_padWithModID(const char *id);

// hideapi

// Check if mod exists
bool tsc_hasMod(const char *id);

// In case the mod forgets
const char *tsc_currentModID();

const char *tsc_registerCell(const char *id, const char *name, const char *description);

typedef struct tsc_categoryitem {
    bool isCategory;
    union {
        const char *cellID;
        struct tsc_category *category;
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
tsc_category *tsc_getCategory(tsc_category *category, const char *path);

// hideapi
void tsc_openCategory(tsc_category *category);
void tsc_closeCategory(tsc_category *category);
void tsc_loadDefaultCellBar();
// hideapi
