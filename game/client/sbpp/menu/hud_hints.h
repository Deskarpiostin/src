//========== Copyright (C) 2026, Team HL2SB++, All rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#ifndef HUD_HINTS
#define HUD_HINTS
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimationController.h>
#include <hud.h>
#include <vector>
#include <unordered_map>
#include <string>

using namespace vgui;

enum NotifyType
{
	NOTIFY_GENERIC = 0,
	NOTIFY_ERROR,
	NOTIFY_UNDO,
	NOTIFY_HINT,
	NOTIFY_CLEANUP
};

class NoticePanel : public Panel
{
	DECLARE_CLASS_SIMPLE( NoticePanel, Panel );

public:
	NoticePanel( Panel *parent );
	virtual ~NoticePanel();

	void SetText( const char *text );
	void SetLegacyType( int t );
	void SetProgress( float frac );
	bool KillSelf();			// returns true if removed
	void Start( float length ); // set start time/length

	virtual void PerformLayout() override;
	virtual void Paint() override;

	float fx, fy;
	float VelX, VelY;
	float StartTime;
	float Length; // seconds; negative = infinite
	bool  Progress;
	float ProgressFrac;

private:
	Label	   *m_pLabel;
	ImagePanel *m_pImage;
	int			m_nType;
};

#endif