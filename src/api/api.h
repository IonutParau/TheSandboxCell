// Cell Categories + mod ID stuff.
// This file is very confusingly named, but fuck you

#ifndef TSC_API_H
#define TSC_API_H

#include <stddef.h>
#include <stdbool.h>
#include "value.h"

// hideapi

void tsc_loadMod(const char *id);
void tsc_initMod(const char *id);
void tsc_loadAllMods();
const char *tsc_padWithModID(const char *id);
void tsc_addCoreSplashes();

// hideapi

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

const char *tsc_registerCell(const char *id, const char *name, const char *description);

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
#define TSC_SETTING_INTEGER 3
#define TSC_SETTING_NUMBER 3

// hideapi
typedef struct tsc_setting {
    unsigned char kind;
    const char *id;
    const char *name;
    union {
        struct {
            const char **items;
            size_t len;
        } list;
        struct {
            float min;
            float max;
        } slider;
        struct {
            int min;
            int max;
        } integer;
        struct {
            float min;
            float max;
        } number;
    };
} tsc_setting;

typedef struct tsc_settingCategory {
    const char *id;
    const char *title;
    tsc_setting *settings;
    size_t settinglen;
    size_t settingcap;
} tsc_settingCategory;
// hideapi

typedef void tsc_settingCallback(const char *settingID);

tsc_category *tsc_rootCategory();
tsc_category *tsc_newCategory(const char *title, const char *description, const char *icon);
tsc_category *tsc_newCellGroup(const char *title, const char *description, const char *mainCell);
void tsc_addCategory(tsc_category *category, tsc_category *toAdd);
void tsc_addCell(tsc_category *category, const char *cell);
void tsc_addButton(tsc_category *category, const char *icon, const char *name, const char *description, void (*click)(void *), void *payload);
tsc_category *tsc_getCategory(tsc_category *category, const char *path);

// hideapi
void tsc_openCategory(tsc_category *category);
void tsc_closeCategory(tsc_category *category);
void tsc_loadDefaultCellBar();
// hideapi

tsc_value tsc_getSetting(const char *settingID);
const char *tsc_addSettingCategory(const char *settingCategoryID, const char *settingTitle);
const char *tsc_addSetting(const char *settingID, const char *name, const char *categoryID, unsigned char kind, void *data, tsc_settingCallback *callback);

// hideapi
extern tsc_settingCategory *tsc_settingCategories;
extern size_t tsc_settingLen;
extern tsc_value tsc_settingStore;

void tsc_loadSettings();
// hideapi

#endif
