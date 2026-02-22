//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "clientmode_hl2mpnormal.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

// Player entries in a death notice
struct DeathNoticePlayer
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	CHudTexture *iconDeath;
	int			iSuicide;
	float		flDisplayTime;
	bool		bHeadshot;

#ifdef SBPP
    bool        bKillerIsNPC;
    bool        bVictimIsNPC;
	bool		bNPCInflictorIsFriendly;
	bool		bNPCVictimIsFriendly;
	bool		bVictimIsPlayer;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );
public:
	CHudDeathNotice( const char *pElementName );

	void Init( void );
	void VidInit( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

#ifndef SBPP
	void SetColorForNoticePlayer( int iTeamNumber );
#endif
	void RetireExpiredDeathNotices( void );
	
	virtual void FireGameEvent( IGameEvent * event );

private:

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "15", "proportional_float" );

	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );

	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudNumbersTimer" );

	// Texture for skull symbol
	CHudTexture		*m_iconD_skull;  
	CHudTexture		*m_iconD_headshot;  

	CUtlVector<DeathNoticeItem> m_DeathNotices;

#ifdef SBPP
	CPanelAnimationVar( float, m_flTextYOffset, "TextYOffset", "4" );
#endif
};

using namespace vgui;

DECLARE_HUDELEMENT( CHudDeathNotice );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iconD_headshot = NULL;
	m_iconD_skull = NULL;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );	
#ifdef SBPP
	ListenForGameEvent("npc_killed");
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit( void )
{
	m_iconD_skull = gHUD.GetIcon( "d_skull" );
	m_DeathNotices.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

#ifndef SBPP
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::SetColorForNoticePlayer( int iTeamNumber )
{
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( iTeamNumber ) );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	if ( !m_iconD_skull )
		return;

	int yStart = GetClientModeHL2MPNormal()->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( GameResources()->GetTeamColor( 0 ) );


	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CHudTexture *icon = m_DeathNotices[i].iconDeath;
#ifdef SBPP
		if ( m_DeathNotices[i].iSuicide )
			icon = m_iconD_skull;

		if ( !icon )
			icon = m_iconD_skull;
#else
		if ( !icon )
			continue;
#endif

		wchar_t victim[ 256 ];
		wchar_t killer[ 256 ];

		// Get the team numbers for the players involved
		int iKillerTeam = 0;
		int iVictimTeam = 0;

		if( g_PR )
		{
			iKillerTeam = g_PR->GetTeam( m_DeathNotices[i].Killer.iEntIndex );
			iVictimTeam = g_PR->GetTeam( m_DeathNotices[i].Victim.iEntIndex );
		}

		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathNotices[i].Killer.szName, killer, sizeof( killer ) );

		// Get the local position for this notice
		int len = UTIL_ComputeStringWidth( m_hTextFont, victim );
		int y = yStart + (m_flLineHeight * i);

		int iconWide;
		int iconTall;

		if( icon->bRenderUsingFont )
		{
			iconWide = surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
			iconTall = surface()->GetFontTall( icon->hFont );
		}
		else
		{
			float scale = ( (float)ScreenHeight() / 480.0f );	//scale based on 640x480
			iconWide = (int)( scale * (float)icon->Width() );
			iconTall = (int)( scale * (float)icon->Height() );
		}

		int x;
		if ( m_bRightJustify )
		{
			x =	GetWide() - len - iconWide;
		}
		else
		{
			x = 0;
		}
#ifdef SBPP
		int yIcon = y;
		int yText = yIcon + (int)m_flTextYOffset;
#endif
		
		// Only draw killers name if it wasn't a suicide
		if ( !m_DeathNotices[i].iSuicide )
		{
			if ( m_bRightJustify )
			{
				x -= UTIL_ComputeStringWidth( m_hTextFont, killer );
			}

#ifdef SBPP
			if (m_DeathNotices[i].bKillerIsNPC)
			{
				if (m_DeathNotices[i].bNPCInflictorIsFriendly)
					surface()->DrawSetTextColor(Color(50, 200, 50, 255));
				else
					surface()->DrawSetTextColor(Color(250, 50, 50, 255));
			}
			else
				surface()->DrawSetTextColor(Color(250, 250, 50, 255));

			// Draw killer's name
			surface()->DrawSetTextPos( x, yText );
#else
			SetColorForNoticePlayer( iKillerTeam );

			// Draw killer's name
			surface()->DrawSetTextPos( x, y );
#endif
			surface()->DrawSetTextFont( m_hTextFont );
			surface()->DrawUnicodeString( killer );
			surface()->DrawGetTextPos( x, y );
		}

		Color iconColor( 255, 80, 0, 255 );

		// Draw death weapon
		//If we're using a font char, this will ignore iconTall and iconWide
		icon->DrawSelf( x, y, iconWide, iconTall, iconColor );
		x += iconWide;		

#ifdef SBPP
		if (m_DeathNotices[i].bVictimIsNPC)
		{
			if (m_DeathNotices[i].bNPCVictimIsFriendly)
				surface()->DrawSetTextColor(Color(50, 200, 50, 255));
			else
				surface()->DrawSetTextColor(Color(250, 50, 50, 255));
		}
		else
			surface()->DrawSetTextColor(Color(250, 250, 50, 255));
		
		// Draw victims name
		surface()->DrawSetTextPos( x, yText );
#else
		SetColorForNoticePlayer( iVictimTeam );

		// Draw victims name
		surface()->DrawSetTextPos( x, y );
#endif
		surface()->DrawSetTextFont( m_hTextFont );	//reset the font, draw icon can change it
		surface()->DrawUnicodeString( victim );
	}

	// Now retire any death notices that have expired
	RetireExpiredDeathNotices();
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::RetireExpiredDeathNotices( void )
{
	// Loop backwards because we might remove one
	int iSize = m_DeathNotices.Size();
	for ( int i = iSize-1; i >= 0; i-- )
	{
		if ( m_DeathNotices[i].flDisplayTime < gpGlobals->curtime )
		{
			m_DeathNotices.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CHudDeathNotice::FireGameEvent( IGameEvent * event )
{
#ifndef SBPP
	if (!g_PR)
		return;
#endif

	if ( hud_deathnotice_time.GetFloat() == 0 )
#ifdef SBPP
		return;

	char fullkilledwith[256];

	int killer;
	int victim;
	const char *killer_name;
	const char *victim_name;
	if (FStrEq(event->GetName(), "npc_killed"))
	{
		const char *attacker_name = event->GetString( "attacker_name" );
		const char *victim_name   = event->GetString( "victim_name" );
		bool attacker_is_player   = event->GetBool( "attacker_isplayer", false );
		bool npc_killer_friendly = event->GetBool( "npc_killer_friendly", false );
		bool npc_victim_friendly = event->GetBool( "npc_victim_friendly", false );
		bool victim_isplayer	 = event->GetBool( "victim_isplayer", false );

		if ( !attacker_name ) attacker_name = "";
		if ( !victim_name )   victim_name = "";

		const char *killedwith = event->GetString( "weaponname" );
		if ( killedwith && *killedwith )
		{
			Q_snprintf( fullkilledwith, sizeof(fullkilledwith), "death_%s", killedwith );
		}
		else
		{
			fullkilledwith[0] = 0;
		}

		killer = 0;
		victim = 0;
		killer_name = attacker_name;
		victim_name = victim_name;

		DeathNoticeItem deathMsg;
		deathMsg.Killer.iEntIndex = killer;
		deathMsg.Victim.iEntIndex = victim;
		Q_strncpy( deathMsg.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH );
		Q_strncpy( deathMsg.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH );
		deathMsg.bVictimIsNPC = !victim_isplayer;
    	deathMsg.bKillerIsNPC = !attacker_is_player;
		deathMsg.bNPCInflictorIsFriendly = npc_killer_friendly;
		deathMsg.bNPCVictimIsFriendly = npc_victim_friendly;
		deathMsg.bVictimIsPlayer	  = victim_isplayer;

		deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();

		if ( attacker_name[0] == '\0' || FStrEq( attacker_name, "world" ) )
		{
			deathMsg.iSuicide = 0; // just "died"
		}
		else
		{
			deathMsg.iSuicide = FStrEq( attacker_name, victim_name );
		}

		deathMsg.iconDeath = gHUD.GetIcon( fullkilledwith );
		if ( !deathMsg.iconDeath || deathMsg.iSuicide )
		{
			deathMsg.iconDeath = m_iconD_skull;
		}

		if ( m_DeathNotices.Count() > 0 && m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
		{
			m_DeathNotices.Remove( 0 );
		}
		m_DeathNotices.AddToTail( deathMsg );

		char sDeathMsg[512];

		if ( deathMsg.iSuicide )
		{
			if ( !strcmp( fullkilledwith, "d_worldspawn" ) )
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died.\n", deathMsg.Victim.szName );
			}
			else	//d_world
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided.\n", deathMsg.Victim.szName );
			}
		}
		else
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", deathMsg.Killer.szName, deathMsg.Victim.szName );

			if ( fullkilledwith && *fullkilledwith && (*fullkilledwith > 13 ) )
			{
				Q_strncat( sDeathMsg, VarArgs( " with %s.\n", fullkilledwith+6 ), sizeof( sDeathMsg ), COPY_ALL_CHARACTERS );
			}
		}

		Msg( "%s", sDeathMsg );

		return;
	}
	else
	{
		if (!g_PR)
			return;
		
		// the event should be "player_death"
		killer = engine->GetPlayerForUserID( event->GetInt("attacker") );
		victim = engine->GetPlayerForUserID( event->GetInt("userid") );
		const char *killedwith = event->GetString( "weapon" );

		if ( killedwith && *killedwith )
		{
			Q_snprintf( fullkilledwith, sizeof(fullkilledwith), "death_%s", killedwith );
		}
		else
		{
			fullkilledwith[0] = 0;
		}

		// Do we have too many death messages in the queue?
		if ( m_DeathNotices.Count() > 0 &&
			m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
		{
			// Remove the oldest one in the queue, which will always be the first
			m_DeathNotices.Remove(0);
		}

		// Get the names of the players
		killer_name = g_PR->GetPlayerName( killer );
		victim_name = g_PR->GetPlayerName( victim );

		if ( !killer_name )
			killer_name = "";
		if ( !victim_name )
			victim_name = "";
	}
#else
		return;

	// the event should be "player_death"
	int killer = engine->GetPlayerForUserID( event->GetInt("attacker") );
	int victim = engine->GetPlayerForUserID( event->GetInt("userid") );
	const char *killedwith = event->GetString( "weapon" );

	char fullkilledwith[128];
	if ( killedwith && *killedwith )
	{
		Q_snprintf( fullkilledwith, sizeof(fullkilledwith), "death_%s", killedwith );
	}
	else
	{
		fullkilledwith[0] = 0;
	}

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() >= (int)m_flMaxDeathNotices )
	{
		// Remove the oldest one in the queue, which will always be the first
		m_DeathNotices.Remove(0);
	}

	// Get the names of the players
	const char *killer_name = g_PR->GetPlayerName( killer );
	const char *victim_name = g_PR->GetPlayerName( victim );

	if ( !killer_name )
		killer_name = "";
	if ( !victim_name )
		victim_name = "";
#endif

	// Make a new death notice
	DeathNoticeItem deathMsg;
	deathMsg.Killer.iEntIndex = killer;
	deathMsg.Victim.iEntIndex = victim;
	Q_strncpy( deathMsg.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( deathMsg.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH );
	deathMsg.flDisplayTime = gpGlobals->curtime + hud_deathnotice_time.GetFloat();
	deathMsg.iSuicide = ( !killer || killer == victim );
#ifdef SBPP
	deathMsg.bVictimIsNPC = false;
    deathMsg.bKillerIsNPC = false;
	deathMsg.bNPCInflictorIsFriendly = false;
	deathMsg.bNPCVictimIsFriendly = false;
#endif

	// Try and find the death identifier in the icon list
	deathMsg.iconDeath = gHUD.GetIcon( fullkilledwith );

	if ( !deathMsg.iconDeath || deathMsg.iSuicide )
	{
		// Can't find it, so use the default skull & crossbones icon
		deathMsg.iconDeath = m_iconD_skull;
	}

	// Add it to our list of death notices
	m_DeathNotices.AddToTail( deathMsg );

	char sDeathMsg[512];

	// Record the death notice in the console
	if ( deathMsg.iSuicide )
	{
		if ( !strcmp( fullkilledwith, "d_worldspawn" ) )
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died.\n", deathMsg.Victim.szName );
		}
		else	//d_world
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided.\n", deathMsg.Victim.szName );
		}
	}
	else
	{
		Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", deathMsg.Killer.szName, deathMsg.Victim.szName );

		if ( fullkilledwith && *fullkilledwith && (*fullkilledwith > 13 ) )
		{
			Q_strncat( sDeathMsg, VarArgs( " with %s.\n", fullkilledwith+6 ), sizeof( sDeathMsg ), COPY_ALL_CHARACTERS );
		}
	}

	Msg( "%s", sDeathMsg );
}



