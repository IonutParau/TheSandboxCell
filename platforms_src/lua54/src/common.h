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

lua54_LuaVM *lua54_tsc_newVM();

#endif
