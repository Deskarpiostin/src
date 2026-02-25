//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#if defined( CLIENT_DLL )
	#include "c_hl2mp_player.h"
	#include "hud.h"
	#include "vgui/ISurface.h"
	#include "vgui/ILocalize.h"
	#include <vgui_controls/Controls.h>
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbase_scriptedweapon.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "luamanager.h"
#include "lbasecombatweapon_shared.h"
#include "mathlib/lvector.h"
#include "lhl2mp_player_shared.h"
#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "hud_macros.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
class CHudLuaWeaponDraw : public CHudElement, public vgui::Panel
{
    DECLARE_CLASS_SIMPLE( CHudLuaWeaponDraw, vgui::Panel );

public:
    CHudLuaWeaponDraw( const char *pElementName ) : CHudElement( pElementName ), Panel( NULL, "HudLuaWeaponDraw" )
    {
        SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
        SetPaintBackgroundEnabled( false );

		SetParent( g_pClientMode->GetViewport() );
    }

    virtual void Paint()
    {
        C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
        if ( !pPlayer )
            return;

        CBaseCombatWeapon *pWpn = pPlayer->GetActiveWeapon();
        if ( !pWpn )
            return;

        CHL2MPScriptedWeapon *pScriptWpn = dynamic_cast<CHL2MPScriptedWeapon*>( pWpn );
        if ( !pScriptWpn )
            return;

        pScriptWpn->CallDrawHUD();
    }
};

DECLARE_HUDELEMENT( CHudLuaWeaponDraw );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPScriptedWeapon, DT_HL2MPScriptedWeapon )

BEGIN_NETWORK_TABLE( CHL2MPScriptedWeapon, DT_HL2MPScriptedWeapon )
#ifdef CLIENT_DLL
	RecvPropString( RECVINFO( m_iScriptedClassname ) ),
#else
	SendPropString( SENDINFO( m_iScriptedClassname ) ),
#endif
END_NETWORK_TABLE()

//=========================================================
//	>> CHLSelectFireScriptedWeapon
//=========================================================
BEGIN_DATADESC( CHL2MPScriptedWeapon )
END_DATADESC()
#ifdef CLIENT_DLL
extern ConVar v_viewmodel_fov;
#endif

#if defined( LUA_SDK )
static bool PushTableFromRef(lua_State *L, int ref)
{
    if (ref == LUA_NOREF || !L)
        return false;

    lua_getref(L, ref);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    return true;
}

static bool GetFieldRemoveTable(lua_State *L, const char *key)
{
    if (!L || !key)
        return false;

    if (!lua_istable(L, -1))
        return false;

    lua_pushvalue(L, -1);
    int table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);
    lua_getfield(L, -1, key);

    if (lua_isnil(L, -1))
    {
        lua_pop(L, 2);
        luaL_unref(L, LUA_REGISTRYINDEX, table_ref);
        return false;
    }

    lua_remove(L, -3);

    lua_remove(L, -2);
    luaL_unref(L, LUA_REGISTRYINDEX, table_ref);

    return true;
}

#endif

// These functions replace the macros above for runtime registration of
// scripted weapons.
#ifdef CLIENT_DLL
static C_BaseEntity *CCHL2MPScriptedWeaponFactory( void )
{
	return static_cast< C_BaseEntity * >( new CHL2MPScriptedWeapon );
};
#endif

#ifndef CLIENT_DLL
static CUtlDict< CEntityFactory<CHL2MPScriptedWeapon>*, unsigned short > m_WeaponFactoryDatabase;
#endif

void RegisterScriptedWeapon( const char *className )
{
#ifdef CLIENT_DLL
	if ( GetClassMap().FindFactory( className ) )
	{
		return;
	}

	GetClassMap().Add( className, "CHL2MPScriptedWeapon", sizeof( CHL2MPScriptedWeapon ),
		&CCHL2MPScriptedWeaponFactory, true );
#else
	if ( EntityFactoryDictionary()->FindFactory( className ) )
	{
		return;
	}

	unsigned short lookup = m_WeaponFactoryDatabase.Find( className );
	if ( lookup != m_WeaponFactoryDatabase.InvalidIndex() )
	{
		return;
	}

	// Andrew; This fixes months worth of pain and anguish.
	CEntityFactory<CHL2MPScriptedWeapon> *pFactory = new CEntityFactory<CHL2MPScriptedWeapon>( className );

	lookup = m_WeaponFactoryDatabase.Insert( className, pFactory );
	Assert( lookup != m_WeaponFactoryDatabase.InvalidIndex() );
#endif
	// BUGBUG: When attempting to precache weapons registered during runtime,
	// they don't appear as valid registered entities.
	// static CPrecacheRegister precache_weapon_(&CPrecacheRegister::PrecacheFn_Other, className);
}

void ResetWeaponFactoryDatabase( void )
{
#ifdef CLIENT_DLL
#ifdef LUA_SDK
	GetClassMap().RemoveAllScripted();
#endif
#else
	for ( int i=m_WeaponFactoryDatabase.First(); i != m_WeaponFactoryDatabase.InvalidIndex(); i=m_WeaponFactoryDatabase.Next( i ) )
	{
		delete m_WeaponFactoryDatabase[ i ];
	}
	m_WeaponFactoryDatabase.RemoveAll();
#endif
}

// These functions serve as skeletons for the our weapons' actions to be
// implemented in Lua.
acttable_t *CHL2MPScriptedWeapon::ActivityList( void ) {
#ifdef LUA_SDK
    if ( !PushTableFromRef( L, m_nTableReference ) )
        return m_acttable;

    lua_getfield( L, -1, "m_acttable" );
    lua_remove( L, -2 );

    if ( lua_istable( L, -1 ) )
    {
        for( int i = 0 ; i < LUA_MAX_WEAPON_ACTIVITIES ; i++ )
        {
            lua_pushinteger( L, i );
            lua_gettable( L, -2 );
            if ( lua_istable( L, -1 ) )
            {
                m_acttable[i].baseAct = ACT_INVALID;
                lua_pushinteger( L, 1 );
                lua_gettable( L, -2 );
                if ( lua_isnumber( L, -1 ) )
                    m_acttable[i].baseAct = lua_tointeger( L, -1 );
                lua_pop( L, 1 );

                m_acttable[i].weaponAct = ACT_INVALID;
                lua_pushinteger( L, 2 );
                lua_gettable( L, -2 );
                if ( lua_isnumber( L, -1 ) )
                    m_acttable[i].weaponAct = lua_tointeger( L, -1 );
                lua_pop( L, 1 );

                m_acttable[i].required = false;
                lua_pushinteger( L, 3 );
                lua_gettable( L, -2 );
                if ( lua_isboolean( L, -1 ) )
                    m_acttable[i].required = (bool)lua_toboolean( L, -1 );
                lua_pop( L, 1 );
            }
            lua_pop( L, 1 );
        }
    }
    lua_pop( L, 1 );
#endif
    return m_acttable;
}
int CHL2MPScriptedWeapon::ActivityListCount( void ) { return LUA_MAX_WEAPON_ACTIVITIES; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL2MPScriptedWeapon::CHL2MPScriptedWeapon( void )
{
    m_nTableReference = LUA_NOREF;
    m_pLuaWeaponInfo = dynamic_cast<CHL2MPSWeaponInfo*>( CreateWeaponInfo() );
    m_pLuaWeaponInfo->bParsedScript = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHL2MPScriptedWeapon::~CHL2MPScriptedWeapon( void )
{
	delete m_pLuaWeaponInfo;
	// Andrew; This is actually done in CBaseEntity. I'm doing it here because
	// this is the class that initialized the reference.
#ifdef LUA_SDK
	lua_unref( L, m_nTableReference );
#endif
}

extern const char *pWeaponSoundCategories[ NUM_SHOOT_SOUND_TYPES ];

#ifdef CLIENT_DLL
extern ConVar hud_fastswitch;
#endif

void CHL2MPScriptedWeapon::InitScriptedWeapon( void )
{
#if defined ( LUA_SDK )
	if ( m_nTableReference != LUA_NOREF )
		lua_unref( L, m_nTableReference );
	m_nTableReference = LUA_NOREF;

	char className[ MAX_WEAPON_STRING ];
#if defined ( CLIENT_DLL )
	if ( strlen( GetScriptedClassname() ) > 0 )
		Q_strncpy( className, GetScriptedClassname(), sizeof( className ) );
	else
		Q_strncpy( className, GetClassname(), sizeof( className ) );
#else
	Q_strncpy( m_iScriptedClassname.GetForModify(), GetClassname(), sizeof( className ) );
 	Q_strncpy( className, GetClassname(), sizeof( className ) );
#endif
 	Q_strlower( className );
	// Andrew; This redundancy is pretty annoying.
	// Classname
	Q_strncpy( m_pLuaWeaponInfo->szClassName, className, MAX_WEAPON_STRING );
	//SetClassname( className );

	lua_getglobal( L, "weapon" );
	if ( lua_istable( L, -1 ) )
	{
		lua_getfield( L, -1, "get" );
		if ( lua_isfunction( L, -1 ) )
		{
			lua_remove( L, -2 );
			lua_pushstring( L, className );
			luasrc_pcall( L, 1, 1, 0 );
			
			if ( !lua_istable( L, -1 ) )
			{
				//Warning( "weapon.get('%s') did not return a table!\n", className );
				lua_pop( L, 1 );
				lua_newtable( L );
			}
		}
		else
		{
			lua_pop( L, 2 );
			//Warning( "weapon.get is not a function!\n" );
			lua_newtable( L );
		}
	}
	else
	{
		lua_pop( L, 1 );
		//Warning( "weapon global is not a table!\n" );
		lua_newtable( L );
	}

	m_nTableReference = luaL_ref( L, LUA_REGISTRYINDEX );

#ifndef CLIENT_DLL
	m_pLuaWeaponInfo->bParsedScript = true;
#endif
	// Printable name
	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "PrintName" ))
		{
			if ( lua_isstring( L, -1 ) )
			{
				Q_strncpy( m_pLuaWeaponInfo->szPrintName, lua_tostring( L, -1 ), MAX_WEAPON_STRING );
			}
			else
			{
				Q_strncpy( m_pLuaWeaponInfo->szPrintName, WEAPON_PRINTNAME_MISSING, MAX_WEAPON_STRING );
			}
			lua_pop( L, 1 );
		}
	}
	else
	{
		Q_strncpy( m_pLuaWeaponInfo->szPrintName, WEAPON_PRINTNAME_MISSING, MAX_WEAPON_STRING );
	}

	// View model & world model
	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "ViewModel" ))
		{
			if ( lua_isstring( L, -1 ) )
			{
				Q_strncpy( m_pLuaWeaponInfo->szViewModel, lua_tostring( L, -1 ), MAX_WEAPON_STRING );
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "WorldModel" ))
		{
			if ( lua_isstring( L, -1 ) )
			{
				Q_strncpy( m_pLuaWeaponInfo->szWorldModel, lua_tostring( L, -1 ), MAX_WEAPON_STRING );
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "AnimPrefix" ))
		{
			if ( lua_isstring( L, -1 ) )
			{
				Q_strncpy( m_pLuaWeaponInfo->szAnimationPrefix, lua_tostring( L, -1 ), MAX_WEAPON_PREFIX );
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Slot" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->iSlot = (int)lua_tointeger( L, -1 );
			}
			else if ( lua_isstring( L, -1 ) )
			{
				m_pLuaWeaponInfo->iSlot = atoi( lua_tostring( L, -1 ) );
			}

			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "DeploySpeed" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->fDeploySpeed = lua_tonumber( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->fDeploySpeed = 1.0f;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "SlotPos" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->iPosition = (int)lua_tonumber( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->iPosition = 0;
			}
			lua_pop( L, 1 );
		}
	}

	// Use the console (X360) buckets if hud_fastswitch is set to 2.
#ifdef CLIENT_DLL
	if ( hud_fastswitch.GetInt() == 2 )
#else
	if ( IsX360() )
#endif
	{
		if ( PushTableFromRef( L, m_nTableReference ) )
		{
			if (GetFieldRemoveTable( L, "Slot_360" ))
			{
				if ( lua_isnumber( L, -1 ) )
				{
					m_pLuaWeaponInfo->iSlot = (int)lua_tonumber( L, -1 );
				}
				lua_pop( L, 1 );
			}
		}
		if ( PushTableFromRef( L, m_nTableReference ) )
		{
			if (GetFieldRemoveTable( L, "SlotPos_360" ))
			{
				if ( lua_isnumber( L, -1 ) )
				{
					m_pLuaWeaponInfo->iPosition = (int)lua_tonumber( L, -1 );
				}
				lua_pop( L, 1 );
			}
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Primary" ))
		{
			if ( lua_istable( L, -1 ) )
			{
				lua_getfield( L, -1, "ClipSize" );
				m_pLuaWeaponInfo->iMaxClip1 = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : WEAPON_NOCLIP;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "DefaultClip" );
				m_pLuaWeaponInfo->iDefaultClip1 = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : m_pLuaWeaponInfo->iMaxClip1;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "Automatic" );
				if ( lua_isboolean( L, -1 ) )
					m_pLuaWeaponInfo->m_bPrimaryAutomatic = !!lua_toboolean( L, -1 );
				else
					m_pLuaWeaponInfo->m_bPrimaryAutomatic = false;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "Ammo" );
				if ( lua_isstring( L, -1 ) )
				{
					const char* ammo = lua_tostring( L, -1 );
					if ( Q_stricmp( ammo, "none" ) == 0 )
						Q_strncpy( m_pLuaWeaponInfo->szAmmo1, "", sizeof( m_pLuaWeaponInfo->szAmmo1 ) );
					else
						Q_strncpy( m_pLuaWeaponInfo->szAmmo1, ammo, sizeof( m_pLuaWeaponInfo->szAmmo1 ) );
					m_pLuaWeaponInfo->iAmmoType = GetAmmoDef()->Index( m_pLuaWeaponInfo->szAmmo1 );
				}
				lua_pop( L, 1 );
			}
			lua_pop( L, 1 );
		}
	}

#ifdef HL2SB
	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "UseHands" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				UseHands = !!lua_toboolean( L, -1 );
			}
			else
			{
				UseHands = true;
			}
			lua_pop( L, 1 );
		}
	}
#endif

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Weight" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->iWeight = (int)lua_tonumber( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->iWeight = 0;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Rumble" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->iRumbleEffect = (int)lua_tonumber( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->iRumbleEffect = -1;
			}

			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "ShowUsageHint" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->bShowUsageHint = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->bShowUsageHint = false;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "autoswitchto" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->bAutoSwitchTo = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->bAutoSwitchTo = true;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "autoswitchfrom" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->bAutoSwitchFrom = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->bAutoSwitchFrom = true;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "BuiltRightHanded" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->m_bBuiltRightHanded = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->m_bBuiltRightHanded = true;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "AllowFlipping" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->m_bAllowFlipping = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->m_bAllowFlipping = true;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "MeleeWeapon" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->m_bMeleeWeapon = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->m_bMeleeWeapon = false;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Secondary" ))
		{
			if ( lua_istable( L, -1 ) )
			{
				lua_getfield( L, -1, "ClipSize" );
				m_pLuaWeaponInfo->iMaxClip2 = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : WEAPON_NOCLIP;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "DefaultClip" );
				m_pLuaWeaponInfo->iDefaultClip2 = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : m_pLuaWeaponInfo->iMaxClip2;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "Automatic" );
				if ( lua_isboolean( L, -1 ) )
					m_pLuaWeaponInfo->m_bSecondaryAutomatic = !!lua_toboolean( L, -1 );
				else
					m_pLuaWeaponInfo->m_bSecondaryAutomatic = false;
				lua_pop( L, 1 );

				lua_getfield( L, -1, "Ammo" );
				if ( lua_isstring( L, -1 ) )
				{
					const char* ammo = lua_tostring( L, -1 );
					if ( Q_stricmp( ammo, "none" ) == 0 )
						Q_strncpy( m_pLuaWeaponInfo->szAmmo2, "", sizeof( m_pLuaWeaponInfo->szAmmo2 ) );
					else
						Q_strncpy( m_pLuaWeaponInfo->szAmmo2, ammo, sizeof( m_pLuaWeaponInfo->szAmmo2 ) );
					m_pLuaWeaponInfo->iAmmo2Type = GetAmmoDef()->Index( m_pLuaWeaponInfo->szAmmo2 );
				}
				lua_pop( L, 1 );
			}
			lua_pop( L, 1 );
		}
	}

	memset( m_pLuaWeaponInfo->aShootSounds, 0, sizeof( m_pLuaWeaponInfo->aShootSounds ) );
	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "SoundData" ) )
		{
			if ( lua_istable( L, -1 ) )
			{
				for ( int i = EMPTY; i < NUM_SHOOT_SOUND_TYPES; i++ )
				{
					lua_getfield( L, -1, pWeaponSoundCategories[i] );
					if ( lua_isstring( L, -1 ) )
					{
						const char *soundname = lua_tostring( L, -1 );
						if ( soundname && soundname[0] )
						{
							Q_strncpy( m_pLuaWeaponInfo->aShootSounds[i], soundname, MAX_WEAPON_STRING );
						}
					}
					lua_pop( L, 1 );
				}
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "IronsightPosOffset" ))
		{
			if ( lua_isuserdata( L, -1 ) )
			{
				m_pLuaWeaponInfo->vecIronsightPosOffset = luaL_checkvector(L, -1);
			}
			else
			{
				m_pLuaWeaponInfo->vecIronsightPosOffset = Vector( 0, 0, 0 );
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "ViewModelFOV" ) )
		{
			if ( lua_isnumber( L, -1 ) )
			{
				flViewModelFOV = (float)lua_tonumber( L, -1 );
			}
			else
			{
				flViewModelFOV = 54.f; // default
			}
			lua_pop( L, 1 );
		}
	}

    if ( PushTableFromRef( L, m_nTableReference ) )
    {
        if (GetFieldRemoveTable( L, "TextureData" ))
		{
			if ( lua_istable( L, -1 ) )
			{
				lua_pushnil( L );
				while ( lua_next( L, -2 ) )
				{
					const char *texName = lua_tostring( L, -2 );
					if ( texName && lua_istable( L, -1 ) )
					{
						lua_getfield( L, -1, "file" );
						const char *file = lua_isstring( L, -1 ) ? lua_tostring( L, -1 ) : NULL;
						lua_pop( L, 1 );

						lua_getfield( L, -1, "x" );
						int x = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : 0;
						lua_pop( L, 1 );

						lua_getfield( L, -1, "y" );
						int y = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : 0;
						lua_pop( L, 1 );

						lua_getfield( L, -1, "width" );
						int w = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : 0;
						lua_pop( L, 1 );

						lua_getfield( L, -1, "height" );
						int h = lua_isnumber( L, -1 ) ? (int)lua_tointeger( L, -1 ) : 0;
						lua_pop( L, 1 );
		
	#ifdef CLIENT_DLL
					CHudTexture temp;
					Q_strncpy( temp.szShortName, texName, sizeof( temp.szShortName ) );

					if ( file )
					{
						Q_strncpy( temp.szTextureFile, file, sizeof( temp.szTextureFile ) );
					}
					else
					{
						temp.szTextureFile[0] = '\0';
					}

					temp.bRenderUsingFont = false;
					temp.cCharacterInFont = 0;
					temp.hFont = 0;
					temp.bPrecached = false;
					temp.textureId = -1;

					temp.rc.left   = x;
					temp.rc.top    = y;
					temp.rc.right  = x + w;
					temp.rc.bottom = y + h;

					CHudTexture *pRegistered = gHUD.AddUnsearchableHudIconToList( temp );
					if ( pRegistered )
					{
						pRegistered->Precache();
						if ( Q_stricmp( texName, "weapon_s" ) == 0 )
							m_pLuaWeaponInfo->iconActive = pRegistered;
						else if ( Q_stricmp( texName, "weapon" ) == 0 )
							m_pLuaWeaponInfo->iconInactive = pRegistered;
						else if ( Q_stricmp( texName, "weapon_small" ) == 0 )
							m_pLuaWeaponInfo->iconSmall = pRegistered;
						else if ( Q_stricmp( texName, "ammo" ) == 0 )
							m_pLuaWeaponInfo->iconAmmo = pRegistered;
						else if ( Q_stricmp( texName, "ammo2" ) == 0 )
							m_pLuaWeaponInfo->iconAmmo2 = pRegistered;
						else if ( Q_stricmp( texName, "crosshair" ) == 0 )
							m_pLuaWeaponInfo->iconCrosshair = pRegistered;
						else if ( Q_stricmp( texName, "autoaim" ) == 0 )
							m_pLuaWeaponInfo->iconAutoaim = pRegistered;
					}
					else
					{
						DevMsg( "Failed to register HUD texture %s (file=%s)\n", texName, file ? file : "(null)" );
					}
	#endif
					}

					lua_pop( L, 1 );
				}

			}
			lua_pop( L, 1 );
		}
    }

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "IronsightAngOffset" ))
		{
			if ( lua_isuserdata( L, -1 ) )
			{
				m_pLuaWeaponInfo->angIronsightAngOffset = luaL_checkangle( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->angIronsightAngOffset = QAngle( 0, 0, 0 );
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "IronsightFOVOffset" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->flIronsightFOVOffset = ( float )lua_tonumber( L, -1 );
			}
			else
			{
				m_pLuaWeaponInfo->flIronsightFOVOffset = 0.0f;
			}
			lua_pop( L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "CanUseIronsight" ))
		{
			if ( lua_isboolean( L, -1 ) )
			{
				m_pLuaWeaponInfo->bCanUseIronsight = ( lua_toboolean( L, -1 ) != 0 );
			}
			else
			{
				m_pLuaWeaponInfo->bCanUseIronsight = false;
			}
			lua_pop(L, 1 );
		}
	}

	if ( PushTableFromRef( L, m_nTableReference ) )
	{
		if (GetFieldRemoveTable( L, "Damage" ))
		{
			if ( lua_isnumber( L, -1 ) )
			{
				m_pLuaWeaponInfo->m_iPlayerDamage = (int)lua_tointeger( L, -1 );
			}
			lua_pop( L, 1 );
		}
	}

	BEGIN_LUA_CALL_WEAPON_METHOD( "Initialize" );
	END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
}

#ifdef CLIENT_DLL
void CHL2MPScriptedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( m_iScriptedClassname.Get() && !m_pLuaWeaponInfo->bParsedScript )
		{
			m_pLuaWeaponInfo->bParsedScript = true;
			SetClassname( m_iScriptedClassname.Get() );
			InitScriptedWeapon();

#ifdef LUA_SDK
			BEGIN_LUA_CALL_WEAPON_METHOD( "Precache" );
			END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
		}
	}
}

const char *CHL2MPScriptedWeapon::GetScriptedClassname( void )
{
	if ( m_iScriptedClassname.Get() )
		return m_iScriptedClassname.Get();
	return BaseClass::GetClassname();
}
#endif

void CHL2MPScriptedWeapon::Precache( void )
{
	BaseClass::Precache();

	InitScriptedWeapon();

	// Get the ammo indexes for the ammo's specified in the data file
	if ( GetWpnData().szAmmo1[0] )
	{
		m_iPrimaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo1 );
		if (m_iPrimaryAmmoType == -1)
		{
			Msg("ERROR: Weapon (%s) using undefined primary ammo type (%s)\n",GetClassname(), GetWpnData().szAmmo1);
		}
	}
	if ( GetWpnData().szAmmo2[0] )
	{
		m_iSecondaryAmmoType = GetAmmoDef()->Index( GetWpnData().szAmmo2 );
		if (m_iSecondaryAmmoType == -1)
		{
			Msg("ERROR: Weapon (%s) using undefined secondary ammo type (%s)\n",GetClassname(),GetWpnData().szAmmo2);
		}
	}

	// Precache models (preload to avoid hitch)
	m_iViewModelIndex = 0;
	m_iWorldModelIndex = 0;
	if ( GetViewModel() && GetViewModel()[0] )
	{
		m_iViewModelIndex = CBaseEntity::PrecacheModel( GetViewModel() );
	}
	if ( GetWorldModel() && GetWorldModel()[0] )
	{
		m_iWorldModelIndex = CBaseEntity::PrecacheModel( GetWorldModel() );
	}

	// Precache sounds, too
	for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i )
	{
		const char *shootsound = GetShootSound( i );
		if ( shootsound && shootsound[0] )
		{
			CBaseEntity::PrecacheScriptSound( shootsound );
		}
	}

#if defined ( LUA_SDK ) && !defined( CLIENT_DLL )
	BEGIN_LUA_CALL_WEAPON_METHOD( "Precache" );
	END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Get my data in the file weapon info array
//-----------------------------------------------------------------------------
const FileWeaponInfo_t &CHL2MPScriptedWeapon::GetWpnData( void ) const
{
	return *m_pLuaWeaponInfo;
}

const char *CHL2MPScriptedWeapon::GetViewModel( int ) const
{
    if (m_nTableReference == LUA_NOREF)
    {
        const char *model = m_pLuaWeaponInfo->szViewModel;
        return (model && model[0]) ? model : NULL;
    }

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->szViewModel;

	if (GetFieldRemoveTable( L, "ViewModel" ))
		RETURN_LUA_STRING();
#endif

	return m_pLuaWeaponInfo->szViewModel;
}

const char *CHL2MPScriptedWeapon::GetWorldModel( void ) const
{
    if (m_nTableReference == LUA_NOREF)
    {
        const char *model = m_pLuaWeaponInfo->szWorldModel;
        return (model && model[0]) ? model : NULL;
    }

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->szWorldModel;

	if (GetFieldRemoveTable( L, "WorldModel" ))
		RETURN_LUA_STRING();
#endif

	return m_pLuaWeaponInfo->szWorldModel;
}

const char *CHL2MPScriptedWeapon::GetAnimPrefix( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->szAnimationPrefix;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->szAnimationPrefix;

	if (GetFieldRemoveTable( L, "AnimPrefix" ))
		RETURN_LUA_STRING();
#endif

	return m_pLuaWeaponInfo->szAnimationPrefix;
}

const char *CHL2MPScriptedWeapon::GetPrintName( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->szPrintName;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->szPrintName;

	if (GetFieldRemoveTable( L, "PrintName" ))
		RETURN_LUA_STRING();
#endif

	return m_pLuaWeaponInfo->szPrintName;
}

int CHL2MPScriptedWeapon::GetMaxClip1( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return -1;

#if defined ( LUA_SDK )
    if ( !PushTableFromRef( L, m_nTableReference ) )
        return m_pLuaWeaponInfo->iMaxClip1;

    if (GetFieldRemoveTable( L, "Primary" ))
	{
		if ( !lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 );
			return m_pLuaWeaponInfo->iMaxClip1;
		}

		lua_getfield( L, -1, "ClipSize" );

		int v = m_pLuaWeaponInfo->iMaxClip1;
		if ( lua_isnumber( L, -1 ) )
		{
			v = (int)lua_tointeger( L, -1 );
		}

		lua_pop( L, 2 );
		return v;
	}
#endif

	return m_pLuaWeaponInfo->iMaxClip1;
}

int CHL2MPScriptedWeapon::GetMaxClip2( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iMaxClip2;

#if defined ( LUA_SDK )
    if ( !PushTableFromRef( L, m_nTableReference ) )
        return m_pLuaWeaponInfo->iMaxClip2;

    if (GetFieldRemoveTable( L, "Secondary" ))
	{
		if ( !lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 );
			return m_pLuaWeaponInfo->iMaxClip2;
		}

		lua_getfield( L, -1, "ClipSize" );

		int v = m_pLuaWeaponInfo->iMaxClip2;
		if ( lua_isnumber( L, -1 ) )
		{
			v = (int)lua_tointeger( L, -1 );
		}

		lua_pop( L, 2 );
		
    	return v;
	}
#endif

	return m_pLuaWeaponInfo->iMaxClip2;
}

int CHL2MPScriptedWeapon::GetDefaultClip1( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iDefaultClip1;

#if defined ( LUA_SDK )
    if ( !PushTableFromRef( L, m_nTableReference ) )
        return m_pLuaWeaponInfo->iDefaultClip1;

    if (GetFieldRemoveTable( L, "Primary" ))
	{
		if ( !lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 );
			return m_pLuaWeaponInfo->iDefaultClip1;
		}

		lua_getfield( L, -1, "DefaultClip" );

		int v = m_pLuaWeaponInfo->iDefaultClip1;
		if ( lua_isnumber( L, -1 ) )
		{
			v = (int)lua_tointeger( L, -1 );
		}

		lua_pop( L, 2 );
		return v;
	}
#endif

	return m_pLuaWeaponInfo->iDefaultClip1;
}

int CHL2MPScriptedWeapon::GetDefaultClip2( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iDefaultClip2;

#if defined ( LUA_SDK )
    if ( !PushTableFromRef( L, m_nTableReference ) )
        return m_pLuaWeaponInfo->iDefaultClip2;

    if (GetFieldRemoveTable( L, "Secondary" ))
	{
		if ( !lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 );
			return m_pLuaWeaponInfo->iDefaultClip2;
		}

		lua_getfield( L, -1, "DefaultClip" );

		int v = m_pLuaWeaponInfo->iDefaultClip2;
		if ( lua_isnumber( L, -1 ) )
		{
			v = (int)lua_tointeger( L, -1 );
		}

		lua_pop( L, 2 );
		return v;
	}
#endif

	return m_pLuaWeaponInfo->iDefaultClip2;
}


bool CHL2MPScriptedWeapon::IsMeleeWeapon() const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->m_bMeleeWeapon;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->m_bMeleeWeapon;

	if (GetFieldRemoveTable( L, "MeleeWeapon" ))
	{
		if ( lua_gettop( L ) == 1 )
		{
			if ( lua_isnumber( L, -1 ) )
			{
				int res = ( (int)lua_tointeger( L, -1 ) != 0 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else if ( lua_isboolean( L, -1 ) )
			{
				int res = lua_toboolean( L, -1 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else
				lua_pop(L, 1);
		}
	}
#endif

	return m_pLuaWeaponInfo->m_bMeleeWeapon;
}

#ifdef CLIENT_DLL
void CHL2MPScriptedWeapon::CallDrawHUD()
{
#if defined( LUA_SDK )
    if ( m_nTableReference == LUA_NOREF )
        return;

    if ( !PushTableFromRef( L, m_nTableReference ) )
        return;

    BEGIN_LUA_CALL_WEAPON_METHOD( "DrawHUD" );
    END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
}
#endif

bool CHL2MPScriptedWeapon::DrawAmmo() const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iconAmmo != nullptr;

#if defined (LUA_SDK)
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->iconAmmo != nullptr;

	if (GetFieldRemoveTable( L, "DrawAmmo" ))
		RETURN_LUA_BOOLEAN();
#endif

	return m_pLuaWeaponInfo->iconAmmo != nullptr;
}

int CHL2MPScriptedWeapon::GetWeight( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iWeight;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->iWeight;

	if (GetFieldRemoveTable( L, "Weight" ))
		RETURN_LUA_INTEGER();
#endif

	return m_pLuaWeaponInfo->iWeight;
}

bool CHL2MPScriptedWeapon::AllowsAutoSwitchTo( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->bAutoSwitchTo;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->bAutoSwitchTo;

	if (GetFieldRemoveTable( L, "AutoSwitchTo" ))
	{
		if ( lua_gettop( L ) == 1 )
		{
			if ( lua_isnumber( L, -1 ) )
			{
				int res = ( (int)lua_tointeger( L, -1 ) != 0 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else if ( lua_isboolean( L, -1 ) )
			{
				int res = lua_toboolean( L, -1 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else
				lua_pop(L, 1);
		}
	}
#endif

	return m_pLuaWeaponInfo->bAutoSwitchTo;
}

bool CHL2MPScriptedWeapon::AllowsAutoSwitchFrom( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->bAutoSwitchFrom;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->bAutoSwitchFrom;

	if (GetFieldRemoveTable( L, "AutoSwitchFrom" ))
	{
		if ( lua_gettop( L ) == 1 )
		{
			if ( lua_isnumber( L, -1 ) )
			{
				int res = ( (int)lua_tointeger( L, -1 ) != 0 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else if ( lua_isboolean( L, -1 ) )
			{
				int res = lua_toboolean( L, -1 ) ? true : false;
				lua_pop(L, 1);
				return res;
			}
			else
				lua_pop(L, 1);
		}
	}
#endif

	return m_pLuaWeaponInfo->bAutoSwitchFrom;
}

bool CHL2MPScriptedWeapon::IsSpawnable( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return false;

#ifdef LUA_SDK
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return false;

	if (GetFieldRemoveTable( L, "Spawnable" ))
		RETURN_LUA_BOOLEAN();
#endif

	return false;
}

int CHL2MPScriptedWeapon::GetWeaponFlags( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iFlags;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->iFlags;

	if (GetFieldRemoveTable( L, "ItemFlags" ))
		RETURN_LUA_INTEGER();
#endif

	return m_pLuaWeaponInfo->iFlags;
}

int CHL2MPScriptedWeapon::GetSlot( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iSlot;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->iSlot;

	if (GetFieldRemoveTable( L, "Slot" ))
		RETURN_LUA_INTEGER();
#endif

	return m_pLuaWeaponInfo->iSlot;
}

int CHL2MPScriptedWeapon::GetPosition( void ) const
{
	if (m_nTableReference == LUA_NOREF)
		return m_pLuaWeaponInfo->iPosition;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return m_pLuaWeaponInfo->iPosition;

	if (GetFieldRemoveTable( L, "SlotPos" ))
		RETURN_LUA_INTEGER();
#endif

	return m_pLuaWeaponInfo->iPosition;
}

const Vector &CHL2MPScriptedWeapon::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CHL2MPScriptedWeapon::PrimaryAttack( void )
{
	if (m_nTableReference == LUA_NOREF)
		return;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return;

	BEGIN_LUA_CALL_WEAPON_METHOD( "PrimaryAttack" );
	END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
}

void CHL2MPScriptedWeapon::SecondaryAttack( void )
{
	if (m_nTableReference == LUA_NOREF)
		return;

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return;

	BEGIN_LUA_CALL_WEAPON_METHOD( "SecondaryAttack" );
	END_LUA_CALL_WEAPON_METHOD( 0, 0 );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CHL2MPScriptedWeapon::FireBullets( const FireBulletsInfo_t &info )
{
	CHL2MP_Player *pHL2MPPlayer = ToHL2MPPlayer( ToBasePlayer( GetOwner() ) );
	if (!pHL2MPPlayer) return;

	pHL2MPPlayer->FireBullets( info );
}

bool CHL2MPScriptedWeapon::Reload( void )
{
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::Reload();

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return BaseClass::Reload();

	BEGIN_LUA_CALL_WEAPON_METHOD( "Reload" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	RETURN_LUA_BOOLEAN();
#endif

	return BaseClass::Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPScriptedWeapon::Deploy( void )
{
#ifdef HL2SB
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( GetOwner() );
	if (!pPlayer)
		return false;

	if (!UseHands)
	{
		CBaseViewModel *pHandModel = pPlayer->GetViewModel(1);
		if ( pHandModel )
		{ 
			pHandModel->SetModel("");
			pHandModel->m_nSkin = 0;
		}
	}

#ifdef GAME_DLL
	// dirty hack
	pPlayer->m_bPredictWeapons  = false;
#endif
#endif

	//if (m_nTableReference == LUA_NOREF)
	// @ThePixelMoon: way too dumb to not execute the original one if
	// lua one exists

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return BaseClass::Deploy();
	else
		BaseClass::Deploy();

	BEGIN_LUA_CALL_WEAPON_METHOD( "Deploy" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	RETURN_LUA_BOOLEAN();
#endif

	return BaseClass::Deploy();
}

Activity CHL2MPScriptedWeapon::GetDrawActivity( void )
{
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::GetDrawActivity();

#if defined ( LUA_SDK )
	BEGIN_LUA_CALL_WEAPON_METHOD( "GetDrawActivity" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	// Kind of lame, but we're required to explicitly cast
	RETURN_LUA_ACTIVITY();
#endif

	return BaseClass::GetDrawActivity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPScriptedWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifdef HL2SB
	if ( !UseHands )
	{
#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetOwner());
	if ( pPlayer )
	{
		CUtlString desiredModel;
		int desiredSkin = 0;
		const char* c_handmodel = engine->GetClientConVarValue( ENTINDEX( pPlayer->edict() ), "c_handmodel" );

		BEGIN_LUA_CALL_HOOK("GetPlayerHandModel")
			lua_pushhl2mpplayer(L, pPlayer);
			lua_pushstring(L, c_handmodel);
			lua_pushinteger(L, pPlayer->GetPlayerModelType());
		END_LUA_CALL_HOOK(3, 2);

		desiredModel = lua_tostring(L, -2);
		desiredSkin = (int)lua_tointeger(L, -1);

		lua_pop(L, 2);

		//if (pPlayer->m_CurrentHandModel != desiredModel)
		{
			CBaseViewModel *pHandModel = pPlayer->GetViewModel(1);
			if ( pHandModel )
			{ 
				pHandModel->SetModel(desiredModel.Get());
				pHandModel->m_nSkin = desiredSkin;
				pPlayer->m_CurrentHandModel = desiredModel;
			}
		}

#ifdef GAME_DLL
		#define QUICKGETCVARVALUE(v) (engine->GetClientConVarValue( pPlayer->entindex(), v ))

		if ( gpGlobals->maxClients == 1 )
			pPlayer->m_bPredictWeapons  = false;
		else
			pPlayer->m_bPredictWeapons  = Q_atoi( QUICKGETCVARVALUE("cl_predictweapons")) != 0;
#endif
	}
#endif
	}
#endif

	//if (m_nTableReference == LUA_NOREF)
	// @ThePixelMoon: way too dumb to not execute the original one if
	// lua one exists

#if defined ( LUA_SDK )
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::Holster( pSwitchingTo );
	else
		BaseClass::Holster( pSwitchingTo );

	BEGIN_LUA_CALL_WEAPON_METHOD( "Holster" );
		lua_pushweapon( L, pSwitchingTo );
	END_LUA_CALL_WEAPON_METHOD( 1, 1 );

	RETURN_LUA_BOOLEAN();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPScriptedWeapon::ItemPostFrame( void )
{
	if (m_nTableReference == LUA_NOREF)
		return;

	BaseClass::ItemPostFrame();

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return;

	BEGIN_LUA_CALL_WEAPON_METHOD( "ItemPostFrame" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	RETURN_LUA_NONE();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called each frame by the player PostThink, if the player's not ready to attack yet
//-----------------------------------------------------------------------------
void CHL2MPScriptedWeapon::ItemBusyFrame( void )
{
	if (m_nTableReference == LUA_NOREF)
		return;

	BaseClass::ItemBusyFrame();

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return;

	BEGIN_LUA_CALL_WEAPON_METHOD( "ItemBusyFrame" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	RETURN_LUA_NONE();
#endif
}

#ifndef CLIENT_DLL
int CHL2MPScriptedWeapon::CapabilitiesGet( void )
{
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::CapabilitiesGet();

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return BaseClass::CapabilitiesGet();

	BEGIN_LUA_CALL_WEAPON_METHOD( "CapabilitiesGet" );
	END_LUA_CALL_WEAPON_METHOD( 0, 1 );

	RETURN_LUA_INTEGER();
#endif

	return BaseClass::CapabilitiesGet();
}
#else
int CHL2MPScriptedWeapon::DrawModel( int flags )
{
	if ( m_nTableReference == LUA_NOREF )
		return BaseClass::DrawModel( flags );

#if defined ( LUA_SDK )
	if ( !PushTableFromRef( L, m_nTableReference ) )
		return BaseClass::DrawModel( flags );

	BEGIN_LUA_CALL_WEAPON_METHOD( "DrawModel" );
		lua_pushinteger( L, flags );
	END_LUA_CALL_WEAPON_METHOD( 1, 1 );

	RETURN_LUA_INTEGER();
#endif

	return BaseClass::DrawModel( flags );
}
#endif
