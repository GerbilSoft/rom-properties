/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * OptionsMenuButton.cpp: Options menu button WC_BUTTON superclass.        *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsMenuButton.hpp"
#include "res/resource.h"

// Other rom-properties libraries
#include "libi18n/i18n.h"
#include "librpbase/RomData.hpp"
#include "librptext/wchar.hpp"
using LibRpBase::RomData;

// libwin32ui
#include "libwin32ui/WinUI.hpp"

// C++ STL classes
using std::vector;

static ATOM atom_optionsMenuButton;
static WNDPROC pfnButtonWndProc;

/** Standard actions. **/
struct option_menu_action_t {
	const char *desc;
	int id;
};
static const option_menu_action_t stdacts[] = {
	{NOP_C_("OptionsMenuButton|StdActs", "Export to Text..."),	OPTION_EXPORT_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Export to JSON..."),	OPTION_EXPORT_JSON},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as Text"),		OPTION_COPY_TEXT},
	{NOP_C_("OptionsMenuButton|StdActs", "Copy as JSON"),		OPTION_COPY_JSON},
};

class OptionsMenuButtonPrivate
{
	public:
		explicit OptionsMenuButtonPrivate(HWND hWnd);
		~OptionsMenuButtonPrivate();

	public:
		HWND hWnd;		// OptionsMenuButton control
		HMENU hMenuOptions;	// Popup menu

		// Is the UI locale right-to-left?
		// If so, this will be set to WS_EX_LAYOUTRTL.
		DWORD dwExStyleRTL;

	public:
		/**
		* Reset the menu items using the specified RomData object.
		 * @param hWnd OptionsMenuButton
		 * @param romData RomData object
		 */
		void reinitMenu(const RomData *romData);

		/**
		 * Update a ROM operation menu item.
		 * @param hWnd OptionsMenuButton
		 * @param id ROM operation ID
		 * @param op ROM operation
		 */
		void updateOp(int id, const RomData::RomOp *op);

		/**
		 * Popup the menu.
		 * @return Selected menu item ID (+IDM_OPTIONS_MENU_BASE), or 0 if none.
		 */
		int popupMenu(void);
};

OptionsMenuButtonPrivate::OptionsMenuButtonPrivate(HWND hWnd)
	: hWnd(hWnd)
	, hMenuOptions(nullptr)
	, dwExStyleRTL(LibWin32UI::isSystemRTL())
{
	// Initialize the text and style for the "Options" menu button.
	const bool isComCtl32_v610 = LibWin32UI::isComCtl32_v610();

	if (isComCtl32_v610) {
		// COMCTL32 is v6.10 or later. Use BS_SPLITBUTTON.
		// (Windows Vista or later)
		LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
		lStyle |= BS_SPLITBUTTON;
		SetWindowLong(hWnd, GWL_STYLE, lStyle);

		// tr: "Options" button.
		SetWindowText(hWnd, U82T_c(C_("OptionsMenuButton", "&Options")));

		// Button split.
		BUTTON_SPLITINFO bsi;
		bsi.mask = BCSIF_STYLE;
		bsi.uSplitStyle = BCSS_NOSPLIT;
		Button_SetSplitInfo(hWnd, &bsi);
	} else {
		// COMCTL32 is older than v6.10. Use a regular button.
		// NOTE: The Unicode down arrow doesn't show on on Windows XP.
		// Maybe we *should* use ownerdraw...

		// tr: "Options" button. (WinXP version, with ellipsis.)
		SetWindowText(hWnd, U82T_c(C_("OptionsMenuButton", "&Options...")));
	}
}

OptionsMenuButtonPrivate::~OptionsMenuButtonPrivate()
{
	// Delete the popup menu.
	if (hMenuOptions) {
		DestroyMenu(hMenuOptions);
	}
}

/**
* Reset the menu items using the specified RomData object.
 * @param hWnd OptionsMenuButton
 * @param romData RomData object
 */
void OptionsMenuButtonPrivate::reinitMenu(const RomData *romData)
{
	// Delete the menu if it was already created.
	if (hMenuOptions) {
		DestroyMenu(hMenuOptions);
	}

	// Create the menu.
	hMenuOptions = CreatePopupMenu();

	// Add the standard actions.
	for (const option_menu_action_t &p : stdacts) {
		AppendMenu(hMenuOptions, MF_STRING, IDM_OPTIONS_MENU_BASE + p.id,
			U82T_c(dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p.desc)));
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		AppendMenu(hMenuOptions, MF_SEPARATOR, 0, nullptr);

		unsigned int i = IDM_OPTIONS_MENU_BASE;
		for (const auto &op : ops) {
			UINT uFlags;
			if (!(op.flags & RomData::RomOp::ROF_ENABLED)) {
				uFlags = MF_STRING | MF_DISABLED;
			} else {
				uFlags = MF_STRING;
			}
			AppendMenu(hMenuOptions, uFlags, i, U82T_c(op.desc));

			// Next menu item.
			i++;
		}
	}
}

/**
 * Update a ROM operation menu item.
 * @param hWnd OptionsMenuButton
 * @param id ROM operation ID
 * @param op ROM operation
 */
void OptionsMenuButtonPrivate::updateOp(int id, const RomData::RomOp *op)
{
	assert(id >= 0);
	assert(op != nullptr);
	if (id < 0 || !op)
		return;

	UINT uFlags;
	if (!(op->flags & RomData::RomOp::ROF_ENABLED)) {
		uFlags = MF_BYCOMMAND | MF_STRING | MF_DISABLED;
	} else {
		uFlags = MF_BYCOMMAND | MF_STRING;
	}

	const int menuId = id + IDM_OPTIONS_MENU_BASE;
	ModifyMenu(hMenuOptions, menuId, uFlags, menuId, U82T_c(op->desc));
}

/**
 * Popup the menu.
 * @return Selected menu item ID (+IDM_OPTIONS_MENU_BASE), or 0 if none.
 */
int OptionsMenuButtonPrivate::popupMenu(void)
{
	// FIXME: Should the owner be the toplevel window?

	// Get the absolute position of the "Options" button.
	RECT rect_btnOptions;
	GetWindowRect(hWnd, &rect_btnOptions);
	int id = TrackPopupMenu(hMenuOptions,
		TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_VERNEGANIMATION |
			TPM_NONOTIFY | TPM_RETURNCMD,
		rect_btnOptions.left, rect_btnOptions.top, 0,
		hWnd, nullptr);

	// TODO: Send a notification instead of returning a value?
	return id;
}

/** OptionsMenuButton **/

static LRESULT CALLBACK
OptionsMenuButtonWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// FIXME: Don't use GWLP_USERDATA; use extra window bytes?
	switch (uMsg) {
		default:
			break;

		case WM_CREATE: {
			// NOTE: WM_NCCREATE is sent too early to set BS_SPLITBUTTON,
			// so we're doing this in WM_CREATE instead.
			OptionsMenuButtonPrivate *const d = new OptionsMenuButtonPrivate(hWnd);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			break;
		}

		case WM_NCDESTROY: {
			OptionsMenuButtonPrivate *const d = reinterpret_cast<OptionsMenuButtonPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			delete d;
			break;
		}

		case WM_OMB_REINIT_MENU: {
			OptionsMenuButtonPrivate *const d = reinterpret_cast<OptionsMenuButtonPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->reinitMenu(reinterpret_cast<const RomData*>(lParam));
			break;
		}

		case WM_OMB_UPDATE_OP: {
			OptionsMenuButtonPrivate *const d = reinterpret_cast<OptionsMenuButtonPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			d->updateOp(static_cast<int>(wParam),
				reinterpret_cast<const RomData::RomOp*>(lParam));
			break;
		}

		case WM_OMB_POPUP_MENU: {
			OptionsMenuButtonPrivate *const d = reinterpret_cast<OptionsMenuButtonPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->popupMenu();
		}
	}

	// Forward the message to the ComboBoxEx class.
	return CallWindowProc(pfnButtonWndProc, hWnd, uMsg, wParam, lParam);
}

/** WNDCLASS registration functions **/

void OptionsMenuButtonRegister(void)
{
	if (atom_optionsMenuButton != 0)
		return;

	// OptionsMenuButton is superclassing ComboBoxEx.

	// Get Button class information.
	// TODO: Do we need to use GetClassInfo for BUTTON?
	WNDCLASS wndClass;
	BOOL bRet = GetClassInfo(nullptr, WC_BUTTON, &wndClass);
	assert(bRet != FALSE);
	if (!bRet) {
		// Error getting class info.
		return;
	}

	pfnButtonWndProc = wndClass.lpfnWndProc;
	wndClass.lpfnWndProc = OptionsMenuButtonWndProc;
	wndClass.style &= ~CS_GLOBALCLASS;
	wndClass.hInstance = HINST_THISCOMPONENT;
	wndClass.lpszClassName = WC_OPTIONSMENUBUTTON;

	atom_optionsMenuButton = RegisterClass(&wndClass);
}

void OptionsMenuButtonUnregister(void)
{
	if (atom_optionsMenuButton != 0) {
		UnregisterClass(MAKEINTATOM(atom_optionsMenuButton), HINST_THISCOMPONENT);
		atom_optionsMenuButton = 0;
	}
}
