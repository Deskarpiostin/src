//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//
#ifndef WORKSHOP_H
#define WORKSHOP_H
#ifdef _WIN32
#pragma once
#endif // _WIN32

#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>
#include <vgui/IImage.h>
#include <vgui_controls/ScrollableEditablePanel.h>

using namespace vgui;

struct Mod
{
	int		   id;
	CUtlString name;
	CUtlString description;
	CUtlString version;
	CUtlString author;
	int		   fileSize;
	CUtlString downloadUrl;
	CUtlString logoUrl;
	CUtlString localLogoPath;

	Mod() : id( 0 ), fileSize( 0 )
	{
	}

	Mod( const Mod &other ) :
		id( other.id ),
		name( other.name ),
		description( other.description ),
		version( other.version ),
		fileSize( other.fileSize ),
		downloadUrl( other.downloadUrl ),
		logoUrl( other.logoUrl ),
		localLogoPath( other.localLogoPath )
	{
	}

	Mod &operator=( const Mod &other )
	{
		if ( this != &other )
		{
			id = other.id;
			name = other.name;
			description = other.description;
			version = other.version;
			fileSize = other.fileSize;
			downloadUrl = other.downloadUrl;
			logoUrl = other.logoUrl;
			localLogoPath = other.localLogoPath;
		}
		return *this;
	}
};

typedef void ( *ModListCallback )( CUtlVector< Mod > &mods, void *user );
typedef void ( *ModActionCallback )( bool success, const char *errorMsg );

class WorkshopClient
{
public:
	WorkshopClient( const char *downloadFolder );
	~WorkshopClient();

	CUtlString GetModDownloadUrlForMod( int modId );

	void FetchMods( ModListCallback cb, void *user );
	bool DownloadMod( Mod *mod, ModActionCallback cb, void *user );

	const CUtlVector< Mod > &GetCachedMods() const
	{
		return m_cachedMods;
	}
	const char *GetAccessToken() const
	{
		return m_accessToken.String();
	}
	bool IsAuthenticated() const
	{
		return !m_accessToken.IsEmpty();
	}

	void GetInstalledModIDs( CUtlVector< int > &outIDs );

	CUtlString m_downloadFolder;

private:
	CUtlString		  m_accessToken;
	CUtlVector< Mod > m_cachedMods;

	CUtlString m_tempFolder;

	bool ParseModsJSON( const char *jsonText, CUtlVector< Mod > &outMods );
	bool DownloadUrlToFile( const char *url, const char *outPath );
};

WorkshopClient *GetModioClient();
void			InitModioClient( const char *downloadFolder );
void			ShutdownModioClient();

class CWorkshopDialog;
class CModThumbnailPanel;

class CModThumbnailPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( CModThumbnailPanel, Panel );

public:
	CModThumbnailPanel( Panel *parent, const char *panelName, const Mod &mod, bool allowSubscribe = true );
	~CModThumbnailPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnMousePressed( MouseCode code );
	virtual void PaintBackground();
	virtual void PerformLayout();

	void ShowModDetails();

	void SetSubscribed( bool subscribed );
	bool IsSubscribed() const
	{
		return m_bSubscribed;
	}
	int GetModID() const
	{
		return m_modId;
	}
	const Mod &GetMod() const
	{
		return m_cachedMod;
	}
	void SetImage( IImage *pImage );

	MESSAGE_FUNC_INT( OnCheckButtonChecked, "CheckButtonChecked", state );

private:
	Panel		*m_pImageContainer;
	CheckButton *m_pCheckbox;
	Label		*m_pNameLabel;
	Label		*m_pSizeLabel;
	bool		 m_bSubscribed;
	int			 m_modId;
	Mod			 m_cachedMod;
	IImage		*m_pDownloadedImage;
	bool		 m_bHovered;
	bool		 m_allowSubscribe;
};

class CBrowsePage : public PropertyPage
{
	DECLARE_CLASS_SIMPLE( CBrowsePage, PropertyPage );

public:
	CBrowsePage( Panel *parent, const char *panelName );
	~CBrowsePage();

	virtual void OnPageShow();
	virtual void PerformLayout();
	void		 RefreshModList();
	void		 ApplySubscriptions();

	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );

	MESSAGE_FUNC( OnRefreshClicked, "RefreshClicked" );

private:
	void		PopulateModList();
	void		OnModsReceived( CUtlVector< Mod > &mods );
	static void ModsReceivedCallback( CUtlVector< Mod > &mods, void *user );

	ScrollableEditablePanel			  *m_pModGrid;
	CUtlVector< CModThumbnailPanel * > m_ModPanels;
};

class CSubscribedPage : public PropertyPage
{
	DECLARE_CLASS_SIMPLE( CSubscribedPage, PropertyPage );

public:
	CSubscribedPage( Panel *parent, const char *panelName );
	~CSubscribedPage();

	virtual void OnPageShow();
	virtual void PerformLayout();
	void		 RefreshSubscribedList();

	void ClearPanels()
	{
		for ( int i = 0; i < m_ModPanels.Count(); i++ )
		{
			m_ModPanels[i]->MarkForDeletion();
		}
		m_ModPanels.RemoveAll();
	}

	void OnModsReceived();

	static void ModsReceivedCallback( CUtlVector< Mod > &mods, void *user )
	{
		CSubscribedPage *pPage = (CSubscribedPage *)user;
		if ( pPage )
			pPage->OnModsReceived();
	}

private:
	ScrollableEditablePanel			  *m_pModGrid;
	CUtlVector< CModThumbnailPanel * > m_ModPanels;
	vgui::EditablePanel				  *m_pModCanvas;
};

class CWorkshopDialog : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE( CWorkshopDialog, PropertyDialog );

public:
	CWorkshopDialog( Panel *parent );
	~CWorkshopDialog();

	virtual void Activate();

private:
	CBrowsePage		*m_pBrowsePage;
	CSubscribedPage *m_pSubscribedPage;
};

CWorkshopDialog *GetWorkshopDialog();
void			 ShowModioWorkshop();

#endif // WORKSHOP_H
