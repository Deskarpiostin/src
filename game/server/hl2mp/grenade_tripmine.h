//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_TRIPMINE_H
#define GRENADE_TRIPMINE_H
#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"

class CBeam;

#ifdef SBPP
// for constraints
#include "vphysics/constraints.h"
#endif

class CTripmineGrenade : public CBaseGrenade
{
public:
	DECLARE_CLASS( CTripmineGrenade, CBaseGrenade );

	CTripmineGrenade();
#ifdef SBPP
	~CTripmineGrenade();
#endif
	void Spawn( void );
	void Precache( void );

#ifdef SBPP
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
#else
#if 0 // FIXME: OnTakeDamage_Alive() is no longer called now that base grenade derives from CBaseAnimating
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
#endif
#endif	
	void WarningThink( void );
	void PowerupThink( void );
	void BeamBreakThink( void );
	void DelayDeathThink( void );
	void Event_Killed( const CTakeDamageInfo &info );

	void MakeBeam( void );
	void KillBeam( void );

#ifdef SBPP
	// Added to create a constraint
	void AttachToEntity( CBaseEntity *pOther );
	bool MakeConstraint( CBaseEntity *pOther );
#endif

public:
	EHANDLE		m_hOwner;

#ifdef SBPP
	// Added for following
	EHANDLE		m_hAttachEntity;
#endif
private:
	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;
#ifdef SBPP
	// signifies if we're attached to something, and need to update slightly differently.
	bool		m_bAttached;
	IPhysicsConstraint	*m_pConstraint;
	Vector		m_vAttachedPosition;	// if the attached position changes, we need to detonate
#endif

	DECLARE_DATADESC();
};

#endif // GRENADE_TRIPMINE_H
