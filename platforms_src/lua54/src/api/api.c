#include "api.h"

void lua54_tsc_loadBindings(lua54_LuaVM *vm) {
    lua54_tsc_loadCellBindings(vm);
    lua54_tsc_loadGridBindings(vm);
    lua54_tsc_loadSubtickBindings(vm);
    lua54_tsc_loadAudioBindings(vm);
}
