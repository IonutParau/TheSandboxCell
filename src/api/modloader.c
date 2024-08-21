#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "../utils.h"
#include "tscjson.h"

#ifdef _WIN32
#include <windows.h>
typedef HINSTANCE tsc_lib_t;
typedef void __cdecl tsc_platform_loadMod(const char *id, const char *path, tsc_value config);
#endif
#ifdef linux
#include <dlfcn.h>
typedef void *tsc_lib_t;
typedef void tsc_platform_loadMod(const char *id, const char *path, tsc_value config);
#endif

typedef struct tsc_platform_t {
    const char *id;
    tsc_lib_t *lib;
} tsc_platform_t;

tsc_platform_t *platforms = NULL;
size_t platformc = 0;

tsc_mod_t *mods = NULL;
size_t modc = 0;

static void tsc_initDLLMod(const char *id) {
    printf("Initializing %s as a DLL mod\n", id);
    static char buffer[256];
    const char *lib;
    #ifdef _WIN32
    lib = "mod.dll";
    #endif
    #ifdef linux
    lib = "mod.so";
    #endif
    snprintf(buffer, 256, "mods/%s/%s", id, lib);
    tsc_pathfix(buffer);
    static char sym[256];
    snprintf(sym, 256, "%s_init", id);
#ifdef _WIN32
    HINSTANCE library = LoadLibraryA(buffer);
    if(library == NULL) {
        fprintf(stderr, "Unable to open %s\n", buffer);
        exit(1);
        return;
    }
    void (__cdecl *init)(void) = (void (__cdecl *)(void))GetProcAddress(library, sym);
#endif
#ifdef linux
    void *library = dlopen(buffer, RTLD_NOW | RTLD_DEEPBIND);
    if(library == NULL) {
        fprintf(stderr, "Unable to open %s\n", dlerror());
        exit(1);
        return;
    }
    void (*init)(void) = dlsym(library, sym);
    if(init == NULL) {
        fprintf(stderr, "%s has no init or an invalid init. Must use the name %s\n", buffer, sym);
        exit(1);
        return;
    }
    init();
#endif
}

static tsc_lib_t *tsc_getPlatform(const char *platform) {
    for(size_t i = 0; i < platformc; i++) {
        if(tsc_streql(platforms[i].id, platform)) {
            return platforms[i].lib;
        }
    }
    static char buffer[256];
    const char *lib;
#ifdef _WIN32
    lib = "platform.dll";
#endif
#ifdef linux
    lib = "platform.so";
#endif
    snprintf(buffer, 256, "platforms/%s/%s", platform, lib);
    tsc_pathfix(buffer);
#ifdef _WIN32
    HINSTANCE library = LoadLibraryA(buffer);
    if(library == NULL) {
        fprintf(stderr, "Unable to open %s\n", buffer);
        exit(1);
        return NULL;
    }
#endif
#ifdef linux
    void *library = dlopen(buffer, RTLD_NOW | RTLD_DEEPBIND);
    if(library == NULL) {
        fprintf(stderr, "Unable to open %s\n", dlerror());
        exit(1);
        return NULL;
    }
#endif
    tsc_platform_t p;
    p.id = tsc_strdup(platform);
    p.lib = library;
    size_t idx = platformc++;
    platforms = realloc(platforms, sizeof(tsc_platform_t) * platformc);
    platforms[idx] = p;
    return library;
}

static void tsc_initPlatformMod(const char *id, const char *platform, tsc_value value) {
    printf("Initializing %s as a %s mod\n", id, platform);
    tsc_lib_t *lib = tsc_getPlatform(platform);
    static char sym[256];
    snprintf(sym, 256, "%s_loadMod", platform);
    char *path;
    asprintf(&path, "mods/%s", id);
    tsc_pathfix(path);
#ifdef _WIN32
    tsc_platform_loadMod *loadMod = GetProcAddress(lib, sym);
    if(loadMod == NULL) {
        printf("Unable to load %s with %s due to missing symbol %s\n", id, platform, sym);
        exit(1);
        return;
    }
    loadMod(id, platform, value);
#endif
#ifdef linux
    tsc_platform_loadMod *loadMod = dlsym(lib, sym);
    if(loadMod == NULL) {
        printf("Unable to load %s with %s due to missing symbol %s\n", id, platform, sym);
        exit(1);
        return;
    }
    loadMod(id, path, value);
#endif
    free(path);
}

bool tsc_hasLoadedMod(const char *id) {
    for(size_t i = 0; i < modc; i++) {
        if(tsc_streql(mods[i].id, id)) {
            return true;
        }
    }
    return false;
}

void tsc_initMod(const char *id) {
    printf("Preparing to intialize %s\n", id);
    static char configpath[256];
    snprintf(configpath, 256, "mods/%s/config.json", id);
    tsc_pathfix(configpath);

    if(!tsc_hasfile(configpath)) {
        // Fallback to DLL mod
        tsc_initDLLMod(id);
        size_t idx = modc++;
        mods = realloc(mods, sizeof(tsc_mod_t) * modc);
        mods[idx].id = tsc_strdup(id);
        mods[idx].value = tsc_null();
        return;
    }

    char *contents = tsc_allocfile(configpath, NULL);
    tsc_buffer buffer = tsc_saving_newBuffer("");
    tsc_value config = tsc_json_decode(contents, &buffer);
    if(buffer.len != 0) {
        printf("Unable to load %s: %s\n", configpath, buffer.mem);
        exit(1);
        return;
    }
    tsc_saving_deleteBuffer(buffer);
    free(contents);

    size_t idx = modc++;
    mods = realloc(mods, sizeof(tsc_mod_t) * modc);
    mods[idx].id = tsc_strdup(id);
    mods[idx].value = config;
    
    tsc_value dependencies = tsc_getKey(config, "dependencies");
    size_t deplen = tsc_getLength(dependencies);
    for(size_t i = 0; i < deplen; i++) {
        tsc_value dep = tsc_index(dependencies, i);
        if(tsc_isString(dep)) {
            tsc_initMod(tsc_toString(dep));
        }
    }

    tsc_value platform = tsc_getKey(config, "platform");
    tsc_value platform_conf = tsc_getKey(config, "platform_config");
    if(tsc_isString(platform)) {
        tsc_initPlatformMod(id, tsc_toString(platform), platform_conf);
    } else {
        tsc_initDLLMod(id);
    }
}

void tsc_loadAllMods() {
    size_t dirfilec;
    char **dirfiles = tsc_dirfiles("mods", &dirfilec);
    for(int i = 0; i < dirfilec; i++) {
        // This mutates, careful you dingus
        if(tsc_fextension(dirfiles[i]) == NULL) {
            tsc_loadMod(dirfiles[i]);
        }
    }
    tsc_freedirfiles(dirfiles);
}
