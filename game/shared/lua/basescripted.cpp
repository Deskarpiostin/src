//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#include "cbase.h"
#include "basescripted.h"
#include "luamanager.h"
#ifdef CLIENT_DLL
#include "lc_baseanimating.h"
#else
#include "lbaseanimating.h"
#endif
#include "lbaseentity_shared.h"
#include "lvphysics_interface.h"
#include "ltakedamageinfo.h"
#include "takedamageinfo.h"
#include "mathlib/lvector.h"
#ifdef GAME_DLL
#include "variant_t.h"
#endif


ConVar sv_spawnmenu_allowed("sv_spawnmenu_allowed", "1", FCVAR_REPLICATED);

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( BaseScripted, DT_BaseScripted )

BEGIN_NETWORK_TABLE( CBaseScripted, DT_BaseScripted )
#ifdef CLIENT_DLL
	RecvPropString( RECVINFO( m_iScriptedClassname ) ),
#else
	SendPropString( SENDINFO( m_iScriptedClassname ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(	CBaseScripted )
END_PREDICTION_DATA()

#ifdef CLIENT_DLL
static C_BaseEntity *CCBaseScriptedFactory( void )
{
	return static_cast< C_BaseEntity * >( new CBaseScripted );
};
#endif

#ifndef CLIENT_DLL
static CUtlDict< CEntityFactory<CBaseScripted>*, unsigned short > m_EntityFactoryDatabase;
#endif

void RegisterScriptedEntity( const char *className )
{
#ifdef CLIENT_DLL
	if ( GetClassMap().FindFactory( className ) )
	{
		return;
	}

	GetClassMap().Add( className, "CBaseScripted", sizeof( CBaseScripted ),
		&CCBaseScriptedFactory, true );
#else
	if ( EntityFactoryDictionary()->FindFactory( className ) )
	{
		return;
	}

	unsigned short lookup = m_EntityFactoryDatabase.Find( className );
	if ( lookup != m_EntityFactoryDatabase.InvalidIndex() )
	{
		return;
	}

	CEntityFactory<CBaseScripted> *pFactory = new CEntityFactory<CBaseScripted>( className );

	lookup = m_EntityFactoryDatabase.Insert( className, pFactory );
	Assert( lookup != m_EntityFactoryDatabase.InvalidIndex() );
#endif
}

void ResetEntityFactoryDatabase( void )
{
#ifdef CLIENT_DLL
#ifdef LUA_SDK
	GetClassMap().RemoveAllScripted();
#endif
#else
	for ( int i=m_EntityFactoryDatabase.First(); i != m_EntityFactoryDatabase.InvalidIndex(); i=m_EntityFactoryDatabase.Next( i ) )
	{
		delete m_EntityFactoryDatabase[ i ];
	}
	m_EntityFactoryDatabase.RemoveAll();
#endif
}

CBaseScripted::CBaseScripted( void )
{
#ifdef LUA_SDK
	// UNDONE: We're done in CBaseEntity
	m_nTableReference = LUA_NOREF;
#endif
}

CBaseScripted::~CBaseScripted( void )
{
    SetTouch( nullptr );
    SetThink( nullptr );

#ifdef LUA_SDK
    if ( m_nTableReference != LUA_NOREF )
    {
        luaL_unref( L, LUA_REGISTRYINDEX, m_nTableReference );
        m_nTableReference = LUA_NOREF;
    }
#endif
}

void CBaseScripted::LoadScriptedEntity( void )
{
	lua_getglobal( L, "entity" );
	if ( lua_istable( L, -1 ) )
	{
		lua_getfield( L, -1, "get" );
		if ( lua_isfunction( L, -1 ) )
		{
			lua_remove( L, -2 );
			lua_pushstring( L, GetClassname() );
			luasrc_pcall( L, 1, 1, 0 );
		}
		else
		{
			lua_pop( L, 2 );
		}
	}
	else
	{
		lua_pop( L, 1 );
	}
}

int CBaseScripted::OnTakeDamage(const CTakeDamageInfo &info)
{
	CTakeDamageInfo nonConstInfo = info;

	BEGIN_LUA_CALL_ENTITY_METHOD( "OnTakeDamage" );
		lua_pushdamageinfo( L, nonConstInfo );
	END_LUA_CALL_ENTITY_METHOD( 1, 0 );

#ifdef CLIENT_DLL
	return 0; // ... what
#else
	return BaseClass::OnTakeDamage(info);
#endif
}

void CBaseScripted::InitScriptedEntity( void )
{
#if defined ( LUA_SDK )
#ifndef CLIENT_DLL
	// Let the instance reinitialize itself for the client.
	if ( m_nTableReference != LUA_NOREF )
		return;
#endif

	SetThink( &CBaseScripted::Think );
#ifdef CLIENT_DLL
	SetNextClientThink( gpGlobals->curtime );
#endif
	SetNextThink( gpGlobals->curtime );

	SetTouch( &CBaseScripted::Touch );

	char className[ 255 ];
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
	SetClassname( className );

	if ( m_nTableReference == LUA_NOREF )
	{
		LoadScriptedEntity();
		m_nTableReference = luaL_ref( L, LUA_REGISTRYINDEX );
	}
	else
	{
		lua_getglobal( L, "table" );
		if ( lua_istable( L, -1 ) )
		{
			lua_getfield( L, -1, "merge" );
			if ( lua_isfunction( L, -1 ) )
			{
				lua_remove( L, -2 );
				lua_getref( L, m_nTableReference );
				LoadScriptedEntity();
				luasrc_pcall( L, 2, 0, 0 );
			}
			else
			{
				lua_pop( L, 2 );
			}
		}
		else
		{
			lua_pop( L, 1 );
		}
	}

	BEGIN_LUA_CALL_ENTITY_METHOD( "Initialize" );
	END_LUA_CALL_ENTITY_METHOD( 0, 0 );
#endif
}

#ifdef CLIENT_DLL
int CBaseScripted::DrawModel( int flags )
{
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::DrawModel( flags );

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return BaseClass::DrawModel( flags );
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "DrawModel" );
		lua_pushinteger( L, flags );
	END_LUA_CALL_ENTITY_METHOD( 1, 1 );

	RETURN_LUA_INTEGER();
#endif

	return BaseClass::DrawModel( flags );
}

void CBaseScripted::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( m_iScriptedClassname.Get() )
		{
			SetClassname( m_iScriptedClassname.Get() );
			InitScriptedEntity();
		}
	}
}

const char *CBaseScripted::GetScriptedClassname( void )
{
	if ( m_iScriptedClassname.Get() )
		return m_iScriptedClassname.Get();
	return BaseClass::GetClassname();
}
#endif

void CBaseScripted::Spawn( void )
{
	BaseClass::Spawn();

	m_takedamage = DAMAGE_YES;

#ifndef CLIENT_DLL
	InitScriptedEntity();
#endif
}

void CBaseScripted::Precache( void )
{
	BaseClass::Precache();

	// InitScriptedEntity();
}

#ifdef GAME_DLL

void lua_pushvariant(lua_State *L, const variant_t &value)
{
    switch (const_cast<variant_t&>(value).FieldType())
    {
        case FIELD_VOID:
            lua_pushnil(L);
            break;

        case FIELD_STRING:
            lua_pushstring(L, value.String());
            break;

        case FIELD_INTEGER:
            lua_pushinteger(L, value.Int());
            break;

        case FIELD_FLOAT:
            lua_pushnumber(L, value.Float());
            break;

        case FIELD_BOOLEAN:
            lua_pushboolean(L, value.Bool());
            break;

        case FIELD_VECTOR: {
            Vector vec;
			value.Vector3D(vec);

            lua_pushvector(L, vec);
            break;
        }

        case FIELD_EHANDLE:
        case FIELD_CLASSPTR:
        case FIELD_EDICT:
		{
            CBaseEntity *ent = value.Entity();
            if (ent)
                lua_pushentity(L, ent);
            else
                lua_pushnil(L);
            break;
        }

        default:
            lua_pushnil(L);
            break;
    }
}

bool CBaseScripted::AcceptInput(const char *inputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t value, int outputID)
{
#ifdef LUA_SDK
	if (m_nTableReference == LUA_NOREF)
		return BaseClass::AcceptInput(inputName, pActivator, pCaller, value, outputID);

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return BaseClass::AcceptInput(inputName, pActivator, pCaller, value, outputID);
	}

	BEGIN_LUA_CALL_ENTITY_METHOD("AcceptInput");
		lua_pushstring(L, inputName);
		lua_pushentity(L, pActivator);
		lua_pushentity(L, pCaller);
		lua_pushvariant(L, value);
		lua_pushinteger(L, outputID);
	END_LUA_CALL_ENTITY_METHOD(5, 1);

	RETURN_LUA_BOOLEAN();
#endif

	return BaseClass::AcceptInput(inputName, pActivator, pCaller, value, outputID);
}
#endif

#ifdef CLIENT_DLL
void CBaseScripted::ClientThink()
{
	if (m_nTableReference == LUA_NOREF)
		return;
	
	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "ClientThink" );
	END_LUA_CALL_ENTITY_METHOD( 0, 0 );
#endif
}
#endif

void CBaseScripted::Think()
{
	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "Think" );
	END_LUA_CALL_ENTITY_METHOD( 0, 0 );
#endif
}

void CBaseScripted::StartTouch( CBaseEntity *pOther )
{
	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "StartTouch" );
		lua_pushentity( L, pOther );
	END_LUA_CALL_ENTITY_METHOD( 1, 0 );
#endif
}

void CBaseScripted::Touch( CBaseEntity *pOther )
{
	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "Touch" );
		lua_pushentity( L, pOther );
	END_LUA_CALL_ENTITY_METHOD( 1, 0 );
#endif
}

void CBaseScripted::EndTouch( CBaseEntity *pOther )
{
	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "EndTouch" );
		lua_pushentity( L, pOther );
	END_LUA_CALL_ENTITY_METHOD( 1, 0 );
#endif
}

void CBaseScripted::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	if (m_nTableReference == LUA_NOREF)
		return;

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_nTableReference);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return;
	}

#ifdef LUA_SDK
	BEGIN_LUA_CALL_ENTITY_METHOD( "VPhysicsUpdate" );
		lua_pushphysicsobject( L, pPhysics );
	END_LUA_CALL_ENTITY_METHOD( 1, 0 );
#endif
}

