//========= Copyright (c) 2025 HL2SB++ ============//
//
// Purpose:
//
//=============================================================================//

#ifndef L_NAVMESH_H
#define L_NAVMESH_H

#ifdef _WIN32
#pragma once
#endif

#include "nav_mesh.h"
#include "luamanager.h"

LUA_API void (lua_pushnavarea) (lua_State *L, CNavArea *area);

LUA_API CNavArea *(lua_tonavarea) (lua_State *L, int idx);
LUA_API CNavArea *(luaL_checknavarea) (lua_State *L, int narg);

#endif // L_NAVMESH_H
