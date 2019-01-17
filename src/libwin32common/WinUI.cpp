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

#include "WinUI.hpp"
#include "AutoGetDC.hpp"
#include <commctrl.h>

// C++ includes.
#include <memory>
#include <string>
#include <unordered_set>
using std::unique_ptr;
using std::unordered_set;
using std::tstring;

namespace LibWin32Common {

/**
 * Convert UNIX line endings to DOS line endings.
 * TODO: Move to RpWin32?
 * @param tstr_unix	[in] String with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return tstring with DOS line endings.
 */
tstring unix2dos(const TCHAR *tstr_unix, int *lf_count)
{
	// TODO: Optimize this!
	tstring tstr_dos;
	tstr_dos.reserve(_tcslen(tstr_unix));
	int lf = 0;
	for (; *tstr_unix != _T('\0'); tstr_unix++) {
		if (*tstr_unix == _T('\n')) {
			tstr_dos += _T("\r\n");
			lf++;
		} else {
			tstr_dos += *tstr_unix;
		}
	}
	if (lf_count) {
		*lf_count = lf;
	}
	return tstr_dos;
}

/**
 * Measure text size using GDI.
 * @param hWnd		[in] hWnd.
 * @param hFont		[in] Font.
 * @param tstr		[in] String.
 * @param lpSize	[out] Size.
 * @return 0 on success; non-zero on errro.
 */
int measureTextSize(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize)
{
	assert(hWnd != nullptr);
	assert(hFont != nullptr);
	assert(tstr != nullptr);
	assert(lpSize != nullptr);
	if (!hWnd || !hFont || !tstr || !lpSize)
		return -EINVAL;

	SIZE size_total = {0, 0};
	AutoGetDC hDC(hWnd, hFont);

	// Find the next newline.
	do {
		const TCHAR *p_nl = _tcschr(tstr, L'\n');
		int len;
		if (p_nl) {
			// Found a newline.
			len = static_cast<int>(p_nl - tstr);
		} else {
			// No newline. Process the rest of the string.
			len = static_cast<int>(_tcslen(tstr));
		}
		assert(len >= 0);
		if (len < 0) {
			len = 0;
		}

		// Check if a '\r' is present before the '\n'.
		if (len > 0 && p_nl != nullptr && *(p_nl-1) == L'\r') {
			// Ignore the '\r'.
			len--;
		}

		// Measure the text size.
		SIZE size_cur;
		BOOL bRet = GetTextExtentPoint32(hDC, tstr, len, &size_cur);
		if (!bRet) {
			// Something failed...
			return -1;
		}

		if (size_cur.cx > size_total.cx) {
			size_total.cx = size_cur.cx;
		}
		size_total.cy += size_cur.cy;

		// Next newline.
		if (!p_nl)
			break;
		tstr = p_nl + 1;
	} while (*tstr != 0);

	*lpSize = size_total;
	return 0;
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
int measureTextSizeLink(HWND hWnd, HFONT hFont, const TCHAR *tstr, LPSIZE lpSize)
{
	assert(tstr != nullptr);
	if (!tstr)
		return -EINVAL;

	// Remove HTML-style tags.
	// NOTE: This is a very simplistic version.
	size_t len = _tcslen(tstr);
	unique_ptr<TCHAR[]> ntstr(new TCHAR[len+1]);
	TCHAR *p = ntstr.get();

	int lbrackets = 0;
	for (; *tstr != 0; tstr++) {
		if (*tstr == _T('<')) {
			// Starting bracket.
			lbrackets++;
			continue;
		} else if (*tstr == _T('>')) {
			// Ending bracket.
			assert(lbrackets > 0);
			lbrackets--;
			continue;
		}

		if (lbrackets == 0) {
			// Not currently in a tag.
			*p++ = *tstr;
		}
	}

	*p = 0;
	return measureTextSize(hWnd, hFont, ntstr.get(), lpSize);
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
	unordered_set<tstring> *const pFonts = reinterpret_cast<unordered_set<tstring>*>(lParam);

	// Check the font attributes:
	// - Must be monospaced.
	// - Must be horizontally-oriented.
	if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) &&
	     lpelfe->lfFaceName[0] != _T('@'))
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
	unordered_set<tstring> enum_fonts;
	// FIXME: Use a CMake check like in librpbase.
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
	static const TCHAR *const mono_font_names[] = {
		_T("DejaVu Sans Mono"),
		_T("Consolas"),
		_T("Lucida Console"),
		_T("Fixedsys Excelsior 3.01"),
		_T("Fixedsys Excelsior 3.00"),
		_T("Fixedsys Excelsior 3.0"),
		_T("Fixedsys Excelsior 2.00"),
		_T("Fixedsys Excelsior 2.0"),
		_T("Fixedsys Excelsior 1.00"),
		_T("Fixedsys Excelsior 1.0"),
		_T("Fixedsys"),
		_T("Courier New"),
		nullptr
	};

	const TCHAR *mono_font = nullptr;
	for (const TCHAR *const *test_font = mono_font_names; *test_font != nullptr; test_font++) {
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
	_tcscpy_s(plfFontMono->lfFaceName, _countof(plfFontMono->lfFaceName), mono_font);
	return 0;
}

/**
 * Get the alternate row color for ListViews.
 *
 * This function should be called on ListView creation
 * and if the system theme is changed.
 *
 * @return Alternate row color for ListViews.
 */
COLORREF getAltRowColor(void)
{
	union {
		COLORREF color;
		struct {
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
	} rgb;
	rgb.color = GetSysColor(COLOR_WINDOW);

	// TODO: Better "convert to grayscale" and brighten/darken algorithms?
	if (((rgb.r + rgb.g + rgb.b) / 3) >= 128) {
		// Subtract 16 from each color component.
		rgb.r -= 16;
		rgb.g -= 16;
		rgb.b -= 16;
	} else {
		// Add 16 to each color component.
		rgb.r += 16;
		rgb.g += 16;
		rgb.b += 16;
	}

	return rgb.color;
}

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
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_KEYDOWN: {
			// Work around Enter/Escape issues.
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			if (!dwRefData) {
				// No parent dialog...
				break;
			}
			HWND hDlg = reinterpret_cast<HWND>(dwRefData);

			switch (wParam) {
				case VK_RETURN:
					SendMessage(hDlg, WM_COMMAND, IDOK, 0);
					return TRUE;

				case VK_ESCAPE:
					SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
					return TRUE;

				default:
					break;
			}
			break;
		}

		case WM_GETDLGCODE: {
			// Filter out DLGC_HASSETSEL.
			// References:
			// - https://stackoverflow.com/questions/20876045/cricheditctrl-selects-all-text-when-it-gets-focus
			// - https://stackoverflow.com/a/20884852
			const DWORD code = static_cast<DWORD>(DefSubclassProc(hWnd, uMsg, wParam, lParam));
			return (code & ~DLGC_HASSETSEL);
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MultiLineEditProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

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
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_GETDLGCODE: {
			// Filter out DLGC_HASSETSEL.
			// References:
			// - https://stackoverflow.com/questions/20876045/cricheditctrl-selects-all-text-when-it-gets-focus
			// - https://stackoverflow.com/a/20884852
			const DWORD code = static_cast<DWORD>(DefSubclassProc(hWnd, uMsg, wParam, lParam));
			return (code & ~DLGC_HASSETSEL);
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MultiLineEditProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

}
