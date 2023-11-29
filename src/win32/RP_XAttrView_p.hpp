/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_XAttrView_p.hpp: Extended attribute viewer property page.            *
 * (Private class)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// librpfile
namespace LibRpFile {
	class XAttrReader;
}

// TCHAR
#include "tcharx.h"

/** RP_XAttrView_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_XAttrViewPrivate RP_XAttrView_Private

class RP_XAttrView_Private
{
public:
	/**
	 * RP_XAttrView_Private constructor
	 * @param q
	 * @param filename Filename (RP_XAttrView_Private takes ownership)
	 */
	explicit RP_XAttrView_Private(RP_XAttrView *q, LPTSTR filename);

	~RP_XAttrView_Private();

private:
	RP_DISABLE_COPY(RP_XAttrView_Private)
private:
	RP_XAttrView *const q_ptr;

public:
	// Property for "tab pointer".
	// This points to the RP_XAttrView_Private::tab object.
	static const TCHAR TAB_PTR_PROP[];

public:
	HWND hDlgSheet;				// Property sheet
	LPTSTR tfilename;			// Opened file
	LibRpFile::XAttrReader *xattrReader;	// XAttrReader

	// wtsapi32.dll for Remote Desktop status. (WinXP and later)
	LibWin32UI::WTSSessionNotification wts;

	/**
	 * ListView CustomDraw function.
	 * @param plvcd	[in/out] NMLVCUSTOMDRAW
	 * @return Return value.
	 */
	inline int ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const;

public:
	// Is the UI locale right-to-left?
	// If so, this will be set to WS_EX_LAYOUTRTL.
	DWORD dwExStyleRTL;

	// Alternate row color.
	COLORREF colorAltRow;
	bool isFullyInit;		// True if the window is fully initialized.

private:
	/**
	 * Update the MS-DOS attribute widgets.
	 * dosAttrs must be set to the current attributes.
	 */
	void updateDosAttrWidgets(void);

	/**
	 * Load MS-DOS attributes, if available.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadDosAttrs(void);

	/**
	 * Load alternate data streams, if available.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadADS(void);

public:
	/**
	 * Load the attributes from the specified file.
	 * The attributes will be loaded into the display widgets.
	 * @return 0 on success; negative POSIX error code on error.
	 */
	int loadAttributes(void);

	/**
	 * Clear the display widgets.
	 */
	void clearDisplayWidgets();

public:
	/**
	 * Initialize the dialog. (hDlgSheet)
	 * Called by WM_INITDIALOG.
	 */
	void initDialog(void);

private:
	/**
	 * Convert a bool value to BST_CHCEKED or BST_UNCHECKED.
	 * @param value Bool value.
	 * @return BST_CHECKED or BST_UNCHECKED.
	 */
	static inline int boolToBstChecked(bool value)
	{
		return (value ? BST_CHECKED : BST_UNCHECKED);
	}

	/**
	 * Convert BST_CHECKED or BST_UNCHECKED to a bool.
	 * @param value BST_CHECKED or BST_UNCHECKED.
	 * @return bool.
	 */
	static inline bool bstCheckedToBool(unsigned int value)
	{
		return (value == BST_CHECKED);
	}

	// Internal functions used by the callback functions.
	INT_PTR DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr);
	INT_PTR DlgProc_WM_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam);

public:
	// Property sheet callback functions.
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	/**
	 * Dialog procedure for subtabs.
	 * @param hDlg
	 * @param uMsg
	 * @param wParam
	 * @param lParam
	 */
	static INT_PTR CALLBACK SubtabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	// Resource map: Maps control IDs to attribute values.
	// Base controL ID is: IDC_XATTRVIEW_DOS_READONLY
	static const std::array<uint16_t, 6> res_map;

private:
	/** Attributes **/
	uint32_t dosAttrs;	// MS-DOS attributes as currently displayed by the checkboxes
};
