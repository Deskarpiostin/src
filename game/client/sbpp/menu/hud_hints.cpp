//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "hud_hints.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <tier0/memdbgon.h>
#include <engine/ivdebugoverlay.h>
#include <game/client/iviewport.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Panel.h>
#include "hudelement.h"
#include "cdll_client_int.h"
#include "clientmode_hl2mpnormal.h"
#include "luamanager.h"
#include "luasrclib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar cl_showhints( "cl_showhints", "1" );

NoticePanel::NoticePanel( Panel *parent ) : Panel( parent, "NoticePanel" )
{
	SetProportional( false );
	SetPaintEnabled( true );
	SetPaintBackgroundEnabled( true );

	m_pLabel = new Label( this, "NoticeLabel", "" );
	m_pLabel->SetContentAlignment( vgui::Label::a_center );
	m_pLabel->SetPaintEnabled( true );
	m_pLabel->SetVisible( true );

	HFont hFont = vgui::scheme()->GetIScheme( GetScheme() )->GetFont( "Roboto", true );
	if ( hFont )
		m_pLabel->SetFont( hFont );

	m_pImage = nullptr;
	m_nType = NOTIFY_GENERIC;

	int x, y;
	surface()->GetScreenSize( x, y );

	fx = (float)x + 200.0f;
	fy = (float)y;
	VelX = -5.0f;
	VelY = 0.0f;
	StartTime = gpGlobals->curtime;
	Length = 5.0f;
	Progress = false;
	ProgressFrac = 0.0f;

	SetAlpha( 255 );
	SetVisible( true );
}

NoticePanel::~NoticePanel()
{
}

void NoticePanel::SetText( const char *text )
{
	if ( m_pLabel )
	{
		m_pLabel->SetText( text );
		m_pLabel->SizeToContents();
	}
	InvalidateLayout();
}

void NoticePanel::SetLegacyType( int t )
{
	m_nType = t;
	if ( !m_pImage )
	{
		m_pImage = new ImagePanel( this, "NoticeImage" );
		m_pImage->SetSize( 32, 32 );
		m_pImage->SetShouldScaleImage( true );
	}

	switch ( m_nType )
	{
		case NOTIFY_GENERIC:
		{
			m_pImage->SetImage( "notices/generic" );
			break;
		}
		case NOTIFY_ERROR:
		{
			m_pImage->SetImage( "notices/error" );
			break;
		}
		case NOTIFY_UNDO:
		{
			m_pImage->SetImage( "notices/undo" );
			break;
		}
		case NOTIFY_CLEANUP:
		{
			m_pImage->SetImage( "notices/cleanup" );
			break;
		}
		default:
		{
			m_pImage->SetImage( "notices/hint" );
			break;
		}
	}
	InvalidateLayout();
}

void NoticePanel::SetProgress( float frac )
{
	Progress = true;
	ProgressFrac = frac;
	InvalidateLayout();
}

void NoticePanel::Start( float length )
{
	StartTime = gpGlobals->curtime;
	Length = length;
}

bool NoticePanel::KillSelf()
{
	if ( Length < 0.0f )
		return false;
	if ( StartTime + Length < gpGlobals->curtime )
	{
		DeletePanel();
		return true;
	}
	return false;
}

void NoticePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pLabel )
	{
		m_pLabel->SetPos( 10, 6 );
		m_pLabel->SetSize( GetWide() - 20, GetTall() - 12 );
	}

	if ( m_pImage )
		m_pImage->SetPos( 4, ( GetTall() - 32 ) / 2 );
}

void NoticePanel::Paint()
{
	int w = GetWide(), h = GetTall();
	surface()->DrawSetColor( 20, 20, 20, (int)( 255 * 0.6f ) );
	surface()->DrawFilledRect( 0, 0, w, h );

	if ( Progress )
	{
		int boxX = 20;
		int boxY = GetTall() - 13;
		int boxW = GetWide() - 20;
		int boxH = 5;
		surface()->DrawSetColor( 0, 100, 0, 150 );
		surface()->DrawFilledRect( boxX, boxY, boxX + boxW, boxY + boxH );

		surface()->DrawSetColor( 0, 50, 0, 255 );
		surface()->DrawFilledRect( boxX + 1, boxY + 1, boxX + boxW - 2, boxY + boxH - 2 );

		int innerW = boxW - 2;
		int drawW = (int)ceilf( innerW * ProgressFrac );
		surface()->DrawSetColor( 0, 255, 0, 255 );
		surface()->DrawFilledRect( boxX + 1, boxY + 1, boxX + 1 + drawW, boxY + boxH - 1 );
	}
}

class CHudHints : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudHints, Panel );

public:
	CHudHints( const char *pElementName );
	virtual void Init() override;
	virtual void ApplySchemeSettings( IScheme *pScheme ) override;
	virtual void Think() override;
	virtual void Paint() override
	{ /* nothing to paint lol */
	}

	void AddHint( const char *name, float delay );
	void SuppressHint( const char *name );
	void AddNotify( const char *text, int type, float length );

	void SetMaxHints( int n )
	{
		m_nMaxHints = n;
	}

private:
	struct ScheduledHint
	{
		std::string name;
		float		fireTime;
	};

	std::vector< ScheduledHint >			m_vecScheduled;
	std::unordered_map< std::string, bool > m_ProcessedHints;
	std::vector< NoticePanel * >			m_Notices;

	int m_nMaxHints;
};

DECLARE_HUDELEMENT( CHudHints );

static CHudHints *g_pHudHints = nullptr;

CHudHints::CHudHints( const char *pElementName ) : CHudElement( pElementName ), Panel( NULL, "HudHints" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( 0 );
	SetPaintEnabled( false );
	SetPaintBackgroundEnabled( false );

	m_nMaxHints = 5;

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	g_pHudHints = this;
}

void CHudHints::Init()
{
	m_vecScheduled.clear();
	m_ProcessedHints.clear();
	m_Notices.clear();
}

void CHudHints::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

void CHudHints::Think()
{
	static float lastThinkTime = 0;
	static int	 thinkCount = 0;

	thinkCount++;

	float now = gpGlobals->curtime;
	for ( size_t i = 0; i < m_vecScheduled.size(); )
	{
		if ( m_vecScheduled[i].fireTime <= now )
		{
			char buf[512];
			snprintf( buf, sizeof( buf ), "Hint_%s", m_vecScheduled[i].name.c_str() );

			const wchar_t *w = g_pVGuiLocalize->Find( buf );
			std::string	   text;
			if ( w )
			{
				char out[512];
				g_pVGuiLocalize->ConvertUnicodeToANSI( w, out, sizeof( out ) );
				text = out;
			}
			else
			{
				text = m_vecScheduled[i].name;
			}

			AddNotify( text.c_str(), NOTIFY_HINT, 5.0f );

			i = m_vecScheduled.erase( m_vecScheduled.begin() + i ) - m_vecScheduled.begin();
		}
		else
		{
			++i;
		}
	}

	int x, y;
	surface()->GetScreenSize( x, y );

	int scrw = x;
	int scrh = y;

	float total_h = 0.0f;
	for ( size_t idx = 0; idx < m_Notices.size(); ++idx )
	{
		NoticePanel *p = m_Notices[idx];
		if ( !p )
			continue;

		float x = p->fx;
		float y = p->fy;
		int	  w = p->GetWide() + 16;
		int	  h = p->GetTall() + 4;

		float ideal_y = scrh - 150 - h - total_h;
		float ideal_x = scrw - w - 20;

		float timeleft = ( p->StartTime + p->Length ) - gpGlobals->curtime;
		if ( p->Length < 0 )
			timeleft = 1.0f;

		if ( timeleft < 0.7f )
			ideal_x -= 50;
		if ( timeleft < 0.2f )
			ideal_x += w * 2;

		float spd = gpGlobals->frametime * 15.0f;

		y = y + p->VelY * spd;
		x = x + p->VelX * spd;

		float dist = ideal_y - y;
		p->VelY = p->VelY + dist * spd * 1.0f;
		if ( fabs( dist ) < 2.0f && fabs( p->VelY ) < 0.1f )
			p->VelY = 0.0f;
		dist = ideal_x - x;
		p->VelX = p->VelX + dist * spd * 1.0f;
		if ( fabs( dist ) < 2.0f && fabs( p->VelX ) < 0.1f )
			p->VelX = 0.0f;

		p->VelX = p->VelX * ( 0.95f - gpGlobals->frametime * 8.0f );
		p->VelY = p->VelY * ( 0.95f - gpGlobals->frametime * 8.0f );

		p->fx = x;
		p->fy = y;

		if ( ideal_y > -scrh )
		{
			p->SetPos( (int)p->fx, (int)p->fy );
		}

		total_h += h;
	}

	for ( size_t i = 0; i < m_Notices.size(); )
	{
		if ( !m_Notices[i] || m_Notices[i]->KillSelf() )
		{
			m_Notices.erase( m_Notices.begin() + i );
		}
		else
			++i;
	}
}

void CHudHints::AddHint( const char *name, float delay )
{
	if ( m_ProcessedHints[name] )
	{
		return;
	}

	float fireTime = gpGlobals->curtime + delay;

	ScheduledHint s;
	s.name = name;
	s.fireTime = fireTime;
	m_vecScheduled.push_back( s );
	m_ProcessedHints[name] = true;
}

void CHudHints::SuppressHint( const char *name )
{
	for ( size_t i = 0; i < m_vecScheduled.size(); )
	{
		if ( m_vecScheduled[i].name == name )
			i = m_vecScheduled.erase( m_vecScheduled.begin() + i ) - m_vecScheduled.begin();
		else
			++i;
	}
}

void CHudHints::AddNotify( const char *text, int type, float length )
{
	if ( !cl_showhints.GetBool() )
		return;

	if ( m_Notices.size() >= (size_t)m_nMaxHints )
	{
		if ( !m_Notices.empty() )
		{
			m_Notices.front()->DeletePanel();
			m_Notices.erase( m_Notices.begin() );
		}
	}

	vgui::Panel *pParent = GetClientModeHL2MPNormal()->GetViewport();
	if ( !pParent )
	{
		return;
	}

	NoticePanel *p = new NoticePanel( pParent );
	if ( !p )
		return;

	p->SetText( text );
	p->SetLegacyType( type );
	p->Start( length );
	p->SetSize( 300, 36 );

	int x, y;
	surface()->GetScreenSize( x, y );
	//DevMsg("[HudHints] Screen size: %d x %d\n", x, y);

	p->fx = (float)( x + 200 );
	p->fy = (float)( y - 200 );

	p->SetPos( (int)p->fx, (int)p->fy );

	p->SetVisible( true );
	p->SetEnabled( true );
	p->SetMouseInputEnabled( false );
	p->SetKeyBoardInputEnabled( false );
	p->SetPaintEnabled( true );
	p->SetPaintBackgroundEnabled( true );

	p->MoveToFront();

	// hacky hack
	if ( RandomInt( 1, 2 ) == 1 )
		surface()->PlaySound( "ambient/water/drip1.wav" );
	else
		surface()->PlaySound( "ambient/water/drip2.wav" );

	m_Notices.push_back( p );
}

static int Hints_AddHint( lua_State *L )
{
	g_pHudHints->AddHint( luaL_checkstring( L, 1 ), luaL_checknumber( L, 2 ) );
	return 0;
}

static int Hints_AddNotify( lua_State *L )
{
	const char *text = luaL_checkstring( L, 1 );
	int			type = luaL_optinteger( L, 2, NOTIFY_GENERIC );
	float		length = luaL_optnumber( L, 3, 5.0f );
	g_pHudHints->AddNotify( text, type, length );
	return 0;
}

static const luaL_Reg hintlib[] = { { "AddHint", Hints_AddHint }, { "AddNotify", Hints_AddNotify }, { NULL, NULL } };

/*
** Open hintlib library
*/
LUALIB_API int luaopen_hintlib( lua_State *L )
{
	luaL_register( L, LUA_HINTLIBNAME, hintlib );
	return 1;
}

void CC_HintAdd( const CCommand &args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Usage: hint_add <text> [duration]\n" );
		return;
	}

	const char *text = args[1];
	float		duration = 5.0f;

	if ( args.ArgC() >= 3 )
		duration = atof( args[2] );

	if ( g_pHudHints )
		g_pHudHints->AddNotify( text, NOTIFY_GENERIC, duration );
}
ConCommand hint_add( "hint_add", CC_HintAdd, "Displays a hint on screen. Usage: hint_add <text> [duration]", FCVAR_CHEAT );
