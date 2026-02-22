//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "workshop.h"
// Oops!
#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#include "curl/curl.h"
#include "filesystem.h"
#include "menu/imageextbutton.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ScrollBar.h>
#include "vgui/IInput.h"
#include "rapidjson/document.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/MessageBox.h"
#ifdef _WIN32
#undef MessageBox
#undef PostMessage
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static WorkshopClient  *g_pWorkshopClient = nullptr;
static CWorkshopDialog *g_pWorkshopDialog = nullptr;

static size_t WriteCallback( void *contents, size_t size, size_t nmemb, void *userp )
{
	CUtlString *str = (CUtlString *)userp;
	str->Append( (char *)contents, size * nmemb );
	return size * nmemb;
}

static size_t FileWriteCallback( void *contents, size_t size, size_t nmemb, void *userp )
{
	FileHandle_t *pFile = (FileHandle_t *)userp;
	if ( *pFile == FILESYSTEM_INVALID_HANDLE )
		return 0;

	return filesystem->Write( contents, size * nmemb, *pFile );
}

WorkshopClient::WorkshopClient( const char *downloadFolder ) : m_downloadFolder( downloadFolder )
{
	curl_global_init( CURL_GLOBAL_DEFAULT );

	filesystem->CreateDirHierarchy( m_downloadFolder.String(), "MOD" );

	m_tempFolder = "downloads/temp";
	filesystem->CreateDirHierarchy( m_tempFolder.String(), "MOD" );
}

WorkshopClient::~WorkshopClient()
{
	curl_global_cleanup();
}

CUtlString WorkshopClient::GetModDownloadUrlForMod( int modId )
{
	CUtlString url;
	url.Format( "https://raw.githubusercontent.com/hl2sbpp/Workshop/main/mods.json" );

	CUtlString response;
	CURL	  *curl = curl_easy_init();
	if ( !curl )
		return CUtlString();

	curl_easy_setopt( curl, CURLOPT_URL, url.String() );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response );
	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2L );
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 30L );
	curl_easy_setopt( curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" );
	char absPath[MAX_PATH];
	filesystem->RelativePathToFullPath( "settings/cacert.pem", "GAME", absPath, sizeof( absPath ) );
	curl_easy_setopt( curl, CURLOPT_CAINFO, absPath );

	struct curl_slist *headers = nullptr;
	headers = curl_slist_append( headers, "Accept: application/json" );
	curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

	CURLcode res = curl_easy_perform( curl );

	long response_code = 0;
	curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );

	curl_slist_free_all( headers );
	curl_easy_cleanup( curl );

	if ( res != CURLE_OK || response_code != 200 )
	{
		Warning( "GitHub: failed to fetch modfile list for mod %d: %s (http %ld)\n", modId, curl_easy_strerror( res ), response_code );
		return CUtlString();
	}

	rapidjson::Document doc;
	if ( doc.Parse( response.String() ).HasParseError() )
	{
		Warning( "GitHub: JSON parse error\n" );
		return CUtlString();
	}

	if ( !doc.HasMember( "mods" ) || !doc["mods"].IsArray() )
	{
		Warning( "GitHub: 'mods' array missing\n" );
		return CUtlString();
	}

	const rapidjson::Value &mods = doc["mods"];
	for ( rapidjson::SizeType i = 0; i < mods.Size(); i++ )
	{
		const rapidjson::Value &modJson = mods[i];
		if ( !modJson.HasMember( "id" ) || !modJson["id"].IsInt() )
			continue;

		if ( modJson["id"].GetInt() == modId )
		{
			if ( modJson.HasMember( "download_url" ) && modJson["download_url"].IsString() )
			{
				return CUtlString( modJson["download_url"].GetString() );
			}
		}
	}

	return CUtlString();
}

bool WorkshopClient::DownloadUrlToFile( const char *url, const char *outPath )
{
	if ( !url || !outPath )
		return false;

	CURL *curl = curl_easy_init();
	if ( !curl )
		return false;

	FileHandle_t fp = filesystem->Open( outPath, "wb", "MOD" );
	if ( fp == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "GitHub: Failed to open file for writing: %s\n", outPath );
		curl_easy_cleanup( curl );
		return false;
	}

	curl_easy_setopt( curl, CURLOPT_URL, url );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, FileWriteCallback );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, &fp );
	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2L );
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 30L );
	curl_easy_setopt( curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" );

	CURLcode res = curl_easy_perform( curl );

	curl_easy_cleanup( curl );
	filesystem->Close( fp );

	if ( res != CURLE_OK )
	{
		Warning( "GitHub: CURL download error: %s\n", curl_easy_strerror( res ) );
		filesystem->RemoveFile( outPath, "MOD" );
		return false;
	}

	return true;
}

bool WorkshopClient::ParseModsJSON( const char *jsonText, CUtlVector< Mod > &outMods )
{
	rapidjson::Document doc;
	if ( doc.Parse( jsonText ).HasParseError() )
	{
		Warning( "GitHub: Failed to parse JSON\n" );
		return false;
	}

	if ( !doc.HasMember( "mods" ) || !doc["mods"].IsArray() )
	{
		Warning( "GitHub: 'mods' array missing\n" );
		return false;
	}

	const rapidjson::Value &modsArray = doc["mods"];
	for ( rapidjson::SizeType i = 0; i < modsArray.Size(); i++ )
	{
		const rapidjson::Value &modJson = modsArray[i];

		Mod mod;

		if ( modJson.HasMember( "id" ) && modJson["id"].IsInt() )
			mod.id = modJson["id"].GetInt();

		if ( modJson.HasMember( "name" ) && modJson["name"].IsString() )
			mod.name = modJson["name"].GetString();

		if ( modJson.HasMember( "description" ) && modJson["description"].IsString() )
			mod.description = modJson["description"].GetString();

		if ( modJson.HasMember( "author" ) && modJson["author"].IsString() )
			mod.author = modJson["author"].GetString();

		if ( modJson.HasMember( "version" ) && modJson["version"].IsString() )
			mod.version = modJson["version"].GetString();

		if ( modJson.HasMember( "download_url" ) && modJson["download_url"].IsString() )
			mod.downloadUrl = modJson["download_url"].GetString();

		if ( modJson.HasMember( "preview" ) && modJson["preview"].IsString() )
			mod.logoUrl = modJson["preview"].GetString();

		if ( modJson.HasMember( "size_mb" ) && modJson["size_mb"].IsInt() )
			mod.fileSize = static_cast< long long >( modJson["size_mb"].GetInt() ) * 1024 * 1024;

		if ( mod.id != 0 && !mod.name.IsEmpty() )
			outMods.AddToTail( mod );
	}

	DevMsg( "GitHub: Parsed %d mods\n", outMods.Count() );
	return outMods.Count() > 0;
}

// -----------------------
// Purpose:
// -----------------------
void WorkshopClient::FetchMods( ModListCallback cb, void *user )
{
	CUtlString url = "https://raw.githubusercontent.com/hl2sbpp/Workshop/main/mods.json";
	CUtlString response;

	CURL *curl = curl_easy_init();
	if ( !curl )
	{
		Warning( "GitHub: Failed to initialize CURL\n" );
		CUtlVector< Mod > emptyMods;
		cb( emptyMods, user );
		return;
	}

	curl_easy_setopt( curl, CURLOPT_URL, url.String() );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, WriteCallback );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response );
	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2L );
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 30L );
	curl_easy_setopt( curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" );
	char absPath[MAX_PATH];
	filesystem->RelativePathToFullPath( "settings/cacert.pem", "GAME", absPath, sizeof( absPath ) );
	curl_easy_setopt( curl, CURLOPT_CAINFO, absPath );

	struct curl_slist *headers = nullptr;
	headers = curl_slist_append( headers, "Accept: application/json" );
	curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );

	DevMsg( "GitHub: Fetching mods...\n" );

	CURLcode res = curl_easy_perform( curl );

	long response_code = 0;
	curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );

	curl_slist_free_all( headers );
	curl_easy_cleanup( curl );

	if ( res != CURLE_OK || response_code != 200 )
	{
		Warning( "GitHub: Failed to fetch mods: %s (HTTP %ld)\n", curl_easy_strerror( res ), response_code );
		CUtlVector< Mod > emptyMods;
		cb( emptyMods, user );
		return;
	}

	m_cachedMods.RemoveAll();
	if ( !ParseModsJSON( response.String(), m_cachedMods ) )
	{
		CUtlVector< Mod > emptyMods;
		cb( emptyMods, user );
		return;
	}

	for ( int i = 0; i < m_cachedMods.Count(); ++i )
	{
		Mod &mod = m_cachedMods[i];
		if ( !mod.logoUrl.IsEmpty() )
		{
			CUtlString url = mod.logoUrl;

			const char *query = V_strrchr( url.String(), '?' );
			if ( query )
			{
				url.SetLength( query - url.String() );
			}

			const char *ext = V_strrchr( url.String(), '.' );
			CUtlString	outPath;

			if ( ext && V_strlen( ext ) <= 5 )
				outPath.Format( "%s/%d%s", m_tempFolder.String(), mod.id, ext );
			else
				outPath.Format( "%s/%d.png", m_tempFolder.String(), mod.id );

			if ( !g_pFullFileSystem->FileExists( outPath.String() ) )
			{
				if ( !DownloadUrlToFile( mod.logoUrl.String(), outPath.String() ) )
				{
					Warning( "GitHub: Failed to download thumbnail for mod %d\n", mod.id );
				}
			}

			mod.localLogoPath = outPath;
		}
	}

	cb( m_cachedMods, user );
}

void WorkshopClient::GetInstalledModIDs( CUtlVector< int > &outIDs )
{
	outIDs.RemoveAll();

	char		searchPattern[512];
	const char *folder = m_downloadFolder.String();
	int			folderLen = V_strlen( folder );
	if ( folderLen > 0 && folder[folderLen - 1] == '/' )
	{
		V_snprintf( searchPattern, sizeof( searchPattern ), "%s*.zip", folder );
	}
	else
	{
		V_snprintf( searchPattern, sizeof( searchPattern ), "%s/*.zip", folder );
	}

	FileFindHandle_t findHandle = 0;
	const char		*found = g_pFullFileSystem->FindFirst( searchPattern, &findHandle );

	while ( found )
	{
		char digits[32];
		int	 di = 0;
		for ( int i = 0; found[i] && di < (int)sizeof( digits ) - 1; ++i )
		{
			if ( found[i] >= '0' && found[i] <= '9' )
				digits[di++] = found[i];
			else
				break;
		}

		if ( di > 0 )
		{
			digits[di] = '\0';
			int id = atoi( digits );
			if ( id > 0 && outIDs.Find( id ) == outIDs.InvalidIndex() )
			{
				outIDs.AddToTail( id );
			}
		}

		found = g_pFullFileSystem->FindNext( findHandle );
	}

	if ( findHandle != 0 )
	{
		g_pFullFileSystem->FindClose( findHandle );
	}
}

bool WorkshopClient::DownloadMod( Mod *mod, ModActionCallback cb, void *user )
{
	if ( !mod || mod->downloadUrl.IsEmpty() )
	{
		Warning( "GitHub: Invalid mod or download URL\n" );
		if ( cb )
			cb( false, "Invalid mod or download URL" );
		return false;
	}

	CUtlString	freshUrl = GetModDownloadUrlForMod( mod->id );
	const char *downloadUrlToUse = nullptr;
	if ( !freshUrl.IsEmpty() )
		downloadUrlToUse = freshUrl.String();
	else
		downloadUrlToUse = mod->downloadUrl.String();

	CURL *curl = curl_easy_init();
	if ( !curl )
	{
		Warning( "GitHub: Failed to initialize CURL\n" );
		if ( cb )
			cb( false, "Failed to initialize CURL" );
		return false;
	}

	CUtlString outputPath;
	outputPath.Format( "%s/%d.zip", m_downloadFolder.String(), mod->id );
	DevMsg( "GitHub: Downloading mod %d to %s\n", mod->id, outputPath.String() );

	FileHandle_t fp = filesystem->Open( outputPath.String(), "wb", "MOD" );
	if ( fp == FILESYSTEM_INVALID_HANDLE )
	{
		Warning( "GitHub: Failed to open file for writing: %s\n", outputPath.String() );
		curl_easy_cleanup( curl );
		if ( cb )
			cb( false, "Failed to open file for writing" );
		return false;
	}

	curl_easy_setopt( curl, CURLOPT_URL, downloadUrlToUse );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, FileWriteCallback );
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, &fp );
	curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 1L );
	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2L );
	curl_easy_setopt( curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" );
	curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 1L );

	//curl_easy_setopt( curl, CURLOPT_USERAGENT, "Sbpp-WorkshopClient/1.0" );

	curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT, 30L );
	curl_easy_setopt( curl, CURLOPT_TIMEOUT, 300L );

	curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2L );
	curl_easy_setopt( curl, CURLOPT_VERBOSE, 1L );
	char absPath[MAX_PATH];
	filesystem->RelativePathToFullPath( "settings/cacert.pem", "GAME", absPath, sizeof( absPath ) );
	curl_easy_setopt( curl, CURLOPT_CAINFO, absPath );

	CURLcode res = curl_easy_perform( curl );
	curl_easy_cleanup( curl );
	filesystem->Close( fp );

	if ( res != CURLE_OK )
	{
		Warning( "GitHub: CURL download error: %s (URL: %s)\n", curl_easy_strerror( res ), mod->downloadUrl.String() );
		filesystem->RemoveFile( outputPath.String(), "MOD" );
		if ( cb )
			cb( false, curl_easy_strerror( res ) );
		return false;
	}

	DevMsg( "GitHub: Successfully downloaded mod %d\n", mod->id );
	if ( cb )
		cb( true, nullptr );
	return true;
}

WorkshopClient *GetWorkshopClient()
{
	return g_pWorkshopClient;
}

void InitWorkshopClient( const char *downloadFolder )
{
	if ( g_pWorkshopClient )
	{
		Warning( "GitHub: Client already initialized\n" );
		return;
	}

	g_pWorkshopClient = new WorkshopClient( downloadFolder );
}

void ShutdownWorkshopClient()
{
	if ( g_pWorkshopClient )
	{
		delete g_pWorkshopClient;
		g_pWorkshopClient = nullptr;
		DevMsg( "GitHub: Client shutdown\n" );
	}
}

CModThumbnailPanel::CModThumbnailPanel( Panel *parent, const char *panelName, const Mod &mod, bool allowSubscribe ) :
	BaseClass( parent, panelName ),
	m_bSubscribed( false ),
	m_allowSubscribe( allowSubscribe ),
	m_modId( mod.id ),
	m_cachedMod( mod ),
	m_pDownloadedImage( nullptr ),
	m_bHovered( false ),
	m_pImageContainer( nullptr ),
	m_pCheckbox( nullptr ),
	m_pNameLabel( nullptr ),
	m_pSizeLabel( nullptr )
{
	const int padding = 8;
	const int imageW = 200;
	const int imageH = 112;
	const int checkboxSize = 32;
	const int infoHeight = 65;
	const int panelW = imageW + padding * 2;
	const int panelH = imageH + padding * 2 + 50;

	SetMouseInputEnabled( true );
	SetSize( panelW, panelH );
	SetPaintBackgroundEnabled( true );

	const char *thumbPath = mod.localLogoPath.IsEmpty() ? "" : mod.localLogoPath.String();

	m_pImageContainer = new Panel( this, "ImageContainer" );
	m_pImageContainer->SetPos( padding, padding );
	m_pImageContainer->SetSize( imageW, imageH );
	m_pImageContainer->SetMouseInputEnabled( false );

	ImageExtButton *imgBtn = new ImageExtButton( m_pImageContainer, "ModImageBtn", thumbPath, nullptr, nullptr, nullptr );
	imgBtn->SetPos( 0, 0 );
	imgBtn->SetSize( imageW, imageH );
	imgBtn->SetScaleImage( true );
	imgBtn->SetMouseInputEnabled( false );

	m_pCheckbox = new CheckButton( this, "SubscribeCheck", "" );
	m_pCheckbox->SetSize( checkboxSize, checkboxSize );
	m_pCheckbox->SetPos( panelW - padding - checkboxSize, imageH + padding + infoHeight - checkboxSize - padding );
	m_pCheckbox->SetMouseInputEnabled( m_allowSubscribe );
	m_pCheckbox->SetVisible( m_allowSubscribe );
	m_pCheckbox->AddActionSignalTarget( this );
	m_pCheckbox->SetText( "" );
	m_pCheckbox->SetPaintBackgroundEnabled( false );

	m_pNameLabel = new Label( this, "ModName", mod.name.String() );
	m_pNameLabel->SetPos( padding, imageH + padding + 8 );
	m_pNameLabel->SetSize( panelW - padding * 2, 16 );
	m_pNameLabel->SetMouseInputEnabled( false );

	char sizeStr[64];
	V_snprintf( sizeStr, sizeof( sizeStr ), "%.1f MB", mod.fileSize / ( 1024.0f * 1024.0f ) );
	m_pSizeLabel = new Label( this, "ModSize", sizeStr );
	m_pSizeLabel->SetPos( padding, imageH + padding + 30 );
	m_pSizeLabel->SetSize( panelW - padding * 2, 16 );
	m_pSizeLabel->SetMouseInputEnabled( false );
}

CModThumbnailPanel::~CModThumbnailPanel()
{
	if ( m_pDownloadedImage )
	{
		delete m_pDownloadedImage;
		m_pDownloadedImage = nullptr;
	}
}

void CModThumbnailPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBorder( pScheme->GetBorder( "ButtonBorder" ) );
	SetBgColor( pScheme->GetColor( "ListPanel.BgColor", Color( 40, 40, 40, 255 ) ) );

	if ( m_pNameLabel )
		m_pNameLabel->SetFgColor( pScheme->GetColor( "Label.TextColor", Color( 255, 255, 255, 255 ) ) );
	if ( m_pSizeLabel )
		m_pSizeLabel->SetFgColor( pScheme->GetColor( "Label.DisabledTextColor", Color( 150, 150, 150, 255 ) ) );
}

void CModThumbnailPanel::PaintBackground()
{
	BaseClass::PaintBackground();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	int mx, my;
	input()->GetCursorPos( mx, my );
	ScreenToLocal( mx, my );

	m_bHovered = ( mx >= 0 && mx < w && my >= 0 && my < h );

	if ( m_bHovered )
	{
		surface()->DrawSetColor( Color( 100, 150, 255, 50 ) );
		surface()->DrawFilledRect( 0, 0, w, h );
	}
}

void CModThumbnailPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CModThumbnailPanel::SetSubscribed( bool subscribed )
{
	if ( m_bSubscribed == subscribed )
		return;

	m_bSubscribed = subscribed;

	if ( m_pCheckbox && m_pCheckbox->IsSelected() != subscribed )
		m_pCheckbox->SetSelected( subscribed );

	CWorkshopDialog *pDialog = GetWorkshopDialog();
	if ( pDialog )
		pDialog->PostMessage( pDialog, new KeyValues( "ApplyButtonEnable" ) );
}

void CModThumbnailPanel::SetImage( IImage *pImage )
{
}

void CModThumbnailPanel::OnMousePressed( MouseCode code )
{
	if ( code != MOUSE_LEFT )
		return;

	int mx, my;
	input()->GetCursorPos( mx, my );
	ScreenToLocal( mx, my );

	int cbX, cbY, cbW, cbH;
	m_pCheckbox->GetBounds( cbX, cbY, cbW, cbH );

	if ( mx >= cbX && mx < cbX + cbW && my >= cbY && my < cbY + cbH && m_allowSubscribe )
		return;

	ShowModDetails();
}

void CModThumbnailPanel::ShowModDetails()
{
	CUtlString detailsText;
	detailsText.Format( "Name: %s\n\nDescription:\n%s\n\n", m_cachedMod.name.String(), m_cachedMod.description.String() );

	detailsText.Append( CUtlString().Format( "Size: %.1f MB\n", m_cachedMod.fileSize / ( 1024.0f * 1024.0f ) ) );

	if ( !m_cachedMod.version.IsEmpty() )
	{
		detailsText.Append( "Version: " );
		detailsText.Append( m_cachedMod.version.String() );
	}

	if ( !m_cachedMod.author.IsEmpty() )
	{
		detailsText.Append( "By: " );
		detailsText.Append( m_cachedMod.author.String() );
	}

	MessageBox *pMsg = new MessageBox( m_cachedMod.name.String(), detailsText.String(), this );
	pMsg->DoModal();
}

void CModThumbnailPanel::OnCheckButtonChecked( int state )
{
	if ( m_allowSubscribe )
	{
		bool isChecked = m_pCheckbox->IsSelected();
		SetSubscribed( isChecked );
	}
}

static void CSubscribedPage_FetchCallback( CUtlVector< Mod > &mods, void *user );

CBrowsePage::CBrowsePage( Panel *parent, const char *panelName ) : PropertyPage( parent, panelName )
{
	SetSize( 800, 600 );

	vgui::EditablePanel *child = new vgui::EditablePanel( this, "ModGridCanvas" );
	child->SetSize( 800, 2000 );
	child->SetPaintBackgroundEnabled( false );

	m_pModGrid = new vgui::ScrollableEditablePanel( this, child, "ModGrid" );
	m_pModGrid->SetBounds( 0, 0, GetWide(), GetTall() );
	m_pModGrid->SetPaintBackgroundEnabled( false );
	m_pModGrid->GetScrollbar()->SetWide( 16 );
	m_pModGrid->GetScrollbar()->SetVisible( true );
}

CBrowsePage::~CBrowsePage()
{
}

void CBrowsePage::OnPageShow()
{
	PropertyPage::OnPageShow();
	RefreshModList();
}

void CBrowsePage::RefreshModList()
{
	WorkshopClient *pClient = GetWorkshopClient();
	if ( !pClient )
	{
		Warning( "GitHub: Workshop opened but client not initialized!\n" );
		return;
	}

	for ( int i = 0; i < m_ModPanels.Count(); i++ )
	{
		m_ModPanels[i]->MarkForDeletion();
	}
	m_ModPanels.RemoveAll();

	pClient->FetchMods( ModsReceivedCallback, this );
}

void CBrowsePage::ModsReceivedCallback( CUtlVector< Mod > &mods, void *user )
{
	CBrowsePage *pPage = (CBrowsePage *)user;
	if ( pPage )
	{
		pPage->OnModsReceived( mods );
	}
}

void CBrowsePage::OnModsReceived( CUtlVector< Mod > &mods )
{
	PopulateModList();
}

void CBrowsePage::OnRefreshClicked()
{
	RefreshModList();
}

void CBrowsePage::PopulateModList()
{
	WorkshopClient *pClient = GetWorkshopClient();
	if ( !pClient )
		return;

	for ( int i = 0; i < m_ModPanels.Count(); i++ )
	{
		if ( m_ModPanels[i] )
		{
			m_ModPanels[i]->MarkForDeletion();
		}
	}
	m_ModPanels.RemoveAll();

	const CUtlVector< Mod > &mods = pClient->GetCachedMods();

	CUtlVector< int > installedIDs;
	pClient->GetInstalledModIDs( installedIDs );

	int columns = 3;
	int panelWidth = 216;
	int panelHeight = 182;
	int spacingX = 15;
	int spacingY = 15;
	int startX = 10;
	int startY = 10;

	int rows = ( mods.Count() + columns - 1 ) / columns;
	int requiredHeight = startY + rows * ( panelHeight + spacingY ) + 50;

	Panel *pCanvas = m_pModGrid->GetChild( 0 );
	if ( pCanvas )
	{
		pCanvas->SetSize( GetWide() - 20, requiredHeight );
		
		for ( int i = 0; i < mods.Count(); i++ )
		{
			int col = i % columns;
			int row = i / columns;

			CModThumbnailPanel *pPanel = new CModThumbnailPanel( pCanvas, "ModPanel", mods[i], true );
			pPanel->SetPos( startX + col * ( panelWidth + spacingX ), startY + row * ( panelHeight + spacingY ) );
			pPanel->SetVisible( true );
			pPanel->SetZPos( 10 );

			bool isInstalled = installedIDs.Find( mods[i].id ) != installedIDs.InvalidIndex();
			pPanel->SetSubscribed( isInstalled );

			m_ModPanels.AddToTail( pPanel );
		}
	}

	m_pModGrid->InvalidateLayout( true, true );
}

void CBrowsePage::OnApplyChanges()
{
	WorkshopClient *pClient = GetWorkshopClient();
	if ( !pClient )
		return;

	CUtlVector< int > installedIDs;
	pClient->GetInstalledModIDs( installedIDs );

	const CUtlVector< Mod > &allMods = pClient->GetCachedMods();

	int pendingDownloads = 0;
	int pendingUnsubscribes = 0;

	for ( int i = 0; i < m_ModPanels.Count(); i++ )
	{
		CModThumbnailPanel *pPanel = m_ModPanels[i];
		int					modId = pPanel->GetModID();
		bool				shouldBeInstalled = pPanel->IsSubscribed();
		bool				currentlyInstalled = installedIDs.Find( modId ) != installedIDs.InvalidIndex();

		if ( shouldBeInstalled && !currentlyInstalled )
		{
			for ( int j = 0; j < allMods.Count(); j++ )
			{
				if ( allMods[j].id == modId )
				{
					pendingDownloads++;

					Mod *pMod = const_cast< Mod * >( &allMods[j] );

					pClient->DownloadMod(
						pMod,
						[]( bool success, const char *err )
						{
							if ( !success )
								Warning( "GitHub: Download failed: %s\n", err ? err : "unknown" );
							else
								DevMsg( "GitHub: Mod downloaded successfully\n" );

							CWorkshopDialog *pDialog = GetWorkshopDialog();
							if ( pDialog )
							{
								CBrowsePage *pPage = dynamic_cast< CBrowsePage * >( pDialog->GetActivePage() );
								if ( pPage )
								{
									pPage->PopulateModList();
								}
							}
						},
						nullptr );

					break;
				}
			}
		}
		else if ( !shouldBeInstalled && currentlyInstalled )
		{
			CUtlString path;
			path.Format( "%s/%d.zip", pClient->m_downloadFolder.String(), modId );
			filesystem->RemoveFile( path.String(), "MOD" );
			DevMsg( "GitHub: Unsubscribed - removed %s\n", path.String() );
			pendingUnsubscribes++;
		}
	}

	if ( pendingUnsubscribes > 0 && pendingDownloads == 0 )
	{
		PopulateModList();
	}
}

void CBrowsePage::PerformLayout()
{
	PropertyPage::PerformLayout();

	if ( m_pModGrid )
		m_pModGrid->SetBounds( 0, 0, GetWide(), GetTall() );
}

CSubscribedPage::CSubscribedPage( Panel *parent, const char *panelName ) : PropertyPage( parent, panelName )
{
	SetSize( 800, 600 );

	m_pModCanvas = new vgui::EditablePanel( this, "ModGridCanvas" );
	m_pModCanvas->SetSize( 800, 2000 );

	m_pModGrid = new vgui::ScrollableEditablePanel( this, m_pModCanvas, "ModGrid" );
	m_pModGrid->SetBounds( 0, 0, GetWide(), GetTall() );
	m_pModGrid->SetPaintBackgroundEnabled( false );
	m_pModGrid->GetScrollbar()->SetWide( 16 );
	m_pModGrid->GetScrollbar()->SetVisible( true );
}

CSubscribedPage::~CSubscribedPage()
{
}

static void CSubscribedPage_FetchCallback( CUtlVector< Mod > &mods, void *user )
{
	CSubscribedPage *pPage = (CSubscribedPage *)user;
	if ( pPage )
		pPage->OnModsReceived();
}

void CSubscribedPage::RefreshSubscribedList()
{
	for ( int i = 0; i < m_ModPanels.Count(); i++ )
	{
		m_ModPanels[i]->MarkForDeletion();
	}
	m_ModPanels.RemoveAll();

	WorkshopClient *pClient = GetWorkshopClient();
	if ( !pClient )
		return;

	CUtlVector< int > installedIDs;
	pClient->GetInstalledModIDs( installedIDs );

	const CUtlVector< Mod > &allMods = pClient->GetCachedMods();

	int columns = 3;
	int panelWidth = 216;
	int panelHeight = 182;
	int spacingX = 15;
	int spacingY = 15;
	int startX = 10;
	int startY = 10;
	int displayed = 0;

	for ( int i = 0; i < allMods.Count(); i++ )
	{
		if ( installedIDs.Find( allMods[i].id ) == installedIDs.InvalidIndex() )
			continue;

		int col = displayed % columns;
		int row = displayed / columns;

		CModThumbnailPanel *pPanel = new CModThumbnailPanel( m_pModCanvas, "SubModPanel", allMods[i], false );
		pPanel->SetPos( startX + col * ( panelWidth + spacingX ), startY + row * ( panelHeight + spacingY ) );
		pPanel->SetVisible( true );
		pPanel->SetZPos( 10 );
		pPanel->SetSubscribed( true );
		m_ModPanels.AddToTail( pPanel );
		displayed++;
	}

	int rows = ( displayed + columns - 1 ) / columns;
	int requiredHeight = startY + rows * ( panelHeight + spacingY ) + 50;

	if ( m_pModCanvas )
		m_pModCanvas->SetSize( GetWide() - 20, requiredHeight );

	m_pModGrid->InvalidateLayout( true, true );
}

void CSubscribedPage::OnPageShow()
{
	PropertyPage::OnPageShow();

	ClearPanels();

	WorkshopClient *pClient = GetWorkshopClient();
	if ( !pClient )
		return;

	pClient->FetchMods( CSubscribedPage_FetchCallback, this );
}

void CSubscribedPage::OnModsReceived()
{
	RefreshSubscribedList();
}

void CSubscribedPage::PerformLayout()
{
	PropertyPage::PerformLayout();

	if ( m_pModGrid )
		m_pModGrid->SetBounds( 0, 0, GetWide(), GetTall() );
}

CWorkshopDialog::CWorkshopDialog( Panel *parent ) : PropertyDialog( parent, "WorkshopDialog" )
{
	SetTitle( "Workshop", true );
	SetSize( 850, 650 );

	SetSizeable( false );
	SetMoveable( true );
	SetCloseButtonVisible( true );
	SetApplyButtonVisible( true );

	int pageW = GetWide() - 30;
	int pageH = GetTall() - 80;

	m_pBrowsePage = new CBrowsePage( this, "BrowsePage" );
	m_pBrowsePage->SetPos( 10, 30 );
	m_pBrowsePage->SetSize( pageW, pageH );

	m_pSubscribedPage = new CSubscribedPage( this, "SubscribedPage" );
	m_pSubscribedPage->SetPos( 10, 30 );
	m_pSubscribedPage->SetSize( pageW, pageH );

	AddPage( m_pBrowsePage, "Browse Addons" );
	AddPage( m_pSubscribedPage, "Subscribed" );
}

CWorkshopDialog::~CWorkshopDialog()
{
	g_pWorkshopDialog = nullptr;
}

void CWorkshopDialog::Activate()
{
	PropertyDialog::Activate();
	MoveToCenterOfScreen();
}

CWorkshopDialog *GetWorkshopDialog()
{
	if ( !g_pWorkshopDialog )
	{
		g_pWorkshopDialog = new CWorkshopDialog( nullptr );
	}
	return g_pWorkshopDialog;
}

void ShowWorkshop()
{
	CWorkshopDialog *pDialog = GetWorkshopDialog();
	if ( pDialog )
		pDialog->Activate();
}

CON_COMMAND( modio_workshop, "Open the GitHub workshop" )
{
	if ( !GetWorkshopClient() )
	{
		Warning( "GitHub: Client not initialized.\n" );
		return;
	}

	ShowWorkshop();
}

class CWorkshopGameSystem : public CAutoGameSystem
{
public:
	CWorkshopGameSystem()
	{
	}
	virtual ~CWorkshopGameSystem()
	{
	}

	virtual bool Init() override
	{
		InitWorkshopClient( "addons/" );
		return true;
	}

	virtual void Shutdown() override
	{
		ShutdownWorkshopClient();
	}
};
static CWorkshopGameSystem g_WorkshopSystem;
