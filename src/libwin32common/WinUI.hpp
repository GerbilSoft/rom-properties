/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * WinUI.hpp: Windows UI common functions.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__
#define __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__

// C++ includes.
#include <string>

// Windows SDK.
#include "RpWin32_sdk.h"

namespace LibWin32Common {

/**
 * Convert UNIX line endings to DOS line endings.
 * @param wstr_unix	[in] Wide string with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return wstring with DOS line endings.
 */
std::wstring unix2dos(const wchar_t *wstr_unix, int *lf_count = nullptr);

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
int measureTextSize(HWND hWnd, HFONT hFont, const wchar_t *wstr, LPSIZE lpSize);

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
static inline int measureTextSize(HWND hWnd, HFONT hFont, const std::wstring &wstr, LPSIZE lpSize)
{
	return measureTextSize(hWnd, hFont, wstr.c_str(), lpSize);
}

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
int measureTextSizeLink(HWND hWnd, HFONT hFont, const wchar_t *wstr, LPSIZE lpSize);

/**
 * Measure text size using GDI.
 * This version removes HTML-style tags before
 * calling the regular measureTextSize() function.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
static inline int measureTextSizeLink(HWND hWnd, HFONT hFont, const std::wstring &wstr, LPSIZE lpSize)
{
	return measureTextSizeLink(hWnd, hFont, wstr.c_str(), lpSize);
}

/**
 * Determine the monospaced font to use.
 * @param plfFontMono Pointer to LOGFONT to store the font name in.
 * @return 0 on success; negative POSIX error code on error.
 */
int findMonospacedFont(LOGFONT *plfFontMono);

/**
 * Get the alternate row color for ListViews.
 *
 * This function should be called on ListView creation
 * and if the system theme is changed.
 *
 * @return Alternate row color for ListViews.
 */
COLORREF getAltRowColor(void);

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
LRESULT CALLBACK SingleLineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

}

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_WINUICOMMON_HPP__ */
