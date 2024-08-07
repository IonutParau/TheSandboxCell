#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"
#include "../utils.h"

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef linux
#include <dlfcn.h>
#endif

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
    printf("Expected init symbol: %s\n", sym);
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

void tsc_initMod(const char *id) {
    printf("Preparing to intialize %s\n", id);
    static char configpath[256];
    snprintf(configpath, 256, "mods/%s/mod.yaml", id);
    tsc_pathfix(configpath);

    if(!tsc_hasfile(configpath)) {
        // Fallback to DLL mod
        tsc_initDLLMod(id);
        return;
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
