// Cell Categories + mod ID stuff.
// This file is very confusingly named, but fuck you

#include <stddef.h>
#include <stdbool.h>

// hideapi

void tsc_loadMod(const char *id);
void tsc_initMod(const char *id);
void tsc_loadAllMods();
const char *tsc_padWithModID(const char *id);

// hideapi

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

// hideapi
void tsc_openCategory(tsc_category *category);
void tsc_closeCategory(tsc_category *category);
void tsc_loadDefaultCellBar();
// hideapi
