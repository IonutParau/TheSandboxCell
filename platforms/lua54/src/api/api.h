#ifndef LUA54_TSC_API_H
#define LUA54_TSC_API_H

#include "../common.h"

void lua54_tsc_loadBindings(lua54_LuaVM *vm);
void lua54_tsc_loadCellBindings(lua54_LuaVM *vm);
void lua54_tsc_loadGridBindings(lua54_LuaVM *vm);
void lua54_tsc_loadSubtickBindings(lua54_LuaVM *vm);
void lua54_tsc_loadCategoryBindings(lua54_LuaVM *vm);
void lua54_tsc_loadTextureBindings(lua54_LuaVM *vm);
void lua54_tsc_loadAudioBindings(lua54_LuaVM *vm);

#endif
