//========= Copyright (c) 2025 HL2SB++ ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "lnavmesh.h"
#include "nav_area.h"
#include "nav_mesh.h"
#include "mathlib/lvector.h"
#include "luasrclib.h"

#include "tier0/memdbgon.h" // must be last include

/*
** Push functions (C -> Lua stack)
*/
LUA_API void lua_pushnavarea(lua_State *L, CNavArea *area) {
    if (!area) {
        lua_pushnil(L);
        return;
    }
    CNavArea **ppArea = (CNavArea **)lua_newuserdata(L, sizeof(CNavArea *));
    *ppArea = area;
    luaL_getmetatable(L, LUA_NAVAREALIBNAME);
    lua_setmetatable(L, -2);
}

/*
** Access functions (Lua -> C)
*/
LUA_API CNavArea *lua_tonavarea(lua_State *L, int idx) {
    CNavArea **ppArea = (CNavArea **)lua_touserdata(L, idx);
    return ppArea ? *ppArea : nullptr;
}

LUA_API CNavArea *luaL_checknavarea(lua_State *L, int narg) {
    CNavArea **ppArea = (CNavArea **)luaL_checkudata(L, narg, LUA_NAVAREALIBNAME);
    if (!ppArea || !*ppArea)
        luaL_argerror(L, narg, "CNavArea expected");
    return *ppArea;
}

/*
** CNavArea methods
*/
static int CNavArea_GetID(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    lua_pushinteger(L, (lua_Integer)area->GetID());
    return 1;
}

static int CNavArea_GetCenter(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    const Vector &center = area->GetCenter();
    lua_pushvector(L, center);
    return 1;
}

static int CNavArea_GetSizeX(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    lua_pushnumber(L, area->GetSizeX());
    return 1;
}

static int CNavArea_GetSizeY(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    lua_pushnumber(L, area->GetSizeY());
    return 1;
}

static int CNavArea_GetCorner(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int corner = (int)luaL_checkint(L, 2);
    if (corner < 0 || corner >= NUM_CORNERS)
        luaL_argerror(L, 2, "corner index out of range (0..3)");
    Vector v = area->GetCorner((NavCornerType)corner);
    lua_pushvector(L, v);
    return 1;
}

static int CNavArea_SetCorner(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int corner = (int)luaL_checkint(L, 2);
    if (corner < 0 || corner >= NUM_CORNERS)
        luaL_argerror(L, 2, "corner index out of range (0..3)");
    Vector v = luaL_checkvector(L, 3);
    area->SetCorner((NavCornerType)corner, v);
    return 0;
}

static int CNavArea_GetRandomPoint(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    Vector v = area->GetRandomPoint();
    lua_pushvector(L, v);
    return 1;
}

/*
   Lua:
     area:IsBlocked([teamID])
     area:SetBlocked(bool blocked, [teamID], [blockerEnt])  -- blockerEnt is optional userdata CBaseEntity* if you have a binding
*/
static int CNavArea_IsBlocked(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int teamID = (int)luaL_optint(L, 2, TEAM_ANY);
    bool val = area->IsBlocked(teamID, true);
    lua_pushboolean(L, val);
    return 1;
}

static int CNavArea_SetBlocked(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    bool blocked = lua_toboolean(L, 2) != 0;
    int teamID = (int)luaL_optint(L, 3, TEAM_ANY);

    CBaseEntity *blocker = NULL;
    if (!lua_isnoneornil(L, 4))
    {
        // TODO: add
    }
    area->MarkAsBlocked(teamID, blocker, true);
    return 0;
}

static int CNavArea_Contains(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    Vector pos = luaL_checkvector(L, 2);
    lua_pushboolean(L, area->Contains(pos));
    return 1;
}

static int CNavArea_GetClosestPointOnArea(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    Vector pos = luaL_checkvector(L, 2);
    Vector close;
    area->GetClosestPointOnArea(&pos, &close);
    lua_pushvector(L, close);
    return 1;
}

static int CNavArea_GetDistanceSquaredToPoint(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    Vector pos = luaL_checkvector(L, 2);
    float dsq = area->GetDistanceSquaredToPoint(pos);
    lua_pushnumber(L, dsq);
    return 1;
}

static int CNavArea_GetLightIntensityAtPos(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    Vector pos = luaL_checkvector(L, 2);
    float intensity = area->GetLightIntensity(pos);
    lua_pushnumber(L, intensity);
    return 1;
}

static int CNavArea_GetLightIntensity(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    lua_pushnumber(L, area->GetLightIntensity());
    return 1;
}

static int CNavArea_GetAdjacentCount(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int dir = (int)luaL_optint(L, 2, 0); // default 0 (NavDirType cast)
    lua_pushinteger(L, area->GetAdjacentCount((NavDirType)dir));
    return 1;
}

static int CNavArea_GetAdjacentArea(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int dir = (int)luaL_optint(L, 2, 0);
    int idx = (int)luaL_checkint(L, 3);
    CNavArea *adj = area->GetAdjacentArea((NavDirType)dir, idx);
    lua_pushnavarea(L, adj);
    return 1;
}

static int CNavArea_GetRandomAdjacentArea(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int dir = (int)luaL_optint(L, 2, 0);
    CNavArea *adj = area->GetRandomAdjacentArea((NavDirType)dir);
    lua_pushnavarea(L, adj);
    return 1;
}

static int CNavArea_GetPlayerCount(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    int teamID = (int)luaL_optint(L, 2, 0);
    lua_pushinteger(L, area->GetPlayerCount(teamID));
    return 1;
}

static int CNavArea_GetHidingSpots(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    const HidingSpotVector *spots = area->GetHidingSpots();
    lua_newtable(L);
    int i = 1;
    for (int s = 0; s < spots->Count(); ++s)
    {
        const HidingSpot &hs = *(*spots)[s];
        lua_pushinteger(L, i++);
        lua_pushvector(L, hs.GetPosition());
        lua_settable(L, -3);
    }
    return 1;
}

static int CNavArea___tostring(lua_State *L)
{
    CNavArea *area = luaL_checknavarea(L, 1);
    lua_pushfstring(L, "CNavArea[%d] (%p)", area->GetID(), area);
    return 1;
}

static const luaL_Reg CNavArea_meta[] = {
    {"GetID", CNavArea_GetID},
    {"GetCenter", CNavArea_GetCenter},
    {"GetSizeX", CNavArea_GetSizeX},
    {"GetSizeY", CNavArea_GetSizeY},
    {"GetCorner", CNavArea_GetCorner},
    {"SetCorner", CNavArea_SetCorner},
    {"GetRandomPoint", CNavArea_GetRandomPoint},
    {"IsBlocked", CNavArea_IsBlocked},
    {"SetBlocked", CNavArea_SetBlocked},
    {"Contains", CNavArea_Contains},
    {"GetClosestPointOnArea", CNavArea_GetClosestPointOnArea},
    {"GetDistanceSquaredToPoint", CNavArea_GetDistanceSquaredToPoint},
    {"GetLightIntensityAtPos", CNavArea_GetLightIntensityAtPos},
    {"GetLightIntensity", CNavArea_GetLightIntensity},
    {"GetAdjacentCount", CNavArea_GetAdjacentCount},
    {"GetAdjacentArea", CNavArea_GetAdjacentArea},
    {"GetRandomAdjacentArea", CNavArea_GetRandomAdjacentArea},
    {"GetPlayerCount", CNavArea_GetPlayerCount},
    {"GetHidingSpots", CNavArea_GetHidingSpots},
    {"__tostring", CNavArea___tostring},
    {NULL, NULL}
};

LUALIB_API int luaopen_CNavArea(lua_State *L) {
    luaL_newmetatable(L, LUA_NAVAREALIBNAME);
    luaL_register(L, NULL, CNavArea_meta);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    lua_pushstring(L, "navarea");
    lua_setfield(L, -2, "__type");
    lua_pop(L, 1);
    return 1;
}

/*
** Global navmesh library
*/
static int navmesh_GetAllNavAreas(lua_State *L) {
    lua_newtable(L);
    int i = 1;
    for (int id = 0; id < TheNavMesh->GetNavAreaCount(); ++id) {
        CNavArea *area = TheNavMesh->GetNavAreaByID(id);
        if (area) {
            lua_pushinteger(L, i++);
            lua_pushnavarea(L, area);
            lua_settable(L, -3);
        }
    }
    return 1;
}

static int navmesh_GetNearestNavArea(lua_State *L) {
    Vector pos = luaL_checkvector(L, 1);
    float beneathLimit = (float)luaL_optnumber(L, 2, 120.0f);
    float distLimit = (float)luaL_optnumber(L, 3, 10000.0f);
    bool checkLOS = lua_toboolean(L, 4) != 0;

    CNavArea *area = TheNavMesh->GetNearestNavArea(pos, false, beneathLimit, checkLOS, distLimit);
    lua_pushnavarea(L, area);
    return 1;
}

static const luaL_Reg navmesh_funcs[] = {
    {"GetAllNavAreas", navmesh_GetAllNavAreas},
    {"GetNearestNavArea", navmesh_GetNearestNavArea},
    {NULL, NULL}
};

LUALIB_API int luaopen_navmesh(lua_State *L) {
    luaL_register(L, LUA_NAVMESHLIBNAME, navmesh_funcs);
    return 1;
}