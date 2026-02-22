#ifndef LAMMODEF_H
#define LAMMODEF_H
#ifdef _WIN32
#pragma once
#endif

#include "ammodef.h"
#include "lua.hpp"

typedef CAmmoDef lua_CAmmoDef;

LUA_API lua_CAmmoDef* lua_toammodef(lua_State* L, int idx);
LUALIB_API lua_CAmmoDef* luaL_checkammodef(lua_State* L, int narg);
LUA_API void lua_pushammodef(lua_State* L, lua_CAmmoDef* pAmmoDef);

#endif