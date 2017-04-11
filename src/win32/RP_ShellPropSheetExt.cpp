/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.cpp: IShellPropSheetExt implementation.            *
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

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"
#include "RpImageWin32.hpp"
#include "AutoGetDC.hpp"
#include "resource.h"

// libromdata
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/RomData.hpp"
#include "libromdata/RomFields.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/RpWin32.hpp"
using namespace LibRomData;

// C includes. (C++ namespace)
#include <cassert>
#include <cctype>
#include <cstring>

// C++ includes.
#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
using std::unique_ptr;
using std::unordered_set;
using std::wostringstream;
using std::wstring;
using std::vector;

// Gdiplus for image drawing.
// NOTE: Gdiplus requires min/max.
#include <algorithm>
namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>
#include "libromdata/img/GdiplusHelper.hpp"
#include "libromdata/img/RpGdiplusBackend.hpp"
#include "libromdata/img/IconAnimData.hpp"
#include "libromdata/img/IconAnimHelper.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// IDC_STATIC might not be defined.
#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

// Control base IDs.
#define IDC_STATIC_BANNER		0x0100
#define IDC_STATIC_ICON			0x0101
#define IDC_TAB_WIDGET			0x0102
#define IDC_TAB_PAGE(idx)		(0x0200 + (idx))
#define IDC_STATIC_DESC(idx)		(0x1000 + (idx))
#define IDC_RFT_STRING(idx)		(0x1400 + (idx))
#define IDC_RFT_LISTDATA(idx)		(0x1800 + (idx))
// Date/Time acts like a string widget internally.
#define IDC_RFT_DATETIME(idx)		IDC_RFT_STRING(idx)

// Bitfield is last due to multiple controls per field.
#define IDC_RFT_BITFIELD(idx, bit)	(0x7000 + ((idx) * 32) + (bit))

// Property for "external pointer".
// This links the property sheet to the COM object.
#define EXT_POINTER_PROP L"RP_ShellPropSheetExt"

/** RP_ShellPropSheetExt_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellPropSheetExtPrivate RP_ShellPropSheetExt_Private

class RP_ShellPropSheetExt_Private
{
	public:
		explicit RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q);
		~RP_ShellPropSheetExt_Private();

	private:
		RP_DISABLE_COPY(RP_ShellPropSheetExt_Private)
	private:
		RP_ShellPropSheetExt *const q_ptr;

	public:
		// ROM data.
		LibRomData::RomData *romData;

		// Useful window handles.
		HWND hDlgProps;		// Property dialog. (parent)
		HWND hDlgSheet;		// Property sheet.

		// Fonts.
		HFONT hFontDlg;		// Main dialog font.
		HFONT hFontBold;	// Bold font.
		HFONT hFontMono;	// Monospaced font.

		// Monospaced font details.
		LOGFONT lfFontMono;
		unordered_set<wstring> monospaced_fonts;
		vector<HWND> hwndMonoControls;			// Controls using the monospaced font.
		bool bPrevIsClearType;	// Previous ClearType setting.

		// Controls that need to be drawn using red text.
		unordered_set<HWND> hwndWarningControls;	// Controls using the "Warning" font.
		// SysLink controls.
		unordered_set<HWND> hwndSysLinkControls;

		// GDI+ token.
		ScopedGdiplus gdipScope;

		// Header row widgets.
		HWND lblSysInfo;

		// Window background color.
		COLORREF colorWinBg;
		// XP theming.
		typedef BOOL (STDAPICALLTYPE* PFNISTHEMEACTIVE)(void);
		HMODULE hUxTheme_dll;
		PFNISTHEMEACTIVE pfnIsThemeActive;

		// Banner.
		HBITMAP hbmpBanner;
		POINT ptBanner;
		SIZE szBanner;

		// Tab layout.
		HWND hTabWidget;
		struct tab {
			HWND hDlg;		// Tab child dialog.
			POINT curPt;		// Current point.
		};
		vector<tab> tabs;
		int curTabIndex;

		// Animated icon data.
		const IconAnimData *iconAnimData;
		std::array<HBITMAP, IconAnimData::MAX_FRAMES> hbmpIconFrames;
		RECT rectIcon;
		SIZE szIcon;
		IconAnimHelper iconAnimHelper;
		UINT_PTR animTimerID;		// Animation timer ID. (non-zero == running)
		int last_frame_number;		// Last frame number.

		/**
		 * Start the animation timer.
		 */
		void startAnimTimer(void);

		/**
		 * Stop the animation timer.
		 */
		void stopAnimTimer(void);

	private:
		/**
		 * Convert UNIX line endings to DOS line endings.
		 * TODO: Move to RpWin32?
		 * @param wstr_unix	[in] wstring with UNIX line endings.
		 * @param lf_count	[out,opt] Number of LF characters found.
		 * @return wstring with DOS line endings.
		 */
		static inline wstring unix2dos(const wstring &wstr_unix, int *lf_count);

		/**
		 * Measure text size using GDI.
		 * @param hWnd		[in] hWnd.
		 * @param hFont		[in] Font.
		 * @param str		[in] String.
		 * @param lpSize	[out] Size.
		 * @return 0 on success; non-zero on error.
		 */
		static int measureTextSize(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize);

		/**
		 * Measure text size using GDI.
		 * This version removes HTML-style tags before
		 * calling the regular measureTextSize() function.
		 * @param hWnd		[in] hWnd.
		 * @param hFont		[in] Font.
		 * @param str		[in] String.
		 * @param lpSize	[out] Size.
		 * @return 0 on success; non-zero on error.
		 */
		static int measureTextSizeLink(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize);

	public:
		/**
		 * Load the banner and icon as HBITMAPs.
		 *
		 * This function should bee called on startup and if
		 * the window's background color changes.
		 *
		 * @param hDlg	[in] Dialog window.
		 */
		void loadImages(HWND hDlg);

	private:
		/**
		 * Increase an image's size to the minimum size, if necesary.
		 * @param sz Image size.
		 */
		void incSizeToMinimum(SIZE &sz);

		/**
		 * Create the header row.
		 * @param hDlg		[in] Dialog window.
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a full-width single line label.
		 * @return Row height, in pixels.
		 */
		int createHeaderRow(HWND hDlg, const POINT &pt_start, const SIZE &size);

		/**
		 * Initialize a string field. (Also used for Date/Time.)
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param wcs		[in,opt] String data. (If nullptr, field data is used.)
		 * @return Field height, in pixels.
		 */
		int initString(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size,
			const RomFields::Field *field, LPCWSTR wcs);

		/**
		 * Initialize a bitfield layout.
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param field		[in] RomFields::Field
		 * @return Field height, in pixels.
		 */
		int initBitfield(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx,
			const RomFields::Field *field);

		/**
		 * Initialize a ListView control.
		 * @param hWnd		[in] HWND of the ListView control.
		 * @param field		[in] RomFields::Field
		 */
		void initListView(HWND hWnd, const RomFields::Field *field);

		/**
		 * Initialize a Date/Time field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @return Field height, in pixels.
		 */
		int initDateTime(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size,
			const RomFields::Field *field);

		/**
		 * Initialize an Age Ratings field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @return Field height, in pixels.
		 */
		int initAgeRatings(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size,
			const RomFields::Field *field);

		/**
		 * Initialize the bold font.
		 * @param hFont Base font.
		 */
		void initBoldFont(HFONT hFont);

		/**
		 * Monospaced font enumeration procedure.
		 * @param lpelfe Enumerated font information.
		 * @param lpntme Font metrics.
		 * @param FontType Font type.
		 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
		 */
		static int CALLBACK MonospacedFontEnumProc(
			const LOGFONT *lpelfe, const TEXTMETRIC *lpntme,
			DWORD FontType, LPARAM lParam);

	public:
		/**
		 * Initialize the monospaced font.
		 * @param hFont Base font.
		 */
		void initMonospacedFont(HFONT hFont);

		/**
		 * Initialize the dialog.
		 * Called by WM_INITDIALOG.
		 * @param hDlg Dialog window.
		 */
		void initDialog(HWND hDlg);
};

/** RP_ShellPropSheetExt_Private **/

RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q)
	: q_ptr(q)
	, romData(nullptr)
	, hDlgProps(nullptr)
	, hDlgSheet(nullptr)
	, hFontBold(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(nullptr)
	, lblSysInfo(nullptr)
	, colorWinBg(0)
	, hUxTheme_dll(nullptr)
	, pfnIsThemeActive(nullptr)
	, hbmpBanner(nullptr)
	, hTabWidget(nullptr)
	, curTabIndex(0)
	, iconAnimData(nullptr)
	, animTimerID(0)
	, last_frame_number(0)
{
	memset(&lfFontMono, 0, sizeof(lfFontMono));
	hbmpIconFrames.fill(nullptr);
	memset(&ptBanner, 0, sizeof(ptBanner));
	memset(&rectIcon, 0, sizeof(rectIcon));
	memset(&szIcon, 0, sizeof(szIcon));

	// Attempt to get IsThemeActive() from uxtheme.dll.
	// TODO: Move this to RP_ComBase so we don't have to look it up
	// every time the property dialog is loaded?
	hUxTheme_dll = LoadLibrary(L"uxtheme.dll");
	if (hUxTheme_dll) {
		pfnIsThemeActive = (PFNISTHEMEACTIVE)GetProcAddress(hUxTheme_dll, "IsThemeActive");
	}
}

RP_ShellPropSheetExt_Private::~RP_ShellPropSheetExt_Private()
{
	stopAnimTimer();
	iconAnimHelper.setIconAnimData(nullptr);
	if (romData) {
		romData->unref();
	}

	// Delete the banner and icon frames.
	if (hbmpBanner) {
		DeleteObject(hbmpBanner);
	}
	for (int i = (int)(hbmpIconFrames.size())-1; i >= 0; i--) {
		if (hbmpIconFrames[i]) {
			DeleteObject(hbmpIconFrames[i]);
		}
	}

	// Delete the fonts.
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}

	// Close uxtheme.dll.
	if (hUxTheme_dll) {
		FreeLibrary(hUxTheme_dll);
	}
}

/**
 * Start the animation timer.
 */
void RP_ShellPropSheetExt_Private::startAnimTimer(void)
{
	if (!iconAnimData) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'd' pointer as nIDEvent.
	animTimerID = SetTimer(hDlgSheet,
		reinterpret_cast<UINT_PTR>(this),
		delay,
		RP_ShellPropSheetExt::AnimTimerProc);
}

/**
 * Stop the animation timer.
 */
void RP_ShellPropSheetExt_Private::stopAnimTimer(void)
{
	if (animTimerID) {
		KillTimer(hDlgSheet, animTimerID);
		animTimerID = 0;
	}
}

/**
 * Convert UNIX line endings to DOS line endings.
 * TODO: Move to RpWin32?
 * @param wstr_unix	[in] wstring with UNIX line endings.
 * @param lf_count	[out,opt] Number of LF characters found.
 * @return wstring with DOS line endings.
 */
inline wstring RP_ShellPropSheetExt_Private::unix2dos(const wstring &wstr_unix, int *lf_count)
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
int RP_ShellPropSheetExt_Private::measureTextSize(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
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
int RP_ShellPropSheetExt_Private::measureTextSizeLink(HWND hWnd, HFONT hFont, const wstring &wstr, LPSIZE lpSize)
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
 * Load the banner and icon as HBITMAPs.
 *
 * This function should be called on startup and if
 * the window's background color changes.
 *
 * @param hDlg	[in] Dialog window.
 */
void RP_ShellPropSheetExt_Private::loadImages(HWND hDlg)
{
	// Window background color.
	// Static controls don't support alpha transparency (?? test),
	// so we have to fake it.
	// NOTE: GetSysColor() has swapped R and B channels
	// compared to GDI+.
	// TODO: Get the actual background color of the window.
	// TODO: Use DrawThemeBackground:
	// - http://www.codeproject.com/Articles/5978/Correctly-drawn-themed-dialogs-in-WinXP
	// - https://blogs.msdn.microsoft.com/dsui_team/2013/06/26/using-theme-apis-to-draw-the-border-of-a-control/
	// - https://blogs.msdn.microsoft.com/pareshj/2011/11/03/draw-the-background-of-static-control-with-gradient-fill-when-theme-is-enabled/
	if (pfnIsThemeActive && pfnIsThemeActive()) {
		// Theme is active.
		colorWinBg = GetSysColor(COLOR_WINDOW);
	} else {
		// Theme is not active.
		colorWinBg = GetSysColor(COLOR_3DFACE);
	}
	Gdiplus::ARGB gdipBgColor =
		   (colorWinBg & 0x00FF00) | 0xFF000000 |
		  ((colorWinBg & 0xFF) << 16) |
		  ((colorWinBg >> 16) & 0xFF);

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Delete the old banner.
		if (hbmpBanner != nullptr) {
			DeleteObject(hbmpBanner);
			hbmpBanner = nullptr;
		}

		// Get the banner.
		const rp_image *banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			// Convert to HBITMAP using the window background color.
			// TODO: Redo if the window background color changes.
			hbmpBanner = RpImageWin32::toHBITMAP(banner, gdipBgColor, szBanner, true);
		}
	}

	// Icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Delete the old icons.
		for (int i = (int)(hbmpIconFrames.size())-1; i >= 0; i--) {
			if (hbmpIconFrames[i]) {
				DeleteObject(hbmpIconFrames[i]);
				hbmpIconFrames[i] = nullptr;
			}
		}

		// Get the icon.
		const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Get the animated icon data.
			if (iconAnimData) {
				// Convert the icons to GDI+ bitmaps.
				// TODO: Refactor this a bit...
				for (int i = 0; i < iconAnimData->count; i++) {
					if (iconAnimData->frames[i] && iconAnimData->frames[i]->isValid()) {
						// Convert to HBITMAP using the window background color.
						// TODO: Redo if the window background color changes.
						hbmpIconFrames[i] = RpImageWin32::toHBITMAP(iconAnimData->frames[i], gdipBgColor, szIcon, true);
					}
				}

				// Set up the IconAnimHelper.
				iconAnimHelper.setIconAnimData(iconAnimData);
				last_frame_number = iconAnimHelper.frameNumber();

				// Icon animation timer is set in startAnimTimer().
			} else {
				// Not an animated icon.
				last_frame_number = 0;

				// Convert to HBITMAP using the window background color.
				// TODO: Redo if the window background color changes.
				hbmpIconFrames[0] = RpImageWin32::toHBITMAP(icon, gdipBgColor, szIcon, true);
			}
		}
	}
}

/**
 * Increase an image's size to the minimum size, if necesary.
 * @param sz Image size.
 */
void RP_ShellPropSheetExt_Private::incSizeToMinimum(SIZE &sz)
{
	// Minimum image size.
	// If images are smaller, they will be resized.
	// TODO: Adjust minimum size for DPI.
	static const SIZE min_img_size = {32, 32};

	if (sz.cx >= min_img_size.cx && sz.cx >= min_img_size.cy) {
		// No resize necessary.
		return;
	}

	// Resize the image.
	SIZE orig_sz = sz;
	do {
		// Increase by integer multiples until
		// the icon is at least 32x32.
		// TODO: Constrain to 32x32?
		sz.cx += orig_sz.cx;
		sz.cy += orig_sz.cy;
	} while (sz.cx < min_img_size.cx && sz.cy < min_img_size.cy);
}

/**
 * Create the header row.
 * @param hDlg		[in] Dialog window.
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a full-width single line label.
 * @return Row height, in pixels.
 */
int RP_ShellPropSheetExt_Private::createHeaderRow(HWND hDlg, const POINT &pt_start, const SIZE &size)
{
	if (!hDlg)
		return 0;

	// Total widget width.
	int total_widget_width = 0;

	// System name.
	// TODO: Logo, game icon, and game title?
	const rp_char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);

	// File type.
	const rp_char *const fileType = romData->fileType_string();

	wstring sysInfo;
	if (systemName) {
		sysInfo = RP2W_c(systemName);
	}
	if (fileType) {
		if (!sysInfo.empty()) {
			sysInfo += L"\r\n";
		}
		sysInfo += RP2W_c(fileType);
	}

	// Label size.
	SIZE sz_lblSysInfo = {0, 0};

	// Font to use.
	// TODO: Handle these assertions in release builds.
	assert(hFontBold != nullptr);
	assert(hFontDlg != nullptr);
	const HFONT hFont = (hFontBold ? hFontBold : hFontDlg);

	if (!sysInfo.empty()) {
		// Determine the appropriate label size.
		int ret = measureTextSize(hDlg, hFont, sysInfo, &sz_lblSysInfo);
		if (ret != 0) {
			// Error determining the label size.
			// Don't draw the label.
			sz_lblSysInfo.cx = 0;
			sz_lblSysInfo.cy = 0;
		} else {
			// Start the total_widget_width.
			total_widget_width = sz_lblSysInfo.cx;
		}
	}

	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	const rp_image *banner = nullptr;
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			szBanner.cx = banner->width();
			szBanner.cy = banner->height();
			incSizeToMinimum(szBanner);

			// Add the banner width.
			// The banner will be assigned to a WC_STATIC control.
			if (total_widget_width > 0) {
				total_widget_width += pt_start.x;
			}
			total_widget_width += szBanner.cx;
		} else {
			// No banner.
			banner = nullptr;
		}
	}

	// Icon.
	const rp_image *icon = nullptr;
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			szIcon.cx = icon->width();
			szIcon.cy = icon->height();
			incSizeToMinimum(szIcon);

			// Add the icon width.
			// The icon will be assigned to a WC_STATIC control.
			if (total_widget_width > 0) {
				total_widget_width += pt_start.x;
			}
			total_widget_width += szIcon.cx;

			// Get the animated icon data.
			iconAnimData = romData->iconAnimData();
		} else {
			// No icon.
			icon = nullptr;
		}
	}

	// Starting point.
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (sz_lblSysInfo.cx > 0 && sz_lblSysInfo.cy > 0) {
		lblSysInfo = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, sysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			curPt.x, curPt.y,
			sz_lblSysInfo.cx, sz_lblSysInfo.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysInfo, hFont, FALSE);
		curPt.x += sz_lblSysInfo.cx + pt_start.x;
	}

	// lblBanner
	if (banner) {
		ptBanner = curPt;
		curPt.x += szBanner.cx + pt_start.x;
	}

	// lblIcon
	if (icon) {
		SetRect(&rectIcon, curPt.x, curPt.y,
			curPt.x + szIcon.cx, curPt.y + szIcon.cy);
		curPt.x += szIcon.cx + pt_start.x;
	}

	// Load the images.
	loadImages(hDlg);

	// Return the label height and some extra padding.
	// TODO: Icon/banner height?
	return sz_lblSysInfo.cy + (pt_start.y * 5 / 8);
}

/**
 * Initialize a string field. (Also used for Date/Time.)
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param wcs		[in,opt] String data. (If nullptr, field data is used.)
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initString(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size,
	const RomFields::Field *field, LPCWSTR wcs)
{
	if (!hDlg || !hWndTab || !field)
		return 0;

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Field object.
	int lf_count = 0;
	wstring wstr;
	if (!wcs) {
		if (field->type != RomFields::RFT_STRING)
			return 0;

		// TODO: NULL string == empty string?
		if (field->data.str) {
			wstr = unix2dos(RP2W_s(*(field->data.str)), &lf_count);
		}
	} else {
		// Use the specified string.
		wstr = unix2dos(wstring(wcs), &lf_count);
	}

	// Field height.
	int field_cy = size.cy;
	if (lf_count > 0) {
		// Multiple lines.
		// NOTE: Only add 5/8 of field_cy per line.
		// FIXME: 5/8 needs adjustment...
		field_cy += (field_cy * lf_count) * 5 / 8;
	}

	// Dialog item.
	const HMENU cId = (HMENU)(INT_PTR)(IDC_RFT_STRING(idx));
	HWND hDlgItem;

	// Get the default font.
	HFONT hFont = hFontDlg;

	// Check for any formatting options.
	bool isWarning = false, isMonospace = false;
	if (field->type == RomFields::RFT_STRING) {
		// FIXME: STRF_MONOSPACE | STRF_WARNING is not supported.
		// Preferring STRF_WARNING.
		assert((field->desc.flags &
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING)) !=
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING));

		if (field->desc.flags & RomFields::STRF_WARNING) {
			// "Warning" font.
			if (hFontBold) {
				hFont = hFontBold;
				isWarning = true;
				// Set the font of the description control.
				HWND hStatic = GetDlgItem(hWndTab, IDC_STATIC_DESC(idx));
				if (hStatic) {
					SetWindowFont(hStatic, hFont, FALSE);
					hwndWarningControls.insert(hStatic);
				}
			}
		} else if (field->desc.flags & RomFields::STRF_MONOSPACE) {
			// Monospaced font.
			if (hFontMono) {
				hFont = hFontMono;
				isMonospace = true;
			}
		}
	}

	if (field->type == RomFields::RFT_STRING &&
	    (field->desc.flags & RomFields::STRF_CREDITS))
	{
		// Align to the bottom of the dialog and center-align the text.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT tmpRect = {7, 7, 8, 8};
		MapDialogRect(hWndTab, &tmpRect);
		RECT winRect;
		GetClientRect(hWndTab, &winRect);

		// Create a SysLink widget.
		// SysLink allows the user to click a link and
		// open a webpage. It does NOT allow highlighting.
		// TODO: SysLink + EDIT?
		// FIXME: Centered text alignment?
		// TODO: With subtabs:
		// - Verify behavior of LWS_TRANSPARENT.
		// - Show below subtabs.
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_LINK, wstr.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			0, 0, 0, 0,	// will be adjusted afterwards
			hWndTab, cId, nullptr, nullptr);
		// There should be a maximum of one STRF_CREDITS per RomData subclass.
		assert(hwndSysLinkControls.empty());
		hwndSysLinkControls.insert(hDlgItem);
		SetWindowFont(hDlgItem, hFont, FALSE);

		// NOTE: We can't use measureTextSize() because that includes
		// the HTML markup, and LM_GETIDEALSIZE is Vista+ only.
		// Use a wrapper measureTextSizeLink() that removes HTML-like
		// tags and then calls measureTextSize().
		SIZE szText;
		measureTextSizeLink(hWndTab, hFont, wstr, &szText);

		// Determine the position.
		const int x = (((winRect.right - winRect.left) - szText.cx) / 2) + winRect.left;
		const int y = winRect.bottom - tmpRect.top - szText.cy;
		// Set the position and size.
		SetWindowPos(hDlgItem, 0, x, y, szText.cx, szText.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Clear field_cy so the description widget won't show up
		// and the "normal" area will be empty.
		field_cy = 0;
	} else {
		// Create a read-only EDIT widget.
		// The STATIC control doesn't allow the user
		// to highlight and copy data.
		DWORD dwStyle;
		if (lf_count > 0) {
			// Multiple lines.
			dwStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | ES_READONLY | ES_AUTOHSCROLL | ES_MULTILINE;
		} else {
			// Single line.
			dwStyle = WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | ES_READONLY | ES_AUTOHSCROLL;
		}
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_EDIT, wstr.c_str(), dwStyle,
			pt_start.x, pt_start.y,
			size.cx, field_cy,
			hWndTab, cId, nullptr, nullptr);
		SetWindowFont(hDlgItem, hFont, FALSE);

		// Get the EDIT control margins.
		const DWORD dwMargins = (DWORD)SendMessage(hDlgItem, EM_GETMARGINS, 0, 0);

		// Adjust the window size to compensate for the margins not being clickable.
		// NOTE: Not adjusting the right margins.
		SetWindowPos(hDlgItem, nullptr, pt_start.x - LOWORD(dwMargins), pt_start.y,
			size.cx + LOWORD(dwMargins), field_cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Subclass multi-line EDIT controls to work around Enter/Escape issues.
		// Reference:  http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
		if (dwStyle & ES_MULTILINE) {
			// Store the object pointer so we can reference it later.
			SetProp(hDlgItem, EXT_POINTER_PROP, static_cast<HANDLE>(q_ptr));

			// Subclass the control.
			// TODO: Error handling?
			SetWindowSubclass(hDlgItem,
				RP_ShellPropSheetExt::MultilineEditProc,
				(UINT_PTR)cId, (DWORD_PTR)q_ptr);
		}
	}

	// Save the control in the appropriate set, if necessary.
	if (isWarning) {
		hwndWarningControls.insert(hDlgItem);
	}
	if (isMonospace) {
		hwndMonoControls.push_back(hDlgItem);
	}

	return field_cy;
}

/**
 * Initialize a bitfield layout.
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param field		[in] RomFields::Field
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initBitfield(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx,
	const RomFields::Field *field)
{
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_BITFIELD)
		return 0;

	// Checkbox size.
	// Reference: http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box
	RECT rect_chkbox = {0, 0, 12+4, 11};
	MapDialogRect(hDlg, &rect_chkbox);

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	AutoGetDC hDC(hWndTab, hFontDlg);

	// Create a grid of checkboxes.
	const auto &bitfieldDesc = field->desc.bitfield;
	int count = bitfieldDesc.elements;
	assert(count <= (int)bitfieldDesc.names->size());
	if (count > (int)bitfieldDesc.names->size()) {
		count = (int)bitfieldDesc.names->size();
	}

	// Determine the available width for checkboxes.
	RECT rectDlg;
	GetClientRect(hWndTab, &rectDlg);
	const int max_width = rectDlg.right - pt_start.x;

	// Column widths for multi-row layouts.
	// (Includes the checkbox size.)
	std::vector<int> col_widths;
	int row = 0, col = 0;
	int elemsPerRow = bitfieldDesc.elemsPerRow;
	if (elemsPerRow == 1) {
		// Optimization: Use the entire width of the dialog.
		// TODO: Testing; right margin.
		col_widths.push_back(max_width);
	} else if (elemsPerRow > 1) {
		// Determine the widest entry in each column.
		// If the columns are wider than the available area,
		// reduce the number of columns until it fits.
		for (; elemsPerRow > 1; elemsPerRow--) {
			col_widths.resize(elemsPerRow);
			row = 0; col = 0;
			for (int j = 0; j < count; j++) {
				const rp_string &name = bitfieldDesc.names->at(j);
				if (name.empty())
					continue;
				// Make sure this is a UTF-16 string.
				wstring s_name = RP2W_s(name);

				// Get the width of this specific entry.
				// TODO: Use measureTextSize()?
				SIZE textSize;
				GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
				int chk_w = rect_chkbox.right + textSize.cx;
				if (chk_w > col_widths[col]) {
					col_widths[col] = chk_w;
				}

				// Next column.
				col++;
				if (col == elemsPerRow) {
					// Next row.
					row++;
					col = 0;
				}
			}

			// Add up the widths.
			int total_width = 0;
			for (auto iter = col_widths.begin(); iter != col_widths.end(); ++iter) {
				total_width += *iter;
			}
			// TODO: "DLL" on Windows executables is forced to the next line.
			// Add 7x7 DLU margins?
			if (total_width <= max_width) {
				// Everything fits.
				break;
			}

			// Too wide; try removing a column.
			// Reset the column widths first.
			// TODO: Better way to clear a vector?
			// TODO: Skip the last element?
			memset(col_widths.data(), 0, col_widths.size() * sizeof(int));
		}

		if (elemsPerRow == 1) {
			// We're left with 1 column.
			col_widths.resize(1);
			col_widths[0] = max_width;
		}
	}

	// Initial position on the dialog, in pixels.
	POINT pt = pt_start;
	// Subtract 0.5 DLU from the starting row.
	RECT rect_subtract = {0, 0, 1, 1};
	MapDialogRect(hDlg, &rect_subtract);
	if (rect_subtract.bottom > 1) {
		rect_subtract.bottom /= 2;
	}
	pt.y -= rect_subtract.bottom;

	row = 0; col = 0;
	for (int j = 0; j < count; j++) {
		const rp_string &name = bitfieldDesc.names->at(j);
		if (name.empty())
			continue;
		// Make sure this is a UTF-16 string.
		wstring s_name = RP2W_s(name);

		// Get the text size.
		int chk_w;
		if (elemsPerRow == 0) {
			// Get the width of this specific entry.
			// TODO: Use measureTextSize()?
			SIZE textSize;
			GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
			chk_w = rect_chkbox.right + textSize.cx;
		} else {
			if (col == elemsPerRow) {
				// Next row.
				row++;
				col = 0;
				pt.x = pt_start.x;
				pt.y += rect_chkbox.bottom;
			}

			// Use the largest width in the column.
			chk_w = col_widths[col];
		}

		// FIXME: Tab ordering?
		HWND hCheckBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_BUTTON, s_name.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hWndTab, (HMENU)(INT_PTR)(IDC_RFT_BITFIELD(idx, j)),
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFontDlg, FALSE);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (field->data.bitfield & (1 << j)) ? BST_CHECKED : BST_UNCHECKED);

		// Next column.
		pt.x += chk_w;
		col++;
	}

	// Return the total number of rows times the checkbox height,
	// plus another 0.25 of a checkbox.
	int ret = ((row + 1) * rect_chkbox.bottom);
	ret += (rect_chkbox.bottom / 4);
	return ret;
}

/**
 * Initialize a ListView control.
 * @param hWnd		[in] HWND of the ListView control.
 * @param field		[in] RomFields::Field
 */
void RP_ShellPropSheetExt_Private::initListView(HWND hWnd, const RomFields::Field *field)
{
	if (!hWnd || !field)
		return;
	if (field->type != RomFields::RFT_LISTDATA)
		return;

	const auto &listDataDesc = field->desc.list_data;
	assert(listDataDesc.names != nullptr);
	if (!listDataDesc.names) {
		// No column names...
		return;
	}

	// Set extended ListView styles.
	ListView_SetExtendedListViewStyle(hWnd, LVS_EX_FULLROWSELECT);

	// Insert columns.
	// TODO: Make sure there aren't any columns to start with?
	const int count = (int)listDataDesc.names->size();
	for (int i = 0; i < count; i++) {
		LVCOLUMN lvColumn;
		lvColumn.mask = LVCF_FMT | LVCF_TEXT;
		lvColumn.fmt = LVCFMT_LEFT;
		const rp_string &name = listDataDesc.names->at(i);
		if (!name.empty()) {
			// TODO: Support for RP_UTF8?
			// NOTE: pszText is LPWSTR, not LPCWSTR...
			lvColumn.pszText = (LPWSTR)(name.c_str());
		} else {
			// Don't show this column.
			// FIXME: Zero-width column is a bad hack...
			lvColumn.pszText = L"";
			lvColumn.mask |= LVCF_WIDTH;
			lvColumn.cx = 0;
		}

		ListView_InsertColumn(hWnd, i, &lvColumn);
	}

	// Add the row data.
	auto list_data = field->data.list_data;
	assert(list_data != nullptr);
	if (list_data) {
		const int count = (int)list_data->size();
		for (int i = 0; i < count; i++) {
			LVITEM lvItem;
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = i;

			const vector<rp_string> &data_row = list_data->at(i);
			int col = 0;
			for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, ++col) {
				lvItem.iSubItem = col;
				// TODO: Support for RP_UTF8?
				// NOTE: pszText is LPWSTR, not LPCWSTR...
				lvItem.pszText = (LPWSTR)iter->c_str();
				if (col == 0) {
					// Column 0: Insert the item.
					ListView_InsertItem(hWnd, &lvItem);
				} else {
					// Columns 1 and higher: Set the subitem.
					ListView_SetItem(hWnd, &lvItem);
				}
			}
		}
	}

	// Resize all of the columns.
	// TODO: Do this on system theme change?
	for (int i = 0; i < count; i++) {
		ListView_SetColumnWidth(hWnd, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}

/**
 * Initialize a Date/Time field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDateTime(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size,
	const RomFields::Field *field)
{
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_DATETIME)
		return 0;

	if (field->data.date_time == -1) {
		// Invalid date/time.
		return initString(hDlg, hWndTab, pt_start, idx, size, field, L"Unknown");
	}

	// Format the date/time using the system locale.
	wchar_t dateTimeStr[256];
	int start_pos = 0;
	int cchBuf = ARRAY_SIZE(dateTimeStr);

	// Convert from Unix time to Win32 SYSTEMTIME.
	SYSTEMTIME st;
	UnixTimeToSystemTime(field->data.date_time, &st);

	// At least one of Date and/or Time must be set.
	assert((field->desc.flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME)) != 0);

	if (!(field->desc.flags & RomFields::RFT_DATETIME_IS_UTC)) {
		// Convert to the current timezone.
		SYSTEMTIME st_utc = st;
		BOOL ret = SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st);
		if (!ret) {
			// Conversion failed.
			return 0;
		}
	}

	if (field->desc.flags & RomFields::RFT_DATETIME_HAS_DATE) {
		// Format the date.
		int ret = GetDateFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			DATE_SHORTDATE,
			&st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (field->desc.flags & RomFields::RFT_DATETIME_HAS_TIME) {
		// Format the time.
		if (start_pos > 0 && cchBuf >= 1) {
			// Add a space.
			dateTimeStr[start_pos] = L' ';
			dateTimeStr[start_pos+1] = 0;
			start_pos++;
			cchBuf--;
		}

		int ret = GetTimeFormat(
			MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
			0, &st, nullptr, &dateTimeStr[start_pos], cchBuf);
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += ret-1;
		cchBuf -= ret-1;
	}

	if (start_pos == 0) {
		// Empty string.
		// Something failed...
		return 0;
	}

	// Initialize the string.
	return initString(hDlg, hWndTab, pt_start, idx, size, field, dateTimeStr);
}

/**
 * Initialize an Age Ratings field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initAgeRatings(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size,
	const RomFields::Field *field)
{
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_AGE_RATINGS)
		return 0;

	const RomFields::age_ratings_t *age_ratings = field->data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		return initString(hDlg, hWndTab, pt_start, idx, size, field, L"ERROR");
	}

	// Convert the age ratings field to a string.
	wostringstream woss;
	unsigned int ratings_count = 0;
	for (int i = 0; i < (int)age_ratings->size(); i++) {
		const uint16_t rating = age_ratings->at(i);
		if (!(rating & RomFields::AGEBF_ACTIVE))
			continue;

		if (ratings_count > 0) {
			// Append a comma.
			if (ratings_count % 4 == 0) {
				// 4 ratings per line.
				woss << L",\n";
			} else {
				woss << L", ";
			}
		}

		const char *abbrev = RomFields::ageRatingAbbrev(i);
		if (abbrev) {
			woss << RP2W_s(latin1_to_rp_string(abbrev, -1));
		} else {
			// Invalid age rating.
			// Use the numeric index.
			woss << i;
		}
		woss << L'=';
		woss << RP2W_s(utf8_to_rp_string(RomFields::ageRatingDecode(i, rating)));
		ratings_count++;
	}

	if (ratings_count == 0) {
		// No age ratings.
		woss << L"None";
	}

	// Initialize the string.
	return initString(hDlg, hWndTab, pt_start, idx, size, field, woss.str().c_str());
}

/**
 * Monospaced font enumeration procedure.
 * @param lpelfe Enumerated font information.
 * @param lpntme Font metrics.
 * @param FontType Font type.
 * @param lParam Pointer to RP_ShellPropSheetExt_Private.
 */
int CALLBACK RP_ShellPropSheetExt_Private::MonospacedFontEnumProc(
	const LOGFONT *lpelfe, const TEXTMETRIC *lpntme,
	DWORD FontType, LPARAM lParam)
{
	RP_ShellPropSheetExt_Private *d =
		reinterpret_cast<RP_ShellPropSheetExt_Private*>(lParam);

	// Check the font attributes:
	// - Must be monospaced.
	// - Must be horizontally-oriented.
	if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) &&
	     lpelfe->lfFaceName[0] != '@')
	{
		d->monospaced_fonts.insert(lpelfe->lfFaceName);
	}

	// Continue enumeration.
	return 1;
}

/**
 * Initialize the bold font.
 * @param hFont Base font.
 */
void RP_ShellPropSheetExt_Private::initBoldFont(HFONT hFont)
{
	if (!hFont || hFontBold) {
		// No base font, or the bold font
		// is already initialized.
		return;
	}

	// Create the bold font.
	LOGFONT lfFontBold;
	if (GetObject(hFont, sizeof(lfFontBold), &lfFontBold) != 0) {
		// Adjust the font and create a new one.
		lfFontBold.lfWeight = FW_BOLD;
		hFontBold = CreateFontIndirect(&lfFontBold);
	}
}

/**
 * Initialize the monospaced font.
 * @param hFont Base font.
 */
void RP_ShellPropSheetExt_Private::initMonospacedFont(HFONT hFont)
{
	if (!hFont) {
		// No base font...
		return;
	}

	// Get the current ClearType setting.
	bool bIsClearType = false;
	BOOL bFontSmoothing;
	BOOL bRet = SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &bFontSmoothing, 0);
	if (bRet) {
		UINT uiFontSmoothingType;
		bRet = SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &uiFontSmoothingType, 0);
		if (bRet) {
			bIsClearType = (bFontSmoothing && (uiFontSmoothingType == FE_FONTSMOOTHINGCLEARTYPE));
		}
	}

	if (hFontMono) {
		// Font exists. Only re-create it if the ClearType setting has changed.
		if (bIsClearType == bPrevIsClearType) {
			// ClearType setting has not changed.
			return;
		}
	} else {
		// Font hasn't been created yet.
		if (GetObject(hFont, sizeof(lfFontMono), &lfFontMono) == 0) {
			// Unable to obtain the LOGFONT.
			return;
		}

		// Enumerate all monospaced fonts.
		// Reference: http://www.catch22.net/tuts/fixed-width-font-enumeration
		monospaced_fonts.clear();
#if !defined(_MSC_VER) || _MSC_VER >= 1700
		monospaced_fonts.reserve(64);
#endif
		LOGFONT lfEnumFonts;
		memset(&lfEnumFonts, 0, sizeof(lfEnumFonts));
		lfEnumFonts.lfCharSet = DEFAULT_CHARSET;
		lfEnumFonts.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
		HDC hdc = GetDC(nullptr);
		EnumFontFamiliesEx(hdc, &lfEnumFonts, MonospacedFontEnumProc,
			reinterpret_cast<LPARAM>(this), 0);
		ReleaseDC(nullptr, hdc);

		// Fonts to try.
		static const wchar_t *const fonts[] = {
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
		};

		const wchar_t *font = nullptr;

		for (int i = 0; i < ARRAY_SIZE(fonts); i++) {
			if (monospaced_fonts.find(fonts[i]) != monospaced_fonts.end()) {
				// Found a font.
				font = fonts[i];
				break;
			}
		}

		// We don't need the enumerated fonts anymore.
		monospaced_fonts.clear();

		if (!font) {
			// Monospaced font not found.
			return;
		}

		// Adjust the font and create a new one.
		wcscpy(lfFontMono.lfFaceName, font);
	}

	// Create the monospaced font.
	// If ClearType is enabled, use DEFAULT_QUALITY;
	// otherwise, use NONANTIALIASED_QUALITY.
	lfFontMono.lfQuality = (bIsClearType ? DEFAULT_QUALITY : NONANTIALIASED_QUALITY);
	HFONT hFontMonoNew = CreateFontIndirect(&lfFontMono);
	if (!hFontMonoNew) {
		// Unable to create new font.
		return;
	}

	// Update all existing monospaced controls.
	for (auto iter = hwndMonoControls.cbegin(); iter != hwndMonoControls.cend(); ++iter) {
		SetWindowFont(*iter, hFontMonoNew, FALSE);
	}

	// Delete the old font and save the new one.
	HFONT hFontMonoOld = hFontMono;
	hFontMono = hFontMonoNew;
	if (hFontMonoOld) {
		DeleteFont(hFontMonoOld);
	}
	bPrevIsClearType = bIsClearType;
}

/**
 * Initialize the dialog.
 * Called by WM_INITDIALOG.
 * @param hDlg Dialog window.
 */
void RP_ShellPropSheetExt_Private::initDialog(HWND hDlg)
{
	if (!romData) {
		// No ROM data loaded.
		return;
	}

	// Get the fields.
	const RomFields *fields = romData->fields();
	if (!fields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = fields->count();

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS | ICC_TAB_CLASSES;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Dialog font and device context.
	hFontDlg = GetWindowFont(hDlg);
	AutoGetDC hDC(hDlg, hFontDlg);

	// Initialize the bold and monospaced fonts.
	initBoldFont(hFontDlg);
	initMonospacedFont(hFontDlg);

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	int max_text_width = 0;
	SIZE textSize;
	for (int i = 0; i < count; i++) {
		const RomFields::Field *field = fields->field(i);
		assert(field != nullptr);
		if (!field || !field->isValid)
			continue;
		if (field->name.empty())
			continue;
		// Make sure this is a UTF-16 string.
		wstring s_name = RP2W_s(field->name);

		// TODO: Handle STRF_WARNING?

		// Get the width of this specific entry.
		// TODO: Use measureTextSize()?
		GetTextExtentPoint32(hDC, s_name.data(), (int)s_name.size(), &textSize);
		if (textSize.cx > max_text_width) {
			max_text_width = textSize.cx;
		}
	}

	// Add additional spacing for the ':'.
	// TODO: Use measureTextSize()?
	GetTextExtentPoint32(hDC, L":  ", 3, &textSize);
	max_text_width += textSize.cx;

	// Create the ROM field widgets.
	// Each static control is max_text_width pixels wide
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	RECT tmpRect = {0, 0, 0, 8+4};
	MapDialogRect(hDlg, &tmpRect);
	SIZE descSize = {max_text_width, tmpRect.bottom};

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlg, &dlgMargin);

	// Get the dialog size.
	// - fullDlgRect: Full dialog size
	// - dlgRect: Adjusted dialog size.
	// FIXME: Vertical height is off by 3px on Win7...
	// Verified with WinSpy++: expected 341x408, got 341x405.
	RECT fullDlgRect, dlgRect;
	GetClientRect(hDlg, &fullDlgRect);
	dlgRect = fullDlgRect;
	// Adjust the rectangle for margins.
	InflateRect(&dlgRect, -dlgMargin.left, -dlgMargin.top);
	// Calculate the size for convenience purposes.
	SIZE dlgSize = {
		dlgRect.right - dlgRect.left,
		dlgRect.bottom - dlgRect.top
	};

	// Current position.
	POINT headerPt = {dlgRect.left, dlgRect.top};
	int dlg_value_width = dlgSize.cx - descSize.cx - 1;

	// Create the header row.
	const SIZE header_size = {dlgSize.cx, descSize.cy};
	const int headerH = createHeaderRow(hDlg, headerPt, header_size);
	dlgRect.top += headerH;
	dlgSize.cy -= headerH;
	headerPt.y += headerH;

	// Do we need to create a tab widget?
	if (fields->tabCount() > 1) {
		// Increase the tab widget width by half of the margin.
		InflateRect(&dlgRect, dlgMargin.left/2, 0);
		dlgSize.cx += dlgMargin.left - 1;
		// TODO: Do this regardless of tabs?
		// NOTE: Margin with this change on Win7 is now 9px left, 12px bottom.
		dlgRect.bottom = fullDlgRect.bottom - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;

		// Create the tab widget.
		tabs.resize(fields->tabCount());
		hTabWidget = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_TABCONTROL, nullptr,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			dlgRect.left, dlgRect.top, dlgSize.cx, dlgSize.cy,
			hDlg, (HMENU)(INT_PTR)IDC_TAB_WIDGET,
			nullptr, nullptr);
		SetWindowFont(hTabWidget, hFontDlg, FALSE);
		curTabIndex = 0;

		// Add tabs.
		// NOTE: Tabs must be added *before* calling TabCtrl_AdjustRect();
		// otherwise, the tab bar height won't be taken into account.
		TCITEM tcItem;
		tcItem.mask = TCIF_TEXT;
		for (int i = 0; i < fields->tabCount(); i++) {
			// Create a tab.
			const rp_char *name = fields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}
			tcItem.pszText = const_cast<LPWSTR>(RP2W_c(name));
			// FIXME: Does the index work correctly if a tab is skipped?
			TabCtrl_InsertItem(hTabWidget, i, &tcItem);
		}

		// Adjust the dialog size for subtabs.
		TabCtrl_AdjustRect(hTabWidget, FALSE, &dlgRect);
		// Update dlgSize.
		dlgSize.cx = dlgRect.right - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;
		// Update dlg_value_width.
		// FIXME: Results in 9px left, 8px right margins for RFT_LISTDATA.
		dlg_value_width = dlgSize.cx - descSize.cx - dlgMargin.left - 1;

		// Create windows for each tab.
		DWORD swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW;
		for (int i = 0; i < fields->tabCount(); i++) {
			if (!fields->tabName(i)) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];

			// Create a child dialog for the tab.
			extern HINSTANCE g_hInstance;
			tab.hDlg = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
				hDlg, RP_ShellPropSheetExt::SubtabDlgProc);
			SetWindowPos(tab.hDlg, nullptr,
				dlgRect.left, dlgRect.top,
				dlgSize.cx, dlgSize.cy,
				swpFlags);
			// Hide subsequent tabs.
			swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_HIDEWINDOW;

			// Current point should be equal to the margins.
			// FIXME: On both WinXP and Win7, ths ends up with an
			// 8px left margin, and 6px top/right margins.
			// (Bottom margin is 6px on WinXP, 7px on Win7.)
			tab.curPt.x = dlgMargin.left/2;
			tab.curPt.y = dlgMargin.top/2;
		}
	} else {
		// No tabs.
		// Don't create a WC_TABCONTROL, but simulate a single
		// tab in tabs[] to make it easier to work with.
		tabs.resize(1);
		auto &tab = tabs[0];
		tab.hDlg = hDlg;
		tab.curPt = headerPt;
	}

	for (int idx = 0; idx < count; idx++) {
		const RomFields::Field *field = fields->field(idx);
		assert(field != nullptr);
		if (!field || !field->isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field->tabIdx;
		assert(tabIdx >= 0 && tabIdx < (int)tabs.size());
		if (tabIdx < 0 || tabIdx >= (int)tabs.size()) {
			// Tab index is out of bounds.
			continue;
		} else if (!tabs[tabIdx].hDlg) {
			// Tab name is empty. Tab is hidden.
			continue;
		}

		// Current tab.
		auto &tab = tabs[tabIdx];

		// Append a ":" to the description.
		// TODO: Localization.
		wstring desc_text = RP2W_s(field->name);
		desc_text += L':';

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, desc_text.c_str(),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			tab.curPt.x, tab.curPt.y, descSize.cx, descSize.cy,
			tab.hDlg, (HMENU)(INT_PTR)(IDC_STATIC_DESC(idx)),
			nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, FALSE);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {tab.curPt.x + descSize.cx, tab.curPt.y};
		switch (field->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				DestroyWindow(hStatic);
				field_cy = 0;
				break;

			case RomFields::RFT_STRING: {
				// String data.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initString(hDlg, tab.hDlg, pt_start, idx, size, field, nullptr);
				if (field_cy == 0) {
					// initString() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(hDlg, tab.hDlg, pt_start, idx, field);
				if (field_cy == 0) {
					// initBitfield() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;

			case RomFields::RFT_LISTDATA: {
				assert(field->desc.list_data.names != nullptr);
				if (!field->desc.list_data.names) {
					// No column names...
					break;
				}

				// Increase row height to 72 DLU.
				// descSize is 8+4 DLU (12); 72/12 == 6
				// FIXME: The last row seems to be cut off with the
				// Windows XP Luna theme, but not Windows Classic...
				field_cy *= 6;

				// Create a ListView widget.
				HWND hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
					WC_LISTVIEW, nullptr,
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_ALIGNLEFT | LVS_REPORT,
					pt_start.x, pt_start.y,
					dlg_value_width, field_cy,
					tab.hDlg, (HMENU)(INT_PTR)(IDC_RFT_LISTDATA(idx)),
					nullptr, nullptr);
				SetWindowFont(hDlgItem, hFontDlg, FALSE);

				// Initialize the ListView data.
				initListView(hDlgItem, field);
				break;
			}

			case RomFields::RFT_DATETIME: {
				// Date/Time in Unix format.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initDateTime(hDlg, tab.hDlg, pt_start, idx, size, field);
				if (field_cy == 0) {
					// initDateTime() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			case RomFields::RFT_AGE_RATINGS: {
				// Age Ratings field.
				SIZE size = {dlg_value_width, field_cy};
				field_cy = initAgeRatings(hDlg, tab.hDlg, pt_start, idx, size, field);
				if (field_cy == 0) {
					// initAgeRatings() failed.
					// Remove the description label.
					DestroyWindow(hStatic);
				}
				break;
			}

			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				DestroyWindow(hStatic);
				field_cy = 0;
				break;
		}

		// Next row.
		tab.curPt.y += field_cy;
	}
}

/** RP_ShellPropSheetExt **/

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d_ptr(new RP_ShellPropSheetExt_Private(this))
{ }

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellPropSheetExt, IShellExtInit),
		QITABENT(RP_ShellPropSheetExt, IShellPropSheetExt),
		{ 0 }
	};
	return pQISearch(this, rgqit, riid, ppvObj);
}

/** IShellExtInit **/

/** IShellPropSheetExt **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb775094(v=vs.85).aspx
IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!pDataObj) {
		return E_INVALIDARG;
	}

	// TODO: Handle CFSTR_MOUNTEDVOLUME for volumes mounted on an NTFS mount point.
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon. In this 
	// example, we get an HDROP handle for enumerating the selected files and 
	// folders.
	if (FAILED(pDataObj->GetData(&fe, &stm)))
		return E_FAIL;

	// Get an HDROP handle.
	HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
	if (!hDrop) {
		ReleaseStgMedium(&stm);
		return E_FAIL;
	}

	RP_D(RP_ShellPropSheetExt);
	HRESULT hr = E_FAIL;
	UINT nFiles, cchFilename;
	wchar_t *filename = nullptr;
	unique_ptr<IRpFile> file;
	RomData *romData = nullptr;

	// Determine how many files are involved in this operation. This
	// code sample displays the custom context menu item when only
	// one file is selected.
	nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	if (nFiles != 1) {
		// Wrong file count.
		goto cleanup;
	}

	// Get the path of the file.
	cchFilename = DragQueryFile(hDrop, 0, nullptr, 0);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	cchFilename++;	// Add one for the NULL terminator.
	filename = (wchar_t*)malloc(cchFilename * sizeof(wchar_t));
	if (!filename) {
		// Memory allocation failed.
		goto cleanup;
	}
	cchFilename = DragQueryFile(hDrop, 0, filename, cchFilename);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	// Check if this is a drive letter.
	if (cchFilename == 3 && iswalpha(filename[0]) &&
	    filename[1] == L':' && filename[2] == L'\\')
	{
		// This is a drive letter.
		// Only CD-ROM (and similar) drives are supported.
		// TODO: Verify if opening by drive letter works,
		// or if we have to resolve the physical device name.
		if (GetDriveType(filename) != DRIVE_CDROM) {
			// Not a CD-ROM drive.
			goto cleanup;
		}
	}

	// Open the file.
	file.reset(new RpFile(W2RP_cl(filename, cchFilename), RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Unable to open the file.
		goto cleanup;
	}

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	romData = RomDataFactory::create(file.get());
	if (!romData) {
		// Could not open the RomData object.
		goto cleanup;
	}

	// Make sure the existing RomData is unreferenced.
	// TODO: If the filename matches, don't reopen?
	if (d->romData) {
		d->romData->unref();
	}
	d->romData = romData;
	hr = S_OK;

cleanup:
	GlobalUnlock(stm.hGlobal);
	ReleaseStgMedium(&stm);
	free(filename);

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7

	// Create a property sheet page.
	extern HINSTANCE g_hInstance;
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTY_SHEET);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"ROM Properties";
	psp.pfnDlgProc = DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = CallbackProc;
	psp.lParam = reinterpret_cast<LPARAM>(this);

	HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
	if (hPage == NULL) {
		return E_OUTOFMEMORY;
	}

	// The property sheet page is then added to the property sheet by calling 
	// the callback function (LPFNADDPROPSHEETPAGE pfnAddPage) passed to 
	// IShellPropSheetExt::AddPages.
	if (pfnAddPage(hPage, lParam)) {
		// By default, after AddPages returns, the shell releases its 
		// IShellPropSheetExt interface and the property page cannot access the
		// extension object. However, it is sometimes desirable to be able to use 
		// the extension object, or some other object, from the property page. So 
		// we increase the reference count and maintain this object until the 
		// page is released in PropPageCallbackProc where we call Release upon 
		// the extension.
		this->AddRef();
	} else {
		DestroyPropertySheetPage(hPage);
		return E_FAIL;
	}

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return S_OK;
}

IFACEMETHODIMP RP_ShellPropSheetExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam)
{
	// Not used.
	((void)uPageID);
	((void)pfnReplaceWith);
	((void)lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Access the property sheet extension from property page.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
			if (!pExt)
				return TRUE;
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;

			// Store the object pointer with this particular page dialog.
			SetProp(hDlg, EXT_POINTER_PROP, static_cast<HANDLE>(pExt));
			// Save handles for later.
			d->hDlgProps = GetParent(hDlg);
			d->hDlgSheet = hDlg;

			// Initialize the dialog.
			d->initDialog(hDlg);

			// Make sure the underlying file handle is closed,
			// since we don't need it once the RomData has been
			// loaded by RomDataView.
			d->romData->close();
			return TRUE;
		}

		// FIXME: Resize the dialog widgets.
#if 0
		case WM_SIZE: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (!pExt) {
				// No RP_ShellPropSheetExt. Can't do anything...
				return FALSE;
			}

			// TODO: Support dynamic resizing? The standard
			// Explorer file properties dialog doesn't support
			// it, but others might...
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			if (d->romData->isOpen()) {
				// Initialize the dialog.
				d->initDialog(hDlg);

				// Make sure the underlying file handle is closed,
				// since we don't need it once the RomData has been
				// loaded by RomDataView.
				d->romData->close();
			}

			// Continue normal processing.
			break;
		}
#endif

		case WM_DESTROY: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				// Stop the animation timer.
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				d->stopAnimTimer();
			}

			// Remove the EXT_POINTER_PROP property from the page. 
			// The EXT_POINTER_PROP property stored the pointer to the 
			// FilePropSheetExt object.
			RemoveProp(hDlg, EXT_POINTER_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (!pExt) {
				// No RP_ShellPropSheetExt. Can't do anything...
				return FALSE;
			}

			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_SETACTIVE:
					d->startAnimTimer();
					break;

				case PSN_KILLACTIVE:
					d->stopAnimTimer();
					break;

				case NM_CLICK:
				case NM_RETURN: {
					// Check if this is a SysLink control.
					RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
					if (d->hwndSysLinkControls.find(lppsn->hdr.hwndFrom) !=
					    d->hwndSysLinkControls.end())
					{
						// It's a SysLink control.
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;
				}

				case TCN_SELCHANGE: {
					// Tab change. Make sure this is the correct WC_TABCONTROL.
					RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
					if (d->hTabWidget != nullptr && d->hTabWidget == lppsn->hdr.hwndFrom) {
						// Tab widget. Show the selected tab.
						int newTabIndex = TabCtrl_GetCurSel(d->hTabWidget);
						ShowWindow(d->tabs[d->curTabIndex].hDlg, SW_HIDE);
						d->curTabIndex = newTabIndex;
						ShowWindow(d->tabs[newTabIndex].hDlg, SW_SHOW);
					}
					break;
				}

				default:
					break;
			}

			// Continue normal processing.
			break;
		}

		case WM_PAINT: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (!pExt) {
				// No RP_ShellPropSheetExt. Can't do anything...
				return FALSE;
			}

			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			if (!d->hbmpBanner && !d->hbmpIconFrames[0]) {
				// Nothing to draw...
				break;
			}

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg, &ps);

			// Memory DC for BitBlt.
			HDC hdcMem = CreateCompatibleDC(hdc);

			// Draw the banner.
			if (d->hbmpBanner) {
				HBITMAP hbmOld = SelectBitmap(hdcMem, d->hbmpBanner);
				BitBlt(hdc, d->ptBanner.x, d->ptBanner.y,
					d->szBanner.cx, d->szBanner.cy,
					hdcMem, 0, 0, SRCCOPY);
				SelectBitmap(hdcMem, hbmOld);
			}

			// Draw the icon.
			if (d->hbmpIconFrames[d->last_frame_number]) {
				HBITMAP hbmOld = SelectBitmap(hdcMem, d->hbmpIconFrames[d->last_frame_number]);
				BitBlt(hdc, d->rectIcon.left, d->rectIcon.top,
					d->szIcon.cx, d->szIcon.cy,
					hdcMem, 0, 0, SRCCOPY);
				SelectBitmap(hdcMem, hbmOld);
			}

			DeleteDC(hdcMem);
			EndPaint(hDlg, &ps);

			return TRUE;
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			// Reload the images.
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				d->loadImages(hDlg);
			}
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				d->initMonospacedFont(d->hFontDlg);
			}
			break;
		}

		case WM_CTLCOLORSTATIC: {
			RP_ShellPropSheetExt *pExt = static_cast<RP_ShellPropSheetExt*>(
				GetProp(hDlg, EXT_POINTER_PROP));
			if (pExt) {
				RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
				if (d->hwndWarningControls.find(reinterpret_cast<HWND>(lParam)) !=
				    d->hwndWarningControls.end())
				{
					// Set the "Warning" color.
					HDC hdc = reinterpret_cast<HDC>(wParam);
					SetTextColor(hdc, RGB(255, 0, 0));
				}
			}
			break;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}


//
//   FUNCTION: FilePropPageCallbackProc
//
//   PURPOSE: Specifies an application-defined callback function that a property
//            sheet calls when a page is created and when it is about to be 
//            destroyed. An application can use this function to perform 
//            initialization and cleanup operations for the page.
//
UINT CALLBACK RP_ShellPropSheetExt::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// When the callback function receives the PSPCB_RELEASE notification, 
			// the ppsp parameter of the PropSheetPageProc contains a pointer to 
			// the PROPSHEETPAGE structure. The lParam member of the PROPSHEETPAGE 
			// structure contains the extension pointer which can be used to 
			// release the object.

			// Release the property sheet extension object. This is called even 
			// if the property page was never actually displayed.
			RP_ShellPropSheetExt *pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
			if (pExt) {
				pExt->Release();
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}

/**
 * Subclass procedure for ES_MULTILINE EDIT controls.
 * @param hWnd		Control handle.
 * @param uMsg		Message.
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID. (usually the control ID)
 * @param dwRefData	RP_ShellProSheetExt*
 */
LRESULT CALLBACK RP_ShellPropSheetExt::MultilineEditProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if (!dwRefData) {
		// No RP_ShellPropSheetExt. Can't do anything...
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
	RP_ShellPropSheetExt *const pExt =
		reinterpret_cast<RP_ShellPropSheetExt*>(dwRefData);

	switch (uMsg) {
		case WM_KEYDOWN: {
			// Work around Enter/Escape issues.
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;
			switch (wParam) {
				case VK_RETURN:
					SendMessage(d->hDlgProps, WM_COMMAND, IDOK, 0);
					return TRUE;

				case VK_ESCAPE:
					SendMessage(d->hDlgProps, WM_COMMAND, IDCANCEL, 0);
					return TRUE;

				default:
					break;
			}
			break;
		}

		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MultilineEditProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Animated icon timer.
 * @param hWnd
 * @param uMsg
 * @param idEvent
 * @param dwTime
 */
void CALLBACK RP_ShellPropSheetExt::AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (hWnd == nullptr || idEvent == 0) {
		// Not a valid timer procedure call...
		// - hWnd should not be nullptr.
		// - idEvent should be the 'd' pointer.
		return;
	}

	RP_ShellPropSheetExt_Private *d =
		reinterpret_cast<RP_ShellPropSheetExt_Private*>(idEvent);

	// Next frame.
	int delay = 0;
	int frame = d->iconAnimHelper.nextFrame(&delay);
	if (delay <= 0 || frame < 0) {
		// Invalid frame...
		KillTimer(hWnd, idEvent);
		d->animTimerID = 0;
		return;
	}

	if (frame != d->last_frame_number) {
		// New frame number.
		// Update the icon.
		d->last_frame_number = frame;
		InvalidateRect(d->hDlgSheet, &d->rectIcon, FALSE);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, RP_ShellPropSheetExt::AnimTimerProc);
}

/**
 * Dialog procedure for subtabs.
 * @param hWnd
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK RP_ShellPropSheetExt::SubtabDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Dummy callback procedure that does nothing.
	return FALSE; // Let system deal with other messages
}
