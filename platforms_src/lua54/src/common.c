#include "common.h"
#include "tinycthread.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

const char *lua54_magic_rkey = "tsc_lua54_vm_self";

lua54_LuaVM *lua54_tsc_newVM() {
    lua54_LuaVM *vm = malloc(sizeof(lua54_LuaVM));
    lua_State *state = luaL_newstate();
    luaL_openlibs(state);
    vm->state = state;
    mtx_init(&vm->gil, mtx_recursive); // recursive to prevent accidentally deadlocking yourself
    lua_pushlightuserdata(state, vm);
    lua_setfield(state, LUA_REGISTRYINDEX, lua54_magic_rkey);
    return vm;
}
