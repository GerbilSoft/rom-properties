/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
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

#include "stdafx.h"
#include "WinUI.hpp"
#include "AutoGetDC.hpp"

// C++ includes.
#include <string>
#include <unordered_set>
using std::unordered_set;
using std::wstring;

namespace WinUI {

/**
 * Convert UNIX line endings to DOS line endings.
 * TODO: Move to RpWin32?
 * @param wstr_unix	[in] wstring with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return wstring with DOS line endings.
 */
wstring unix2dos(const wstring &wstr_unix, int *lf_count)
{
	// TODO: Optimize this!
	wstring wstr_dos;
	wstr_dos.reserve(wstr_unix.size());
	int lf = 0;
	for (size_t i = 0; i < wstr_unix.size(); i++) {
		if (wstr_unix[i] == L'\n') {
			wstr_dos += L"\r\n";
			lf++;
		} else {
			wstr_dos += wstr_unix[i];
		}
	}
	if (lf_count) {
		*lf_count = lf;
	}
	return wstr_dos;
}

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param wstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on errro.
 */
int measureTextSize(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
{
	SIZE size_total = {0, 0};
	AutoGetDC hDC(hWnd, hFont);

	// Handle newlines.
	const wchar_t *data = wstr.data();
	int nl_pos_prev = -1;
	size_t nl_pos = 0;	// Assuming no NL at the start.
	do {
		nl_pos = wstr.find(L'\n', nl_pos + 1);
		const int start = nl_pos_prev + 1;
		int len;
		if (nl_pos != wstring::npos) {
			len = (int)(nl_pos - start);
		} else {
			len = (int)(wstr.size() - start);
		}

		// Check if a '\r' is present before the '\n'.
		if (data[nl_pos - 1] == L'\r') {
			// Ignore the '\r'.
			len--;
		}

		SIZE size_cur;
		BOOL bRet = GetTextExtentPoint32(hDC, &data[start], len, &size_cur);
		if (!bRet) {
			// Something failed...
			return -1;
		}

		if (size_cur.cx > size_total.cx) {
			size_total.cx = size_cur.cx;
		}
		size_total.cy += size_cur.cy;

		// Next newline.
		nl_pos_prev = (int)nl_pos;
	} while (nl_pos != wstring::npos);

	*lpSize = size_total;
	return 0;
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
int measureTextSizeLink(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
{
	// Remove HTML-style tags.
	// NOTE: This is a very simplistic version.
	wstring nwstr;
	nwstr.reserve(wstr.size());

	int lbrackets = 0;
	for (int i = 0; i < (int)wstr.size(); i++) {
		if (wstr[i] == L'<') {
			// Starting bracket.
			lbrackets++;
			continue;
		} else if (wstr[i] == L'>') {
			// Ending bracket.
			assert(lbrackets > 0);
			lbrackets--;
			continue;
		}

		if (lbrackets == 0) {
			// Not currently in a tag.
			nwstr += wstr[i];
		}
	}

	return measureTextSize(hWnd, hFont, nwstr, lpSize);
}

/**
 * Monospaced font enumeration procedure.
 * @param lpelfe Enumerated font information.
 * @param lpntme Font metrics.
 * @param FontType Font type.
 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
 */
static int CALLBACK MonospacedFontEnumProc(
	const LOGFONT *lpelfe, const TEXTMETRIC *lpntme,
	DWORD FontType, LPARAM lParam)
{
	unordered_set<wstring> *pFonts = reinterpret_cast<unordered_set<wstring>*>(lParam);

	// Check the font attributes:
	// - Must be monospaced.
	// - Must be horizontally-oriented.
	if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) &&
	     lpelfe->lfFaceName[0] != '@')
	{
		pFonts->insert(lpelfe->lfFaceName);
	}

	// Continue enumeration.
	return 1;
}

/**
 * Determine the monospaced font to use.
 * @param plfFontMono Pointer to LOGFONT to store the font name in.
 * @return 0 on success; negative POSIX error code on error.
 */
int findMonospacedFont(LOGFONT *plfFontMono)
{
	// Enumerate all monospaced fonts.
	// Reference: http://www.catch22.net/tuts/fixed-width-font-enumeration
	unordered_set<wstring> enum_fonts;
#if !defined(_MSC_VER) || _MSC_VER >= 1700
	enum_fonts.reserve(64);
#endif

	LOGFONT lfEnumFonts = { 0 };
	lfEnumFonts.lfCharSet = DEFAULT_CHARSET;
	lfEnumFonts.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
	HDC hdc = GetDC(nullptr);
	EnumFontFamiliesEx(hdc, &lfEnumFonts, MonospacedFontEnumProc,
		reinterpret_cast<LPARAM>(&enum_fonts), 0);
	ReleaseDC(nullptr, hdc);

	if (enum_fonts.empty()) {
		// No fonts...
		return -ENOENT;
	}

	// Fonts to try.
	static const wchar_t *const mono_font_names[] = {
		L"DejaVu Sans Mono",
		L"Consolas",
		L"Lucida Console",
		L"Fixedsys Excelsior 3.01",
		L"Fixedsys Excelsior 3.00",
		L"Fixedsys Excelsior 3.0",
		L"Fixedsys Excelsior 2.00",
		L"Fixedsys Excelsior 2.0",
		L"Fixedsys Excelsior 1.00",
		L"Fixedsys Excelsior 1.0",
		L"Fixedsys",
		L"Courier New",
		nullptr
	};

	const wchar_t *mono_font = nullptr;
	for (const wchar_t *const *test_font = mono_font_names; *test_font != nullptr; test_font++) {
		if (enum_fonts.find(*test_font) != enum_fonts.end()) {
			// Found a font.
			mono_font = *test_font;
			break;
		}
	}

	if (!mono_font) {
		// No monospaced font found.
		return -ENOENT;
	}

	// Found the monospaced font.
	wcscpy_s(plfFontMono->lfFaceName, _countof(plfFontMono->lfFaceName), mono_font);
	return 0;
}

}
