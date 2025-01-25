#include "api.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

void lua54_tsc_loadBindings(lua54_LuaVM *vm) {
    lua_newtable(vm->state);
    lua54_tsc_loadCellBindings(vm);
    lua_setfield(vm->state, -2, "Cell");
    // lua54_tsc_loadGridBindings(vm);
    // lua54_tsc_loadSubtickBindings(vm);
    // lua54_tsc_loadCategoryBindings(vm);
    // lua54_tsc_loadTextureBindings(vm);
    // lua54_tsc_loadAudioBindings(vm);
    lua_setglobal(vm->state, "TSC");
}
