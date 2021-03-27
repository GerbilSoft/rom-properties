/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsMenuButton.hpp: Options menu button WC_BUTTON superclass.        *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_OPTIONSMENUBUTTON_HPP__
#define __ROMPROPERTIES_WIN32_OPTIONSMENUBUTTON_HPP__

#include "libwin32common/RpWin32_sdk.h"

// librpbase
#include "librpbase/RomData.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#define WC_OPTIONSMENUBUTTON		_T("rp-OptionsMenuButton")

/** WNDCLASS registration functions **/
void OptionsMenuButtonRegister(void);
void OptionsMenuButtonUnregister(void);

#define WM_OMB_REINIT_MENU		(WM_USER + 1)	// lParam == RomData*
#define WM_OMB_UPDATE_OP		(WM_USER + 2)	// wParam == id; lParam == RomData::RomOp*
#define WM_OMB_POPUP_MENU		(WM_USER + 3)	// returns: id+IDM_OPTIONS_MENU_BASE, or 0 if none.

enum StandardOptionID {
	OPTION_EXPORT_TEXT = -1,
	OPTION_EXPORT_JSON = -2,
	OPTION_COPY_TEXT = -3,
	OPTION_COPY_JSON = -4,
};

// "Options" menu item.
#define IDM_OPTIONS_MENU_BASE		0x8000
#define IDM_OPTIONS_MENU_EXPORT_TEXT	(IDM_OPTIONS_MENU_BASE + OPTION_EXPORT_TEXT)
#define IDM_OPTIONS_MENU_EXPORT_JSON	(IDM_OPTIONS_MENU_BASE + OPTION_EXPORT_JSON)
#define IDM_OPTIONS_MENU_COPY_TEXT	(IDM_OPTIONS_MENU_BASE + OPTION_COPY_TEXT)
#define IDM_OPTIONS_MENU_COPY_JSON	(IDM_OPTIONS_MENU_BASE + OPTION_COPY_JSON)

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/**
 * Reset the menu items using the specified RomData object.
 * @param hWnd OptionsMenuButton
 * @param romData RomData object
 */
static inline void OptionsMenuButton_ReinitMenu(HWND hWnd, const LibRpBase::RomData *romData)
{
	SendMessage(hWnd, WM_OMB_REINIT_MENU, 0, reinterpret_cast<LPARAM>(romData));
}

/**
 * Update a ROM operation menu item.
 * @param hWnd OptionsMenuButton
 * @param id ROM operation ID
 * @param op ROM operation
 */
static inline void OptionsMenuButton_UpdateOp(HWND hWnd, int id, const LibRpBase::RomData::RomOp *op)
{
	SendMessage(hWnd, WM_OMB_UPDATE_OP, static_cast<WPARAM>(id), reinterpret_cast<LPARAM>(op));
}

/**
 * Popup the menu.
 * FIXME: Move WM_COMMAND handling from RP_ShellPropSheetExt to here.
 * @param hWnd OptionsMenuButton
 * @return Selected menu item ID (+IDM_OPTIONS_MENU_BASE), or 0 if none.
 */
static inline int OptionsMenuButton_PopupMenu(HWND hWnd)
{
	return static_cast<int>(SendMessage(hWnd, WM_OMB_POPUP_MENU, 0, 0));
}

#endif /* __cplusplus */

#endif /* __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__ */
