/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * WinUI.hpp: Windows UI common functions.                                 *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__

// C++ includes
#include <string>

#include "RpWin32_sdk.h"
#include "common.h"	// for RP_LIBROMDATA_PUBLIC

namespace LibWin32Common {

/**
 * Convert UNIX line endings to DOS line endings.
 * @param tstr_unix	[in] String with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return tstring with DOS line endings.
 */
RP_LIBROMDATA_PUBLIC
std::tstring unix2dos(const TCHAR *tstr_unix, int *lf_count = nullptr);

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
RP_LIBROMDATA_PUBLIC
int measureTextSize(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize);

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
static inline int measureTextSize(HWND hWnd, HFONT hFont, const std::tstring &tstr, LPSIZE lpSize)
{
	return measureTextSize(hWnd, hFont, tstr.c_str(), lpSize);
}

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
RP_LIBROMDATA_PUBLIC
int measureTextSizeLink(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize);

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
static inline int measureTextSizeLink(HWND hWnd, HFONT hFont, const std::tstring &tstr, LPSIZE lpSize)
{
	return measureTextSizeLink(hWnd, hFont, tstr.c_str(), lpSize);
}

/**
 * Get the alternate row color for ListViews.
 *
 * This function should be called on ListView creation
 * and if the system theme is changed.
 *
 * @return Alternate row color for ListViews.
 */
RP_LIBROMDATA_PUBLIC
COLORREF getAltRowColor(void);

/**
 * Get the alternate row color for ListViews in ARGB32 format.
 * @return Alternate row color for ListViews in ARGB32 format.
 */
static inline uint32_t getAltRowColor_ARGB32(void)
{
	const COLORREF color = getAltRowColor();
	return  (color & 0x00FF00) | 0xFF000000 |
	       ((color & 0xFF) << 16) |
	       ((color >> 16) & 0xFF);
}

/**
 * Get a Windows system color in ARGB32 format.
 *
 * Both GDI+ and rp_image use ARGB32 format,
 * whereas Windows' GetSysColor() uses ABGR32.
 *
 * @param nIndex System color index.
 * @return ARGB32 system color.
 */
static inline uint32_t GetSysColor_ARGB32(int nIndex)
{
	const COLORREF color = GetSysColor(nIndex);
	return  (color & 0x00FF00) | 0xFF000000 |
	       ((color & 0xFF) << 16) |
	       ((color >> 16) & 0xFF);
}

/**
 * Are we using COMCTL32.DLL v6.10 or later?
 * @return True if it's v6.10 or later; false if not.
 */
RP_LIBROMDATA_PUBLIC
bool isComCtl32_v610(void);

/**
 * Measure the width of a string for ListView.
 * This function handles newlines.
 * @param hDC          [in] HDC for text measurement.
 * @param tstr         [in] String to measure.
 * @param pNlCount     [out,opt] Newline count.
 * @return Width. (May return LVSCW_AUTOSIZE_USEHEADER if it's a single line.)
 */
RP_LIBROMDATA_PUBLIC
int measureStringForListView(HDC hDC, const std::tstring &tstr, int *pNlCount = nullptr);

/**
 * Is the system using an RTL language?
 * @return WS_EX_LAYOUTRTL if the system is using RTL; 0 if not.
 */
RP_LIBROMDATA_PUBLIC
DWORD isSystemRTL(void);

/** File dialogs **/

/**
 * Get a filename using the Open File Name dialog.
 *
 * Depending on OS, this may use:
 * - Vista+: IFileOpenDialog
 * - XP: GetOpenFileName()
 *
 * @param hWnd		[in] Owner.
 * @param dlgTitle	[in] Dialog title.
 * @param filterSpec	[in] Filter specification. (RP format, UTF-8)
 * @param origFilename	[in,opt] Starting filename.
 * @return Filename, or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
std::tstring getOpenFileName(HWND hWnd, const TCHAR *dlgTitle, const char *filterSpec, const TCHAR *origFilename);

/**
 * Get a filename using the Save File Name dialog.
 *
 * Depending on OS, this may use:
 * - Vista+: IFileSaveDialog
 * - XP: GetSaveFileName()
 *
 * @param hWnd		[in] Owner.
 * @param dlgTitle	[in] Dialog title.
 * @param filterSpec	[in] Filter specification. (RP format, UTF-8)
 * @param origFilename	[in,opt] Starting filename.
 * @return Filename, or empty string on error.
 */
RP_LIBROMDATA_PUBLIC
std::tstring getSaveFileName(HWND hWnd, const TCHAR *dlgTitle, const char *filterSpec, const TCHAR *origFilename);

/** Window procedure subclasses **/

/**
 * Subclass procedure for multi-line EDIT and RICHEDIT controls.
 * This procedure does the following:
 * - ENTER and ESCAPE are forwarded to the parent window.
 * - DLGC_HASSETSEL is masked.
 *
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	HWND of parent dialog to forward WM_COMMAND messages to.
 */
RP_LIBROMDATA_PUBLIC
LRESULT CALLBACK MultiLineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);


/**
 * Subclass procedure for single-line EDIT and RICHEDIT controls.
 * This procedure does the following:
 * - DLGC_HASSETSEL is masked.
 *
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	HWND of parent dialog to forward WM_COMMAND messages to.
 */
RP_LIBROMDATA_PUBLIC
LRESULT CALLBACK SingleLineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

/**
 * Subclass procedure for ListView controls to disable HDN_DIVIDERDBLCLICK handling.
 * @param hWnd		Dialog handle
 * @param uMsg		Message
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID (usually the control ID)
 * @param dwRefData	RP_ShellPropSheetExt_Private*
 */
RP_LIBROMDATA_PUBLIC
LRESULT CALLBACK ListViewNoDividerDblClickSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__ */
