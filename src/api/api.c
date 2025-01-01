#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "../threads/workers.h"
#include "../cells/grid.h"
#include "../utils.h"
#include <raylib.h>
#include "../saving/saving.h"
#include "../graphics/resources.h"
#include "../cells/ticking.h"
#include "../graphics/rendering.h"

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
    tsc_addSplash("Beware of viruses lurking in mods!", 10);
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
    tsc_addSplash("tpc bad", 1);
    tsc_addSplash("cellua bad", 1);
    tsc_addSplash("kell buggy", 1);
    tsc_addSplash("yo my name is Jeremy", 1);
    tsc_addSplash("Pesky Windows", 1);
    tsc_addSplash("Photon dev helped", 1);
    tsc_addSplash("go suck an egg with your small brain", 0.01);
    tsc_addSplash("PhoenixCM", 1);
    tsc_addSplash("Program in C, program in C", 0.05);
    tsc_addSplash("No memory leaks ever seen", 1);
    tsc_addSplash("Segmentation Fault (Core Dumped)", 1);
    tsc_addSplash("Cancer cause new cells", 1);
    tsc_addSplash("Mods, give him more cells (cancer)", 1);
    tsc_addSplash("Free & Open Source Software", 1);
    tsc_addSplash("\"🔥\" - k_lemon", 1);
    tsc_addSplash("Mods, nuke his grid", 1);
    tsc_addSplash("Mods, use 100% of his CPU", 1);
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
    if(tsc_hasLoadedMod(id)) return;
    const char *old = currentMod;
    currentMod = id;
    tsc_initMod(id); // Actually initialize it with the modloader
    currentMod = old;
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
    category->usedAsCell = false;
    return category;
}

tsc_category *tsc_newCellGroup(const char *title, const char *description, const char *mainCell) {
    tsc_category *category = malloc(sizeof(tsc_category));
    category->title = title;
    category->description = description;
    category->icon = mainCell;
    category->itemc = 0;
    category->itemcap = 20;
    category->items = malloc(sizeof(tsc_categoryitem) * category->itemcap);
    category->parent = NULL;
    category->open = false;
    category->usedAsCell = true;
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
    if(category != NULL && category->parent != NULL) {
        for(size_t i = 0; i < category->parent->itemc; i++) {
            if(category->parent->items[i].kind == TSC_CATEGORY_SUBCATEGORY) tsc_closeCategory(category->parent->items[i].category);
        }
    }
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
        tsc_saving_decodeWithAny(clipboard, currentGrid);
        isInitial = true;
        tickCount = 0;
        tsc_copyGrid(tsc_getGrid("initial"), tsc_getGrid("main"));
    }
}

static void tsc_saveButton(void *_) {
    tsc_buffer buffer = tsc_saving_newBuffer("");
    tsc_saving_encodeWithSmallest(&buffer, currentGrid);
    if(buffer.len == 0) {
        tsc_sound_play(builtin.audio.explosion);
    }
    SetClipboardText(buffer.mem);
    tsc_saving_deleteBuffer(buffer);
}

static void tsc_saveV3Button(void *_) {
    tsc_buffer buffer = tsc_saving_newBuffer("");
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
    tickCount = 0;
}

static void tsc_setInitial(void *_) {
    if(isGameTicking) return;
    tsc_copyGrid(tsc_getGrid("initial"), currentGrid);
    isInitial = true;
    tickCount = 0;
}

static void tsc_pasteButton(void *_) {
    tsc_pasteGridClipboard();
}

void tsc_loadDefaultCellBar() {
    tsc_category *root = tsc_rootCategory();

    tsc_category *tools = tsc_newCategory("Tools", "Simple tools and buttons", "icon");
    tsc_addButton(tools, "save", "Save to Clipboard", "Saves the current grid to clipboard using the smallest format", tsc_saveButton, NULL);
    tsc_addButton(tools, "save_v3", "Save to V3", "Saves the current grid using V3, which is supported by a lot more remakes. This can fail.", tsc_saveV3Button, NULL);
    tsc_addButton(tools, "load", "Load from Clipboard", "Load a level code from clipboard", tsc_loadButton, NULL);
    tsc_addButton(tools, "setinitial", "Set Initial", "Set the current grid state as the initial one", tsc_setInitial, NULL);
    tsc_addButton(tools, "restoreinitial", "Restore Initial", "Restore the initial grid state", tsc_restoreInitial, NULL);
    tsc_addButton(tools, "paste", "Paste", "Paste the copied selection (drag with Middle Click to select)", tsc_pasteButton, NULL);

    tsc_addCategory(root, tools);
    tsc_category *movers = tsc_newCellGroup("Movers", "Cells that move by themselves and may also move other cells", builtin.mover);
    tsc_addCell(movers, builtin.mover);
    tsc_addCategory(root, movers);
    tsc_category *generators = tsc_newCellGroup("Generators", "Cells that create other cells", builtin.generator);
    tsc_addCell(generators, builtin.generator);
    tsc_addCategory(root, generators);
    tsc_category *pushables = tsc_newCellGroup("Pushables", "Cells that dont move by themselves but can be pushed", builtin.push);
    tsc_addCell(pushables, builtin.push);
    tsc_addCell(pushables, builtin.slide);
    tsc_addCell(pushables, builtin.wall);
    tsc_addCategory(root, pushables);
    tsc_category *rotators = tsc_newCellGroup("Rotators", "Cells that rotate other nearby cells", builtin.rotator_cw);
    tsc_addCell(rotators, builtin.rotator_cw);
    tsc_addCell(rotators, builtin.rotator_ccw);
    tsc_addCategory(root, rotators);
    tsc_category *destroyers = tsc_newCellGroup("Destroyers", "Cells that destroy other cells", builtin.trash);
    tsc_addCell(destroyers, builtin.enemy);
    tsc_addCell(destroyers, builtin.trash);
    tsc_addCategory(root, destroyers);
    tsc_category *backgrounds = tsc_newCellGroup("Backgrounds", "Cells sitting in the background", builtin.placeable);
    tsc_addCell(backgrounds, builtin.placeable);
    tsc_addCategory(root, backgrounds);
}

tsc_settingCategory *tsc_settingCategories = NULL;
size_t tsc_settingLen = 0;
tsc_value tsc_settingStore;

void tsc_settingHandler(const char *title) {
    if(title == builtin.settings.vsync) {
        ClearWindowState(FLAG_VSYNC_HINT);
        if(tsc_toBoolean(tsc_getSetting(builtin.settings.vsync))) {
            SetWindowState(FLAG_VSYNC_HINT);
        }
    } else if(title == builtin.settings.fullscreen) {
        if(tsc_toBoolean(tsc_getSetting(builtin.settings.fullscreen)) != IsWindowFullscreen()) {
            ToggleFullscreen();
        }
    }
}

void tsc_loadSettings() {
    tsc_settingStore = tsc_object();

    // Load default settings
    const char *performance = tsc_addSettingCategory("performance", "Performance");
    const char *graphics = tsc_addSettingCategory("graphics", "Graphics");
    const char *audio = tsc_addSettingCategory("audio", "Audio");
    const char *saving = tsc_addSettingCategory("saving", "Saving");

    char *tc = NULL;
    asprintf(&tc, "%d", workers_amount());
    const char *threadCountStuff[2] = {"-0123456789", tc};
    builtin.settings.threadCount = tsc_addSetting("threadCount", "Thread Count", performance, TSC_SETTING_INPUT, threadCountStuff, tsc_settingHandler);

    builtin.settings.vsync = tsc_addSetting("vsync", "V-Sync", graphics, TSC_SETTING_TOGGLE, NULL, tsc_settingHandler);
    builtin.settings.fullscreen = tsc_addSetting("fullscreen", "Fullscreen", graphics, TSC_SETTING_TOGGLE, NULL, tsc_settingHandler);

    float volumes[2] = {0, 1};
    builtin.settings.sfxVolume = tsc_addSetting("sfxVolume", "SFX Volume", audio, TSC_SETTING_SLIDER, &volumes, tsc_settingHandler);
    builtin.settings.musicVolume = tsc_addSetting("musicVolume", "Music Volume", audio, TSC_SETTING_SLIDER, &volumes, tsc_settingHandler);

    tsc_setSetting(builtin.settings.sfxVolume, tsc_number(1));
    tsc_setSetting(builtin.settings.musicVolume, tsc_number(0.5));
}

tsc_value tsc_getSetting(const char *settingID) {
    return tsc_getKey(tsc_settingStore, settingID);
}

void tsc_setSetting(const char *settingID, tsc_value v) {
    tsc_setKey(tsc_settingStore, settingID, v);
}

const char *tsc_addSettingCategory(const char *settingCategoryID, const char *settingTitle) {
    size_t idx = tsc_settingLen++;
    tsc_settingCategories = tsc_realloc(tsc_settingCategories, sizeof(tsc_settingCategory) * tsc_settingLen);
    const char *id = tsc_strdup(tsc_padWithModID(settingCategoryID));
    size_t settingcap = 10;
    tsc_settingCategory category = {
        .id = id,
        .title = tsc_strdup(settingTitle),
        .settings = malloc(sizeof(tsc_setting) * settingcap),
        .settingcap = settingcap,
        .settinglen = 0,
    };
    tsc_settingCategories[idx] = category;
    return id;
}
const char *tsc_addSetting(const char *settingID, const char *name, const char *categoryID, unsigned char kind, void *data, tsc_settingCallback *callback) {
    tsc_settingCategory *category = NULL;
    for(size_t i = 0; i < tsc_settingLen; i++) {
        if(tsc_settingCategories[i].id == categoryID) {
            category = tsc_settingCategories + i;
            break;
        }
    }
    settingID = tsc_strdup(tsc_padWithModID(settingID));
    tsc_setting setting = {
        .kind = kind,
        .id = settingID,
        .name = tsc_strdup(name),
        .callback = callback,
    };
    if(kind == TSC_SETTING_SLIDER) {
        float *range = data;
        setting.slider.min = range[0];
        setting.slider.max = range[1];
    } else if(kind == TSC_SETTING_LIST) {
        size_t len = 0;
        const char **buf = data;
        while(buf[len] != NULL) len++;
        const char **buf2 = malloc(sizeof(const char *) * len);
        memcpy(buf2, buf, sizeof(const char *) * len);
        setting.list.items = buf2;
        setting.list.len = len;
    } else if(kind == TSC_SETTING_INPUT) {
        const char **buf = data;
        setting.string.charset = tsc_strdup(buf[0]);
        setting.string.bufferlen = 256;
        setting.string.buffer = malloc(sizeof(char) * setting.string.bufferlen);
        strncpy(setting.string.buffer, buf[1], setting.string.bufferlen - 1);
        setting.string.selected = false;
    }
    if(tsc_isNull(tsc_getKey(tsc_settingStore, settingID))) {
        if(kind == TSC_SETTING_TOGGLE) {
            tsc_setKey(tsc_settingStore, settingID, tsc_boolean(data != NULL));
        } else if(kind == TSC_SETTING_SLIDER) {
            float *range = data;
            tsc_setKey(tsc_settingStore, settingID, tsc_number(range[2]));
        } else if(kind == TSC_SETTING_LIST) {
            const char **buf = data;
            tsc_value firstVal = tsc_string(*buf);
            tsc_setKey(tsc_settingStore, settingID, firstVal); // retains
            tsc_destroy(firstVal);
        } else if(kind == TSC_SETTING_INPUT) {
            const char **buf = data;
            tsc_value s = tsc_string(buf[1]);
            tsc_setKey(tsc_settingStore, settingID, s);
            tsc_destroy(s);
        }
    }
    if(category->settinglen == category->settingcap) {
        category->settingcap *= 2;
        category->settings = realloc(category->settings, sizeof(tsc_setting) * category->settingcap);
    }
    size_t i = category->settinglen++;
    category->settings[i] = setting;
    return settingID;
}
