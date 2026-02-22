//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Random number generator
//
// $Workfile: $
// $NoKeywords: $
//===========================================================================//

#define lrandom_cpp

#include "cbase.h"
#include "lua.hpp"
#include "luasrclib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void getIntRange(lua_State *L, int &a, int &b) {
    int top = lua_gettop(L);
    if (top == 0) {
        a = b = 0; // default [0,0]
    } else if (top == 1) {
        int v = luaL_checkint(L, 1);
        a = -v; b = v; // symmetric range
    } else {
        a = luaL_checkint(L, 1);
        b = luaL_checkint(L, 2);
    }
}

static void getFloatRange(lua_State *L, float &a, float &b) {
    int top = lua_gettop(L);
    if (top == 0) {
        a = b = 0.0f;
    } else if (top == 1) {
        float v = (float)luaL_checknumber(L, 1);
        a = -v; b = v;
    } else {
        a = (float)luaL_checknumber(L, 1);
        b = (float)luaL_checknumber(L, 2);
    }
}

static int random_RandomFloat(lua_State *L) {
    float a, b;
    getFloatRange(L, a, b);
    lua_pushnumber(L, random->RandomFloat(a, b));
    return 1;
}

static int random_RandomFloatExp(lua_State *L) {
    float a, b;
    getFloatRange(L, a, b);
    float exponent = (float)luaL_optnumber(L, 3, 1.0f);
    lua_pushnumber(L, random->RandomFloatExp(a, b, exponent));
    return 1;
}

static int random_RandomInt(lua_State *L) {
    int a, b;
    getIntRange(L, a, b);
    lua_pushinteger(L, random->RandomInt(a, b));
    return 1;
}

static int random_SetSeed(lua_State *L) {
    random->SetSeed(luaL_checkint(L, 1));
    return 0;
}

static const luaL_Reg randomlib[] = {
  {"RandomFloat",   random_RandomFloat},
  {"RandomFloatExp",   random_RandomFloatExp},
  {"RandomInt",   random_RandomInt},
  {"SetSeed",   random_SetSeed},
  {"RandomFloat",  random_RandomFloat},
  {"RandomFloatExp",  random_RandomFloatExp},
  {"RandomInt",  random_RandomInt},
  {NULL, NULL}
};

/*
** Open random library
*/
LUALIB_API int luaopen_random (lua_State *L) {
  luaL_register(L, LUA_RANDOMLIBNAME, randomlib);
  // UNDONE: this has always been redundant.
  // luaL_register(L, "_G", random_funcs);
  return 1;
}

