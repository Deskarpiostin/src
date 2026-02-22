//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <ienginevgui.h>
#include <vgui_controls/Panel.h>
#include <GameUI/IGameUI.h>
#include <vgui_controls/Label.h>
#include "filesystem.h"
#include "tier1/KeyValues.h"

#include "basepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static CMainMenuSystem g_MySystem;
static CMainMenu	  *g_pMainMenu = nullptr;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule
//  over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

CHoverButton::CHoverButton( vgui::Panel *parent, const char *panelName ) : BaseClass( parent, panelName, "" )
{
	SetMouseInputEnabled( true );

	m_DefaultColor = Color( 200, 200, 200, 255 );
	m_HoverColor = Color( 255, 255, 0, 255 );

	m_DefaultFont = 0;
	m_BoldFont = 0;
}

void CHoverButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// default crap
	m_DefaultFont = pScheme->GetFont( "MainMenuFont", IsProportional() );
	m_BoldFont = pScheme->GetFont( "MainMenuFontBold", IsProportional() );

	SetFgColor( m_DefaultColor );
	SetFont( m_DefaultFont );
}

void CHoverButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	surface()->PlaySound( "ui/buttonrollover.wav" );

	SetCursor( vgui::dc_hand );

	SetFgColor( m_HoverColor );
	SetFont( m_BoldFont );
	SizeToContents();
	Repaint();
}

void CHoverButton::OnCursorExited()
{
	BaseClass::OnCursorExited();

	SetCursor( vgui::dc_arrow );

	SetFgColor( m_DefaultColor );
	SetFont( m_DefaultFont );
	SizeToContents();
	Repaint();
}

void CHoverButton::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	surface()->PlaySound( "ui/buttonclickrelease.wav" );

	if ( !m_Command.IsEmpty() )
		engine->ExecuteClientCmd( m_Command.String() );
}

void CHoverButton::SetCommand( const char *cmd )
{
	m_Command = cmd;
}

CMainMenuSystem::CMainMenuSystem() : CAutoGameSystem( "CMainMenuSystem" )
{
}

void CMainMenuSystem::PostInit()
{
	CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
	if ( !gameUIFactory )
		return;

	// and get the gameui object
	IGameUI *pGameUI = (IGameUI *)gameUIFactory( GAMEUI_INTERFACE_VERSION, NULL );
	if ( !pGameUI )
		return;

	VPANEL root = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	if ( !root )
		return;

	g_pMainMenu = new CMainMenu( root );
	if ( !g_pMainMenu )
		return;

	pGameUI->SetMainMenuOverride( g_pMainMenu->GetVPanel() );
}

void CMainMenuSystem::LevelInitPostEntity()
{
	g_pMainMenu->LoadGameMenu();
}

void CMainMenuSystem::LevelShutdownPostEntity()
{
	g_pMainMenu->LoadGameMenu();
}

CMainMenu::CMainMenu( VPANEL parent ) : Panel( NULL, "MainMenu" )
{
	SetParent( parent );

	SetVisible( true );
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	SetPaintBackgroundEnabled( false );

	SetProportional( true );

	m_pMenuBar = new CMenuBar( this );
	m_pMenuBar->SetBounds( 0, GetTall() - 50, GetWide(), 50 );

	// hack: hl2sbpp is default for sandbox mode so its fine
	m_pLogo = new ImageExtButton( this, "Logo", "materials/gamemode/sandbox.png" );
	m_pLogo->SetBounds( scheme()->GetProportionalScaledValue( 70 ), scheme()->GetProportionalScaledValue( 25 ), 256, 256 );

	LoadGameMenu();
}

void CMainMenu::LoadGameMenu()
{
	for ( int i = 0; i < m_GameMenuButtons.Count(); i++ )
	{
		if ( m_GameMenuButtons[i] )
		{
			m_GameMenuButtons[i]->MarkForDeletion();
		}
	}
	m_GameMenuButtons.RemoveAll();

	KeyValues *pKV = new KeyValues( "GameMenu" );

	if ( !pKV->LoadFromFile( g_pFullFileSystem, "resource/gamemenu.res", "GAME" ) )
	{
		Warning( "Failed to load gamemenu.res!\n" );
		pKV->deleteThis();
		return;
	}

	int y = scheme()->GetProportionalScaledValue( 165 );
	int x = scheme()->GetProportionalScaledValue( 75 );
	int spacing = scheme()->GetProportionalScaledValue( 25 );

	for ( KeyValues *pItem = pKV->GetFirstSubKey(); pItem; pItem = pItem->GetNextKey() )
	{
		const char *pszLabel = pItem->GetString( "label", "" );
		const char *pszCommand = pItem->GetString( "command", "" );

		if ( pszLabel[0] == '\0' && pszCommand[0] == '\0' )
		{
			y += scheme()->GetProportionalScaledValue( 25 );
			continue;
		}

		bool onlyInGame = pItem->GetInt( "OnlyInGame", 0 ) != 0;
		if ( onlyInGame && !engine->IsInGame() )
			continue;

		CUtlString finalCmd;

		if ( Q_strnicmp( pszCommand, "engine ", 7 ) == 0 )
		{
			const char *stripped = pszCommand + 7;
			finalCmd = stripped;
		}
		else
			finalCmd.Format( "gamemenucommand %s", pszCommand );

		CHoverButton *button = new CHoverButton( this, pszLabel );
		button->SetText( pszLabel );
		button->SetCommand( finalCmd );

		button->SetPos( x, y );
		//button->SetWide(250);
		//button->SetTall(20);
		button->InvalidateLayout( true, true );
		button->SizeToContents();

		m_GameMenuButtons.AddToTail( button );

		y += scheme()->GetProportionalScaledValue( 20 );
	}

	pKV->deleteThis();
}

void CMainMenu::PerformLayout()
{
	BaseClass::PerformLayout();

	// get the size...darn
	int wide, tall;
	GetSize( wide, tall );

	if ( m_pMenuBar )
		m_pMenuBar->SetBounds( 0, tall - 50, wide, 50 );
}

void CMainMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// Resize the panel to the screen size
	// Otherwise, it'll just be in a little corner
	int wide, tall;
	surface()->GetScreenSize( wide, tall );
	SetSize( wide, tall );
}
