#include "api.h"
#include "../libtsc.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char *lua54_tsc_cellptr_meta = "lua54_tsc_cellptr_meta";

int lua54_tsc_cellptr_index(lua_State *L) {
    tsc_cell **ptr = luaL_checkudata(L, 1, lua54_tsc_cellptr_meta);
    if(ptr == NULL) {
        luaL_argerror(L, 1, "`cellptr` expected");
    }
    tsc_cell *cell = *ptr;

    const char *key = lua_tostring(L, 2);
    if(key == NULL) return 0;

    if(tsc_streql(key, "id")) {
        lua_pushstring(L, cell->id);
        return 1;
    }
    if(tsc_streql(key, "texture")) {
        if(cell->texture == NULL) {
            return 0;
        }
        lua_pushstring(L, cell->texture);
        return 1;
    }
    if(tsc_streql(key, "rot")) {
        lua_pushinteger(L, cell->rot);
        return 1;
    }
    if(tsc_streql(key, "addedRot")) {
        lua_pushinteger(L, cell->addedRot);
        return 1;
    }
    if(tsc_streql(key, "lx")) {
        lua_pushinteger(L, cell->lx);
        return 1;
    }
    if(tsc_streql(key, "ly")) {
        lua_pushinteger(L, cell->ly);
        return 1;
    }
    if(tsc_streql(key, "updated")) {
        lua_pushboolean(L, cell->updated);
        return 1;
    }
    return 0; // return nil cuz wtf
}

int lua54_tsc_cellptr_newindex(lua_State *L) {
    tsc_cell **ptr = luaL_checkudata(L, 1, lua54_tsc_cellptr_meta);
    if(ptr == NULL) {
        luaL_argerror(L, 1, "`cellptr` expected");
    }
    tsc_cell *cell = *ptr;

    const char *key = lua_tostring(L, 2);
    if(key == NULL) return 0;

    if(tsc_streql(key, "texture")) {
        if(lua_isnoneornil(L, 3)) {
            cell->texture = NULL;
            return 0;
        }
        const char *s = lua_tostring(L, 3);
        if(s == NULL) {
            luaL_error(L, "texture expected to be string");
        }
        cell->texture = s;
    }
    if(tsc_streql(key, "lx")) {
        cell->lx = lua_tonumber(L, 3);
    }
    if(tsc_streql(key, "ly")) {
        cell->ly = lua_tonumber(L, 3);
    }
    if(tsc_streql(key, "addedRot")) {
        cell->addedRot = lua_tonumber(L, 3);
    }
    return 0;
}

void lua54_tsc_loadCellPtrMeta(lua_State *L) {
    luaL_newmetatable(L, lua54_tsc_cellptr_meta);
    int meta = lua_gettop(L);
    lua_pushcfunction(L, lua54_tsc_cellptr_index);
    lua_setfield(L, meta, "__index");
    lua_pushcfunction(L, lua54_tsc_cellptr_index);
    lua_setfield(L, meta, "__newindex");
}

void lua54_tsc_pushcellptr(lua_State *L, tsc_cell *cell) {
    tsc_cell **ptr = lua_newuserdata(L, sizeof(tsc_cell *));
    *ptr = cell;
    luaL_getmetatable(L, lua54_tsc_cellptr_meta);
    lua_setmetatable(L, -2);
}

int lua54_tsc_binding_canMove(tsc_grid *grid, tsc_cell *cell, int x, int y, char dir, const char *forceType, double force, void *payload) {

}

int lua54_tsc_addCell(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushvalue(L, 1); // push table
	int config = lua_gettop(L);
	lua_geti(L, config, 1);
	if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
		lua_getfield(L, config, "id");
	}
	const char *id = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid ID");
	}
	lua_geti(L, config, 2);
	if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
		lua_getfield(L, config, "name");
	}
	const char *name = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid name for %s", id);
	}
	lua_geti(L, config, 3);
	if(lua_isnoneornil(L, -1)) {
        lua_pop(L, 1);
		lua_getfield(L, config, "desc");
	}
	const char *description = lua_tostring(L, -1);
	if(id == NULL) {
		luaL_error(L, "TSC: invalid description for %s", id);
	}
	id = tsc_registerCell(id, name, description);
	printf("[LUA] Adding a cell: %s\n", id);
    // This bullshit can cause memory leaks if an error occurs, but if one does, you're fucked anyways
    tsc_cell tmp = tsc_cell_create(id, 0); // lite cell, needn't worry about deleting
    tsc_celltable *table = tsc_cell_getTable(&tmp);
    lua54_CellPayload *payload = malloc(sizeof(lua54_CellPayload));
    payload->cellID = id;
    lua_getfield(L, LUA_REGISTRYINDEX, lua54_magic_rkey);
    payload->vm = lua_touserdata(L, -1);
    lua_pop(L, 1);
    // Config on top
    lua_pushvalue(L, config);
    payload->configRef = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(L, payload);
    int luaPayload = lua_gettop(L);

    // Config on top, time to define the functions.
    lua_pushvalue(L, config);
    lua_getfield(L, -1, "canMove");
    if(lua_isfunction(L, -1)) {
        table->canMove = lua54_tsc_binding_canMove;
    }
    lua_pop(L, 1);

    // Return ID
	lua_pushstring(L, id);
	return 1;
}

void lua54_tsc_loadCellBindings(lua54_LuaVM *vm) {
	lua_newtable(vm->state);
	int cellFuncs = lua_gettop(vm->state);
	lua_pushcfunction(vm->state, lua54_tsc_addCell);
	lua_setfield(vm->state, cellFuncs, "Add");
}
