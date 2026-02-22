//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//



// Client-side CBasePlayer

#ifndef C_PHYSBOX_H
#define C_PHYSBOX_H
#pragma once


#ifdef SBPP
#include "c_baseanimating.h"


class C_PhysBox : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_PhysBox, C_BaseAnimating );
#else
#include "c_baseentity.h"


class C_PhysBox : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_PhysBox, C_BaseEntity );
#endif
	DECLARE_CLIENTCLASS();

					C_PhysBox();
	virtual			~C_PhysBox();
	virtual ShadowType_t ShadowCastType();
	
public:
	float			m_mass;	// TEST..
};


#endif



