#include "api.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void lua54_tsc_loadBindings(lua54_LuaVM *vm) {
    lua_newtable(vm->state);
    lua_setglobal(vm->state, "TSC");
    // lua54_tsc_loadCellBindings(vm);
    // lua54_tsc_loadGridBindings(vm);
    // lua54_tsc_loadSubtickBindings(vm);
    // lua54_tsc_loadAudioBindings(vm);
}
