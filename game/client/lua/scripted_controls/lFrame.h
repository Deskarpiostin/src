//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_LFRAME_H
#define VGUI_LFRAME_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Lua wrapper for a windowed frame
//-----------------------------------------------------------------------------
class LFrame : public Frame
{
	DECLARE_CLASS_SIMPLE( LFrame, Frame );

public:
	LFrame(Panel *parent, const char *panelName, bool showTaskbarIcon = true, lua_State *L = NULL);
	virtual ~LFrame();

	virtual void OnMove();

	// called when scheme settings need to be applied; called the first time before the panel is painted
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// interface to build settings
	// takes a group of settings and applies them to the control
	virtual void ApplySettings(KeyValues *inResourceData);

	// message handlers
	// override to get access to the message
	// override to get access to the message
	virtual void OnMessage(const KeyValues *params, VPANEL fromPanel);	// called when panel receives message; must chain back
	MESSAGE_FUNC_CHARPTR( OnCommand, "Command", command );	// called when a panel receives a command
	MESSAGE_FUNC( OnMouseCaptureLost, "MouseCaptureLost" );	// called after the panel loses mouse capture
	MESSAGE_FUNC( OnSetFocus, "SetFocus" );			// called after the panel receives the keyboard focus
	MESSAGE_FUNC( OnKillFocus, "KillFocus" );		// called after the panel loses the keyboard focus
	MESSAGE_FUNC( OnDelete, "Delete" );				// called to delete the panel; Panel::OnDelete() does simply { delete this; }
	virtual void OnThink();							// called every frame before painting, but only if panel is visible
	virtual void OnChildAdded(VPANEL child);		// called when a child has been added to this panel
	virtual void OnSizeChanged(int newWide, int newTall);	// called after the size of a panel has been changed
	
	// called every frame if ivgui()->AddTickSignal() is called
	MESSAGE_FUNC( OnTick, "Tick" );

	// input messages
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMouseWheeled(int delta);

	// Trip pressing (e.g., select all text in a TextEntry) requires this to be enabled
	virtual void OnMouseTriplePressed( MouseCode code );

	// base implementation forwards Key messages to the Panel's parent 
	// - override to 'swallow' the input
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyCodeReleased(KeyCode code);
	virtual void OnKeyFocusTicked(); // every window gets key ticked events

	// forwards mouse messages to the panel's parent
	MESSAGE_FUNC( OnMouseFocusTicked, "OnMouseFocusTicked" );

	// message handlers that don't go through the message pump
	virtual void PaintBackground();
	virtual void Paint();
	virtual void PaintBorder();
	virtual void PaintBuildOverlay();		// the extra drawing for when in build mode
	virtual void PostChildPaint();
	virtual void PerformLayout();

protected:
	MESSAGE_FUNC_ENUM_ENUM( OnRequestFocus, "OnRequestFocus", VPANEL, subFocus, VPANEL, defaultPanel);
	MESSAGE_FUNC_INT_INT( OnScreenSizeChanged, "OnScreenSizeChanged", oldwide, oldtall );

public:
#if defined( LUA_SDK )
	lua_State *m_lua_State;
	int m_nTableReference;
	int m_nRefCount;
#endif
};

} // namespace vgui

/* type for Frame functions */
typedef vgui::Frame lua_Frame;



/*
** access functions (stack -> C)
*/

LUA_API lua_Frame     *(lua_toframe) (lua_State *L, int idx);


/*
** push functions (C -> stack)
*/
LUA_API void  (lua_pushframe) (lua_State *L, lua_Frame *pFrame);



LUALIB_API lua_Frame *(luaL_checkframe) (lua_State *L, int narg);

#endif // VGUI_LFRAME_H
