#include "api.h"
#include "../libtsc.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

int lua54_tsc_addCell(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushvalue(L, 1); // push table
	int config = lua_gettop(L);
	lua_geti(L, config, 1);
	if(lua_isnoneornil(L, -1)) {
		lua_getfield(L, config, "id");
	}
	const char *id = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid ID");
	}
	lua_geti(L, config, 2);
	if(lua_isnoneornil(L, -1)) {
		lua_getfield(L, config, "name");
	}
	const char *name = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid name for %s", id);
	}
	lua_geti(L, config, 3);
	if(lua_isnoneornil(L, -1)) {
		lua_getfield(L, config, "desc");
	}
	const char *description = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid description for %s", id);
	}
	id = tsc_registerCell(id, name, description);
	printf("Adding a cell: %s\n", id);
	lua_pushstring(L, id);
	return 1;
}

void lua54_tsc_loadCellBindings(lua54_LuaVM *vm) {
	lua_newtable(vm->state);
	int cellFuncs = lua_gettop(vm->state);
	lua_pushcfunction(vm->state, lua54_tsc_addCell);
	lua_setfield(vm->state, cellFuncs, "Add");
}
