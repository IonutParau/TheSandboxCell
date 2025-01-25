#include <lua.h>
#include <lauxlib.h>
#include "libtsc.h"
#include "common.h"
#include "api/api.h"

void lua54_loadMod(const char *modID, const char *path, tsc_value config) {
    lua54_LuaVM *vm = lua54_tsc_newVM();

    lua54_tsc_loadBindings(vm);

    lua_getglobal(vm->state, "package");
    lua_pushfstring(vm->state, "./mods/%s/?.lua;./mods/%s/?/init.lua;", modID, path);
    lua_getfield(vm->state, -2, "path");
    lua_concat(vm->state, 2);
    lua_setfield(vm->state, -2, "path");

    lua_getglobal(vm->state, "require");
    lua_pushstring(vm->state, "main");
    lua_call(vm->state, 1, 0);
}
