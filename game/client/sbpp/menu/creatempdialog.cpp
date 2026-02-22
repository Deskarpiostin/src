//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "creatempdialog.h"
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Tooltip.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "sbpp/mapload_background.h"
#include "filesystem.h"
#include "tier1/utlvector.h"
#include <list>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar selmap( "selmap", "", FCVAR_DEVELOPMENTONLY );
ConVar mpdialog( "mpdialog", "0" );

CUtlVector< const char * > hardcodedMaps;
const char				  *hl2_maps[] = { "d1_trainstation_01", "d1_trainstation_02", "d1_trainstation_03", "d1_trainstation_04", "d1_trainstation_05", "d1_trainstation_06",

				   "d1_canals_01", "d1_canals_01a", "d1_canals_02", "d1_canals_03", "d1_canals_05", "d1_canals_06", "d1_canals_07", "d1_canals_08", "d1_canals_09", "d1_canals_10", "d1_canals_11", "d1_canals_12", "d1_canals_13",

				   "d1_eli_01", "d1_eli_02",

				   "d1_town_01", "d1_town_01a", "d1_town_02", "d1_town_03", "d1_town_02a", "d1_town_04", "d1_town_05",

				   "d2_coast_01", "d2_coast_03", "d2_coast_04", "d2_coast_05", "d2_coast_07", "d2_coast_08",

				   "d2_coast_09", "d2_coast_10", "d2_coast_11", "d2_coast_12", "d2_prison_01",

				   "d2_prison_02", "d2_prison_03", "d2_prison_04", "d2_prison_05",

				   "d2_prison_06", "d2_prison_07", "d2_prison_08", "d3_c17_01", "d3_c17_02", "d3_c17_03", "d3_c17_04", "d3_c17_05", "d3_c17_06", "d3_c17_07", "d3_c17_08", "d3_c17_09", "d3_c17_10a", "d3_c17_10b", "d3_c17_11", "d3_c17_12", "d3_c17_12b",
				   "d3_c17_13",

				   "d3_citadel_01", "d3_citadel_02", "d3_citadel_03", "d3_citadel_04", "d3_citadel_05",

				   "d3_breen_01" };

const char *hl2mp_maps[] = { "dm_lockdown", "dm_overwatch", "dm_powerhouse", "dm_resistance", "dm_runoff", "dm_steamlab", "dm_underpass", "halls3" };

const char *episodic_maps[] = { "ep1_citadel_00", "ep1_citadel_01", "ep1_citadel_02", "ep1_citadel_02b",

	"ep1_citadel_03", "ep1_citadel_04",

	"ep1_c17_00", "ep1_c17_00a",

	"ep1_c17_01", "ep1_c17_02", "ep1_c17_02b", "ep1_c17_02a",

	"ep1_c17_05", "ep1_c17_06" };

const char *ep2_maps[] = { "ep2_outland_01", "ep2_outland_01a",

	"ep2_outland_02", "ep2_outland_03", "ep2_outland_04",

	"ep2_outland_05", "ep2_outland_06",

	"ep2_outland_06a", "ep2_outland_07", "ep2_outland_08",

	"ep2_outland_09", "ep2_outland_10", "ep2_outland_10a",

	"ep2_outland_11", "ep2_outland_11a", "ep2_outland_11b",

	"ep2_outland_12", "ep2_outland_12a" };

const char *css_maps[] = { "de_dust", "de_dust2", "de_inferno", "de_nuke", "de_train", "de_aztec", "de_mirage", "de_prodigy", "de_chateau", "cs_office", "cs_assault", "cs_compound", "cs_havana", "cs_italy", "cs_militia" };

const char *portal_maps[] = { "testchmb_a_00", "testchmb_a_01", "testchmb_a_02", "testchmb_a_03", "testchmb_a_04", "testchmb_a_05", "testchmb_a_06", "testchmb_a_07", "testchmb_a_08", "testchmb_a_09", "testchmb_a_10", "testchmb_a_11",
	"testchmb_a_12", "testchmb_a_13", "testchmb_a_14", "testchmb_a_15", "testchmb_a_16", "testchmb_a_17", "testchmb_a_18", "testchmb_a_19", "escape_00", "escape_01", "escape_02" };

const char *hl1_maps[] = { "c0a0", "c0a0a", "c0a0b", "c0a0c", "c0a0d", "c0a0e",

	"c1a0", "c1a0d", "c1a0a", "c1a0b", "c1a0e",

	"c1a1a", "c1a1f", "c1a1b", "c1a1c", "c1a1d",

	"c1a2", "c1a2a", "c1a2b", "c1a2c", "c1a2d",

	"c1a3", "c1a3a", "c1a3b", "c1a3c", "c1a3d",

	"c1a4", "c1a4k", "c1a4b", "c1a4f", "c1a4d", "c1a4e", "c1a4i", "c1a4g", "c1a4j",

	"c2a1", "c2a1a", "c2a1b",

	"c2a2", "c2a2a", "c2a2b1", "c2a2b2", "c2a2c", "c2a2d", "c2a2e", "c2a2f", "c2a2g", "c2a2h",

	"c2a3", "c2a3a", "c2a3b", "c2a3c", "c2a3d", "c2a3e",

	"c2a4", "c2a4a", "c2a4b", "c2a4c",

	"c2a4d", "c2a4e", "c2a4f", "c2a4g",

	"c2a5", "c2a5w", "c2a5x", "c2a5a", "c2a5b", "c2a5c", "c2a5d", "c2a5e", "c2a5f", "c2a5g",

	"c3a1", "c3a1a", "c3a1b",

	"c3a2e", "c3a2", "c3a2a", "c3a2b", "c3a2c", "c3a2d", "c3a2f",

	"c4a1",

	"c4a2", "c4a2a", "c4a2b",

	"c4a1a", "c4a1b", "c4a1c", "c4a1d", "c4a1e", "c4a1f",

	"c4a3",

	"c5a1",

	"t0a0", "t0a0a", "t0a0b", "t0a0b1", "t0a0b2", "t0a0c", "t0a0d" };

extern ConVar hl2_mounted;
extern ConVar portal_mounted;
extern ConVar css_mounted;
extern ConVar hl1_mounted;
extern ConVar hl2mp_mounted;
extern ConVar episodic_mounted;
extern ConVar ep2_mounted;

GameMapsPanel::GameMapsPanel( vgui::Panel *parent, const char *pName ) : MapListPanel( parent, pName )
{
	SetBounds( 0, 0, 800, 640 );

	if ( hl2_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( hl2_maps ); i++ )
			hardcodedMaps.AddToTail( hl2_maps[i] );

	if ( css_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( css_maps ); i++ )
			hardcodedMaps.AddToTail( css_maps[i] );

	if ( portal_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( portal_maps ); i++ )
			hardcodedMaps.AddToTail( portal_maps[i] );

	if ( hl1_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( hl1_maps ); i++ )
			hardcodedMaps.AddToTail( hl1_maps[i] );

	if ( hl2mp_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( hl2mp_maps ); i++ )
			hardcodedMaps.AddToTail( hl2mp_maps[i] );

	if ( episodic_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( episodic_maps ); i++ )
			hardcodedMaps.AddToTail( episodic_maps[i] );

	if ( ep2_mounted.GetBool() )
		for ( int i = 0; i < ARRAYSIZE( ep2_maps ); i++ )
			hardcodedMaps.AddToTail( ep2_maps[i] );

	for ( int i = 0; i < hardcodedMaps.Count(); i++ )
	{
		char pngPath[260];
		Q_snprintf( pngPath, sizeof( pngPath ), "maps/thumb/%s.png", hardcodedMaps[i] );

		char command[128];
		Q_snprintf( command, sizeof( command ), "select %s", hardcodedMaps[i] );

		if ( g_pFullFileSystem->FileExists( pngPath ) )
		{
			AddButton( this, pngPath, command, hardcodedMaps[i] );
		}
		else
			AddButton( this, "materials/gui/noicon.png", command, hardcodedMaps[i] );
	}

	InvalidateLayout( true );
	MoveScrollBarToTop();

	PerformLayout();
}

ServerSettingsPanel::ServerSettingsPanel( vgui::Panel *parent, const char *pName ) : BaseClass( parent, pName )
{
	SetBounds( 0, 0, 800, 640 );

	const int labelWidth = 180;
	const int inputWidth = 200;
	const int rowHeight = 30;
	const int xLabel = 10;
	const int xInput = xLabel + labelWidth + 10;
	int		  y = 10;

	vgui::Label *lblMaxPlayers = new vgui::Label( this, "MaxPlayersLabel", "Max Players:" );
	lblMaxPlayers->SetPos( xLabel, y );
	lblMaxPlayers->SetSize( labelWidth, rowHeight );
	lblMaxPlayers->SetContentAlignment( vgui::Label::a_west );

	m_pMaxPlayers = new vgui::TextEntry( this, "MaxPlayersEntry" );
	m_pMaxPlayers->SetText( "1" );
	m_pMaxPlayers->SetSize( inputWidth, rowHeight );
	m_pMaxPlayers->SetPos( xInput, y );
	y += rowHeight + 15;

	vgui::Label *lblHostname = new vgui::Label( this, "HostnameLabel", "Hostname:" );
	lblHostname->SetPos( xLabel, y );
	lblHostname->SetSize( labelWidth, rowHeight );
	lblHostname->SetContentAlignment( vgui::Label::a_west );

	m_pHostname = new vgui::TextEntry( this, "HostnameEntry" );
	m_pHostname->SetText( "My HL2SB++ Server" );
	m_pHostname->SetSize( inputWidth, rowHeight );
	m_pHostname->SetPos( xInput, y );
	y += rowHeight + 15;

	vgui::Label *lblPassword = new vgui::Label( this, "PasswordLabel", "Password:" );
	lblPassword->SetPos( xLabel, y );
	lblPassword->SetSize( labelWidth, rowHeight );
	lblPassword->SetContentAlignment( vgui::Label::a_west );

	m_pPassword = new vgui::TextEntry( this, "PasswordEntry" );
	m_pPassword->SetText( "" ); // placeholder
	m_pPassword->SetSize( inputWidth, rowHeight );
	m_pPassword->SetPos( xInput, y );
	m_pPassword->SetTextHidden( true );

	y += rowHeight + 15;

	vgui::Label *lblGamemode = new vgui::Label( this, "GamemodeLabel", "Gamemode:" );
	lblGamemode->SetPos( xLabel, y );
	lblGamemode->SetSize( labelWidth, rowHeight );
	lblGamemode->SetContentAlignment( vgui::Label::a_west );

	m_pGamemodeCombo = new vgui::ComboBox( this, "GamemodeCombo", 6, false );
	m_pGamemodeCombo->SetSize( inputWidth, rowHeight );
	m_pGamemodeCombo->SetPos( xInput, y );
	y += rowHeight + 15;

	LoadGamemodes();
}

void ServerSettingsPanel::LoadGamemodes()
{
	if ( !m_pGamemodeCombo )
		return;

	int defaultIndex = m_pGamemodeCombo->AddItem( "Default", NULL );
	m_pGamemodeCombo->ActivateItem( defaultIndex );

	FileFindHandle_t findhandle;
	for ( const char *p = g_pFullFileSystem->FindFirstEx( "gamemodes/*", "MOD", &findhandle ); p && *p; p = g_pFullFileSystem->FindNext( findhandle ) )
	{
		if ( strchr( p, '.' ) )
			continue;

		m_pGamemodeCombo->AddItem( p, NULL );
	}
	g_pFullFileSystem->FindClose( findhandle );
}

void ServerSettingsPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( !IsVisible() )
		return;
}

void ServerSettingsPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void ServerSettingsPanel::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}

MapListPanel::MapListPanel( vgui::Panel *parent, const char *pName ) : BaseClass( parent, pName )
{
	SetBounds( 0, 0, 800, 640 );
	m_pSelectedButton = nullptr;
	m_bMapsLoaded = false;
}

void MapListPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( !IsVisible() )
		return;

	int c = layoutItems.Count();
	for ( int i = 0; i < c; i++ )
	{
		vgui::Panel *p = layoutItems[i];
		p->OnTick();
	}
}

void MapListPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int w = 127;
	int h = 127;
	int x = 5;
	int y = 5;
	int gap = 2;

	int c = layoutItems.Count();
	int wide = GetWide();

	for ( int i = 0; i < c; i++ )
	{
		vgui::Panel *p = layoutItems[i];
		p->SetBounds( x, y, w, h );

		x += ( w + gap );
		if ( x >= wide - w )
		{
			y += ( h + gap );
			x = 5;
		}
	}
}

class LabImageExtButton : public ImageExtButton
{
	DECLARE_CLASS_SIMPLE( LabImageExtButton, ImageExtButton );

public:
	LabImageExtButton( vgui::Panel *parent, const char *panelName, const char *normalImage, const char *mouseOverImage = nullptr, const char *mouseClickImage = nullptr, const char *pCmd = nullptr ) :
		ImageExtButton( parent, panelName, normalImage, mouseOverImage, mouseClickImage, pCmd ),
		m_bHovered( false ),
		m_bSelected( false ),
		m_overlayAlpha( 128 )
	{
		m_pLabel = new vgui::Label( this, "MapLabel", panelName );
		m_pLabel->SetContentAlignment( vgui::Label::a_center );
		m_pLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
		m_pLabel->SetBgColor( Color( 0, 0, 0, 0 ) );

		s_allButtons.push_back( this );
	}

	virtual ~LabImageExtButton()
	{
		s_allButtons.remove( this );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );
		m_pLabel->SetFont( pScheme->GetFont( "DefaultSmall", true ) );
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		int wide, tall;
		GetSize( wide, tall );
		int labelHeight = tall * 0.25f;
		m_pLabel->SetBounds( 0, tall - labelHeight, wide, labelHeight );
	}

	virtual void OnCursorEntered() OVERRIDE
	{
		BaseClass::OnCursorEntered();
		m_bHovered = true;
		Repaint();
	}

	virtual void OnCursorExited() OVERRIDE
	{
		BaseClass::OnCursorExited();
		m_bHovered = false;
		Repaint();
	}

	virtual void OnMouseReleased( vgui::MouseCode code ) OVERRIDE
	{
		BaseClass::OnMouseReleased( code );

		for ( auto *btn : s_allButtons )
		{
			if ( btn != this )
				btn->SetSelected( false );
		}

		// select self lol
		SetSelected( true );
		Repaint();
	}

	virtual void Paint() OVERRIDE
	{
		BaseClass::Paint();

		int wide, tall;
		GetSize( wide, tall );

		// overlay
		Color overlayColor( 0, 0, 0, m_overlayAlpha );
		vgui::surface()->DrawSetColor( overlayColor );
		int overlayHeight = tall * 0.25f;
		vgui::surface()->DrawFilledRect( 0, tall - overlayHeight, wide, tall );

		if ( m_bSelected )
		{
			Color selectedOverlay( 0, 255, 255, 64 ); // 25% im lazy pls send help
			vgui::surface()->DrawSetColor( selectedOverlay );
			vgui::surface()->DrawFilledRect( 0, 0, wide, tall );
		}

		if ( m_bHovered || m_bSelected )
		{
			Color borderColor = m_bSelected ? Color( 0, 255, 255, 255 ) : Color( 0, 255, 255, 128 );
			vgui::surface()->DrawSetColor( borderColor );
			vgui::surface()->DrawOutlinedRect( 0, 0, wide, tall );
		}
	}

	void SetSelected( bool state )
	{
		m_bSelected = state;
	}

	bool IsSelected() const
	{
		return m_bSelected;
	}

private:
	vgui::Label *m_pLabel;
	bool		 m_bHovered;
	bool		 m_bSelected;
	int			 m_overlayAlpha;

	static std::list< LabImageExtButton * > s_allButtons;
};

std::list< LabImageExtButton * > LabImageExtButton::s_allButtons;

void MapListPanel::AddButton( MapListPanel *panel, const char *image, const char *command, const char *mapName )
{
	LabImageExtButton *btn = new LabImageExtButton( panel, mapName, image, nullptr, nullptr, command );

	layoutItems.AddToTail( btn );
	panel->AddItem( NULL, btn );

	btn->SetFgColor( Color( 180, 180, 180, 255 ) );

	if ( BaseTooltip *pTooltip = btn->GetTooltip() )
		pTooltip->SetText( mapName );
}

void MapList::OnCancel()
{
	mpdialog.SetValue( 0 );
}

void MapList::OnClose()
{
	mpdialog.SetValue( 0 );
}

static const char *FilenameOnly( const char *path )
{
	if ( !path || !path[0] )
		return path;
	const char *p1 = strrchr( path, '/' );
	const char *p2 = strrchr( path, '\\' );
	const char *p = p1 > p2 ? p1 : p2;
	return p ? ( p + 1 ) : path;
}

void MapListPanel::LoadMaps( MapListPanel *panel )
{
	if ( m_bMapsLoaded )
		return;
	m_bMapsLoaded = true;

	layoutItems.RemoveAll();

	FileFindHandle_t mapHandle;
	for ( const char *pMap = g_pFullFileSystem->FindFirstEx( "maps/*.bsp", "MOD", &mapHandle ); pMap && *pMap; pMap = g_pFullFileSystem->FindNext( mapHandle ) )
	{
		char mapName[MAX_PATH];
		Q_FileBase( pMap, mapName, sizeof( mapName ) );

		char searchPattern[MAX_PATH];
		Q_snprintf( searchPattern, sizeof( searchPattern ), "maps/thumb/%s.*", mapName );

		FileFindHandle_t thumbHandle;
		char			 imageName[MAX_PATH] = "";

		for ( const char *foundFile = g_pFullFileSystem->FindFirstEx( searchPattern, "MOD", &thumbHandle ); foundFile && *foundFile; foundFile = g_pFullFileSystem->FindNext( thumbHandle ) )
		{
			char base[MAX_PATH];
			Q_FileBase( foundFile, base, sizeof( base ) );

			if ( !Q_stricmp( base, mapName ) ) // exact match
			{
				Q_snprintf( imageName, sizeof( imageName ), "maps/thumb/%s", foundFile );
				break;
			}
		}
		g_pFullFileSystem->FindClose( thumbHandle );

		if ( imageName[0] == '\0' )
		{
			Q_strncpy( imageName, "materials/gui/noicon.png", sizeof( imageName ) );
		}

		char imageCommand[MAX_PATH];
		Q_snprintf( imageCommand, sizeof( imageCommand ), "select %s", mapName );

		AddButton( panel, imageName, imageCommand, mapName );
	}

	g_pFullFileSystem->FindClose( mapHandle );

	panel->InvalidateLayout( true );
	panel->MoveScrollBarToTop();
}

void MapListPanel::OnCommand( const char *command )
{
	if ( Q_strnicmp( command, "select ", 7 ) == 0 )
	{
		const char *mapName = command + 7;
		char		mapNameNoExt[MAX_PATH];
		Q_strncpy( mapNameNoExt, mapName, sizeof( mapNameNoExt ) );
		Q_StripExtension( mapNameNoExt, mapNameNoExt, sizeof( mapNameNoExt ) );
		selmap.SetValue( mapNameNoExt );

		if ( m_pSelectedButton )
			m_pSelectedButton->SetFgColor( Color( 180, 180, 180, 255 ) );

		Msg( "Selected %s\n", mapNameNoExt );

		for ( int i = 0; i < layoutItems.Count(); i++ )
		{
			ImageExtButton *btn = dynamic_cast< ImageExtButton * >( layoutItems[i] );
			if ( !btn )
				continue;

			if ( Q_strcmp( btn->GetCommand(), command ) == 0 )
			{
				btn->SetFgColor( Color( 255, 255, 255, 255 ) );

				m_pSelectedButton = btn;
				break;
			}
		}
	}
}

MapList::MapList( vgui::VPANEL *parent, const char *pName ) : BaseClass( NULL, "MapList" )
{
	int iWide, iTall;
	surface()->GetScreenSize( iWide, iTall );

	SetSize( iWide / 1.25, iTall / 1.25 );
	SetTitle( "New Game", true );

	int screenWide, screenTall;
	vgui::surface()->GetScreenSize( screenWide, screenTall );
	SetPos( ( screenWide - GetWide() ) / 2, ( screenTall - GetTall() ) / 2 );

	MapListPanel		*maplist = new MapListPanel( this, NULL );
	ServerSettingsPanel *info = new ServerSettingsPanel( this, NULL );
	GameMapsPanel		*gamemaps = new GameMapsPanel( this, NULL );
	maplist->LoadMaps( maplist );
	AddPage( maplist, "Maps" );
	AddPage( gamemaps, "Game Maps" );
	AddPage( info, "Server Settings" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	GetPropertySheet()->SetTabWidth( 72 );
	SetMoveable( true );
	SetVisible( true );
	SetSizeable( true );
	SetProportional( true );
	SetApplyButtonVisible( false );
}

bool MapList::OnOK( bool applyOnly )
{
	const char *mapName = selmap.GetString();
	if ( mapName && mapName[0] != '\0' )
	{
		int			maxPlayers = 16;
		const char *hostname = "My Server";
		const char *password = "";

		ServerSettingsPanel *infoPanel = dynamic_cast< ServerSettingsPanel * >( GetPropertySheet()->GetPage( 2 ) );

		char maxPlayersBuffer[2048];
		infoPanel->m_pMaxPlayers->GetText( maxPlayersBuffer, sizeof( maxPlayersBuffer ) );
		maxPlayers = atoi( maxPlayersBuffer );

		char hostnameBuffer[2048];
		infoPanel->m_pHostname->GetText( hostnameBuffer, sizeof( hostnameBuffer ) );

		char passwordBuffer[2048];
		infoPanel->m_pPassword->GetText( passwordBuffer, sizeof( passwordBuffer ) );

		char gamemodeBuffer[2048] = "";
		if ( infoPanel->m_pGamemodeCombo )
			infoPanel->m_pGamemodeCombo->GetText( gamemodeBuffer, sizeof( gamemodeBuffer ) );

		char szMapCommand[2048];
		if ( gamemodeBuffer[0] != '\0' && Q_strcmp( gamemodeBuffer, "Default" ) != 0 )
		{
			Q_snprintf( szMapCommand, sizeof( szMapCommand ), "disconnect\nwait\nwait\nsv_lan 1\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\ngamemode \"%s\"\nmap %s\n", maxPlayers, password, hostnameBuffer, gamemodeBuffer,
				selmap.GetString() );
		}
		else
		{
			Q_snprintf( szMapCommand, sizeof( szMapCommand ), "disconnect\nwait\nwait\nsv_lan 1\nmaxplayers %i\nsv_password \"%s\"\nhostname \"%s\"\nprogress_enable\nmap %s\ngamemode sandbox\n", maxPlayers, password, hostnameBuffer,
				selmap.GetString() );
		}

		engine->ClientCmd_Unrestricted( szMapCommand );
	}

	mpdialog.SetValue( "0" ); // cannibalism

	return true;
}

static bool isfront = false;

void MapList::OnTick()
{
	BaseClass::OnTick();

	if ( !mpdialog.GetBool() )
		isfront = false;

	SetVisible( mpdialog.GetBool() );
	if ( IsVisible() && isfront == false )
	{
		// LOL!
		MoveToFront();
		RequestFocus();
		isfront = true;
	}
}

int MapListPanel::ComputeVPixelsNeeded()
{
	int count = layoutItems.Count();
	if ( count == 0 )
		return 0;

	const int tileW = 127;
	const int tileH = 127;
	const int gap = 2;
	const int leftPadding = 5;
	int		  wide = GetWide();

	if ( wide <= tileW )
		wide = tileW + 1;

	int cols = ( wide - leftPadding ) / ( tileW + gap );
	if ( cols < 1 )
		cols = 1;

	int rows = ( count + cols - 1 ) / cols;

	int total = rows * ( tileH + gap ) + leftPadding;
	return total;
}

class MapListInterface : public CMapList
{
private:
	MapList *CMapList;

public:
	MapListInterface()
	{
		CMapList = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		CMapList = new MapList( &parent, NULL );
	}
	void Destroy()
	{
		if ( CMapList )
		{
			CMapList->SetParent( (vgui::Panel *)NULL );
			delete CMapList;
		}
	}
	void Activate( void )
	{
		if ( CMapList )
		{
			CMapList->Activate();
		}
	}
};
static MapListInterface g_MapList;
CMapList			   *maplist = (CMapList *)&g_MapList;