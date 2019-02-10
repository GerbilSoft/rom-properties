/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * WinUI.hpp: Windows UI common functions.                                 *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
 * @param tstr_unix	[in] String with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return tstring with DOS line endings.
 */
std::tstring unix2dos(const TCHAR *tstr_unix, int *lf_count = nullptr);

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on error.
 */
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
