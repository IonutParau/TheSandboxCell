#ifndef PLATFORM_LUA54_COMMON_H
#define PLATFORM_LUA54_COMMON_H

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include "tinycthread.h"

typedef struct lua54_LuaVM {
    lua_State *state;
    mtx_t gil;
} lua54_LuaVM;

typedef struct lua54_CellPayload {
    lua54_LuaVM *vm;
    const char *cellID;
    // A lua ref (see https://www.lua.org/pil/27.3.2.html)
    int configRef;
} lua54_CellPayload;

extern const char *lua54_magic_rkey;

lua54_LuaVM *lua54_tsc_newVM();

#endif
