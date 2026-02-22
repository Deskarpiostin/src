//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#ifndef BASEPANEL_H
#define BASEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "menubar.h"
#include "menu/imageextbutton.h"

namespace vgui
{
class Panel;
class IScheme;
class Label;
}; // namespace vgui

// I'm too lazy to include the file here
class CAutoGameSystem;

class CHoverButton : public vgui::Label
{
	DECLARE_CLASS_SIMPLE( CHoverButton, vgui::Label );

public:
	CHoverButton( vgui::Panel *parent, const char *panelName );

	virtual void OnCursorEntered() override;
	virtual void OnCursorExited() override;
	virtual void OnMousePressed( vgui::MouseCode code ) override;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) override;

	void SetCommand( const char *cmd );

private:
	Color		m_DefaultColor;
	Color		m_HoverColor;
	vgui::HFont m_DefaultFont;
	vgui::HFont m_BoldFont;
	CUtlString	m_Command;
};

class CMainMenu : public vgui::Panel
{
	DECLARE_CLASS( CMainMenu, vgui::Panel );

public:
	CMainMenu( vgui::VPANEL parent );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	void		 LoadGameMenu();

private:
	CMenuBar					*m_pMenuBar;
	CHoverButton				*m_pStartButton;
	ImageExtButton				*m_pLogo;
	CUtlVector< CHoverButton * > m_GameMenuButtons;
};

class CMainMenuSystem : public CAutoGameSystem
{
	DECLARE_CLASS( CMainMenuSystem, CAutoGameSystem );

public:
	CMainMenuSystem();

	virtual void PostInit() OVERRIDE;

	virtual void LevelInitPostEntity() OVERRIDE;
	virtual void LevelShutdownPostEntity() OVERRIDE;
};

#endif
