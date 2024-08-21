#include "common.h"
#include "tinycthread.h"

lua54_LuaVM *lua54_tsc_newVM() {
    lua54_LuaVM *vm = malloc(sizeof(lua54_LuaVM));
    lua_State *state = luaL_newstate();
    vm->state = luaL_newstate();
    mtx_init(&vm->gil, mtx_recursive); // recursive to prevent accidentally deadlocking yourself
    return vm;
}
