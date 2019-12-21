/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.cpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// References:
// - http://www.codeproject.com/Articles/338268/COM-in-C
// - https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
// - https://msdn.microsoft.com/en-us/library/ms677109(v=vs.85).aspx

#include "stdafx.h"
#include "RP_ShellPropSheetExt.hpp"
#include "RpImageWin32.hpp"
#include "res/resource.h"

// libwin32common
#include "libwin32common/AutoGetDC.hpp"
#include "libwin32common/WinUI.hpp"
#include "libwin32common/w32time.h"
#include "libwin32common/WTSSessionNotification.hpp"
using LibWin32Common::AutoGetDC;
using LibWin32Common::WTSSessionNotification;

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/TextFuncs_wchar.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;

// libi18n
// NOTE: Using "RomDataView" for the context, since that
// matches what's used for the KDE and GTK+ frontends.
#include "libi18n/i18n.h"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using std::array;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::string;
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
#include "librptexture/img/GdiplusHelper.hpp"
#include "librptexture/img/RpGdiplusBackend.hpp"

// Icon animation.
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

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
		// Property for "D pointer".
		// This points to the ConfigDialogPrivate object.
		static const TCHAR D_PTR_PROP[];

	public:
		// ROM filename.
		string filename;
		// ROM data. (Not opened until the properties tab is shown.)
		RomData *romData;

		// Useful window handles.
		HWND hDlgSheet;		// Property sheet.

		// Fonts.
		HFONT hFontDlg;		// Main dialog font.
		HFONT hFontBold;	// Bold font.
		HFONT hFontMono;	// Monospaced font.

		// Monospaced font details.
		LOGFONT lfFontMono;
		vector<HWND> hwndMonoControls;			// Controls using the monospaced font.
		bool bPrevIsClearType;	// Previous ClearType setting.

		// Controls that need to be drawn using red text.
		unordered_set<HWND> hwndWarningControls;	// Controls using the "Warning" font.
		// SysLink controls.
		unordered_set<HWND> hwndSysLinkControls;
		// ListView controls. (for toggling LVS_EX_DOUBLEBUFFER)
		vector<HWND> hwndListViewControls;

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

		// wtsapi32.dll for Remote Desktop status. (WinXP and later)
		WTSSessionNotification wts;

		// Alternate row color.
		COLORREF colorAltRow;
		bool isFullyInit;		// True if the window is fully initialized.

		// ListView string data.
		// - Key: ListView dialog ID (TODO: Use uint16_t instead?)
		// - Value: Double-vector of tstrings.
		unordered_map<unsigned int, vector<vector<tstring> > > map_lvStringData;

		// ListView checkboxes.
		unordered_map<unsigned int, uint32_t> map_lvCheckboxes;

		// ListView ImageList indexes.
		unordered_map<unsigned int, vector<int> > map_lvImageList;

		/**
		 * ListView GetDispInfo function.
		 * @param plvdi	[in/out] NMLVDISPINFO
		 * @return TRUE if handled; FALSE if not.
		 */
		inline BOOL ListView_GetDispInfo(NMLVDISPINFO *plvdi);

		/**
		 * ListView CustomDraw function.
		 * @param plvcd	[in/out] NMLVCUSTOMDRAW
		 * @return Return value.
		 */
		inline int ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd);

		// Banner.
		HBITMAP hbmpBanner;
		POINT ptBanner;
		SIZE szBanner;
		bool nearest_banner;

		// Tab layout.
		HWND hTabWidget;
		struct tab {
			HWND hDlg;		// Tab child dialog.
			POINT curPt;		// Current point.
		};
		vector<tab> tabs;
		int curTabIndex;

		// Animated icon data.
		array<HBITMAP, IconAnimData::MAX_FRAMES> hbmpIconFrames;
		RECT rectIcon;
		SIZE szIcon;
		bool nearest_icon;
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

	public:
		/**
		 * Load the banner and icon as HBITMAPs.
		 *
		 * This function should bee called on startup and if
		 * the window's background color changes.
		 *
		 * NOTE: The HWND isn't needed here, since this function
		 * doesn't touch the dialog at all.
		 */
		void loadImages(void);

	private:
		/**
		 * Rescale an image to be as close to the required size as possible.
		 * @param req_sz	[in] Required size.
		 * @param sz		[in/out] Image size.
		 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
		 */
		static bool rescaleImage(const SIZE &req_sz, SIZE &sz);

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
		 * @param str		[in,opt] String data. (If nullptr, field data is used.)
		 * @return Field height, in pixels.
		 */
		int initString(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size,
			const RomFields::Field *field, LPCTSTR str);

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
		 * Initialize a ListData field.
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a default ListView.
		 * @param doResize	[in] If true, resize the ListView to accomodate rows_visible.
		 * @param field		[in] RomFields::Field
		 * @return Field height, in pixels.
		 */
		int initListData(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size, bool doResize,
			const RomFields::Field *field);

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
		 * Initialize a Dimensions field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param idx		[in] Field index.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @return Field height, in pixels.
		 */
		int initDimensions(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, int idx, const SIZE &size,
			const RomFields::Field *field);

		/**
		 * Initialize the bold font.
		 * @param hFont Base font.
		 */
		void initBoldFont(HFONT hFont);

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

	private:
		// Internal functions used by the callback functions.
		INT_PTR DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr);
		INT_PTR DlgProc_WM_PAINT(HWND hDlg);

	public:
		// Property sheet callback functions.
		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static UINT CALLBACK CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

		/**
		 * Animated icon timer.
		 * @param hWnd
		 * @param uMsg
		 * @param idEvent
		 * @param dwTime
		 */
		static void CALLBACK AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

		/**
		 * Dialog procedure for subtabs.
		 * @param hDlg
		 * @param uMsg
		 * @param wParam
		 * @param lParam
		 */
		static INT_PTR CALLBACK SubtabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/** RP_ShellPropSheetExt_Private **/

// Property for "D pointer".
// This points to the ConfigDialogPrivate object.
const TCHAR RP_ShellPropSheetExt_Private::D_PTR_PROP[] = _T("RP_ShellPropSheetExt_Private");

RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q)
	: q_ptr(q)
	, romData(nullptr)
	, hDlgSheet(nullptr)
	, hFontDlg(nullptr)
	, hFontBold(nullptr)
	, hFontMono(nullptr)
	, bPrevIsClearType(nullptr)
	, lblSysInfo(nullptr)
	, colorWinBg(0)
	, hUxTheme_dll(nullptr)
	, pfnIsThemeActive(nullptr)
	, colorAltRow(0)
	, isFullyInit(false)
	, hbmpBanner(nullptr)
	, nearest_banner(true)
	, hTabWidget(nullptr)
	, curTabIndex(0)
	, nearest_icon(true)
	, animTimerID(0)
	, last_frame_number(0)
{
	memset(&lfFontMono, 0, sizeof(lfFontMono));
	hbmpIconFrames.fill(nullptr);
	memset(&ptBanner, 0, sizeof(ptBanner));
	memset(&szBanner, 0, sizeof(szBanner));
	memset(&rectIcon, 0, sizeof(rectIcon));
	memset(&szIcon, 0, sizeof(szIcon));

	// Attempt to get IsThemeActive() from uxtheme.dll.
	// TODO: Move this to RP_ComBase so we don't have to look it up
	// every time the property dialog is loaded?
	hUxTheme_dll = LoadLibrary(_T("uxtheme.dll"));
	if (hUxTheme_dll) {
		pfnIsThemeActive = (PFNISTHEMEACTIVE)GetProcAddress(hUxTheme_dll, "IsThemeActive");
	}

	// Initialize the alternate row color.
	colorAltRow = LibWin32Common::getAltRowColor();
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
	std::for_each(hbmpIconFrames.begin(), hbmpIconFrames.end(),
		[](HBITMAP hbmp) { if (hbmp) { DeleteObject(hbmp); } }
	);

	// Delete the fonts.
	if (hFontBold) {
		DeleteFont(hFontBold);
	}
	if (hFontMono) {
		DeleteFont(hFontMono);
	}

	// Close DLLs.
	if (hUxTheme_dll) {
		FreeLibrary(hUxTheme_dll);
	}
}

/**
 * Start the animation timer.
 */
void RP_ShellPropSheetExt_Private::startAnimTimer(void)
{
	if (!iconAnimHelper.isAnimated()) {
		// Not an animated icon.
		return;
	}

	// Get the current frame information.
	last_frame_number = iconAnimHelper.frameNumber();
	const int delay = iconAnimHelper.frameDelay();
	assert(delay > 0);
	if (delay <= 0) {
		// Invalid delay value.
		return;
	}

	// Set a timer for the current frame.
	// We're using the 'd' pointer as nIDEvent.
	animTimerID = SetTimer(hDlgSheet,
		reinterpret_cast<UINT_PTR>(this),
		delay, AnimTimerProc);
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
 * Load the banner and icon as HBITMAPs.
 *
 * This function should be called on startup and if
 * the window's background color changes.
 *
 * NOTE: The HWND isn't needed here, since this function
 * doesn't touch the dialog at all.
 */
void RP_ShellPropSheetExt_Private::loadImages(void)
{
	// Window background color.
	// Static controls don't support alpha transparency (?? test),
	// so we have to fake it.
	// TODO: Get the actual background color of the window.
	// TODO: Use DrawThemeBackground:
	// - http://www.codeproject.com/Articles/5978/Correctly-drawn-themed-dialogs-in-WinXP
	// - https://blogs.msdn.microsoft.com/dsui_team/2013/06/26/using-theme-apis-to-draw-the-border-of-a-control/
	// - https://blogs.msdn.microsoft.com/pareshj/2011/11/03/draw-the-background-of-static-control-with-gradient-fill-when-theme-is-enabled/
	int colorIndex;
	if (pfnIsThemeActive && pfnIsThemeActive()) {
		// Theme is active.
		colorIndex = COLOR_WINDOW;
	} else {
		// Theme is not active.
		colorIndex = COLOR_3DFACE;
	}
	const Gdiplus::ARGB gdipBgColor = LibWin32Common::GetSysColor_ARGB32(colorIndex);

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
			// Save the banner size.
			if (szBanner.cx == 0) {
				szBanner.cx = banner->width();
				szBanner.cy = banner->height();
				// FIXME: Uncomment once proper aspect ratio scaling has been implemented.
				// All banners are 96x32 right now.
				//static const SIZE req_szBanner = {96, 32};
				//nearest = rescaleImage(req_szBanner, szBanner);
				nearest_banner = true;
			}

			// Convert to HBITMAP using the window background color.
			// TODO: Redo if the window background color changes.
			hbmpBanner = RpImageWin32::toHBITMAP(banner, gdipBgColor, szBanner, nearest_banner);
		}
	}

	// Icon.
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Delete the old icons.
		std::for_each(hbmpIconFrames.begin(), hbmpIconFrames.end(),
			[](HBITMAP &hbmp) {
				if (hbmp) {
					DeleteObject(hbmp);
					hbmp = nullptr;
				}
			}
		);

		// Get the icon.
		const rp_image *icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			// Save the icon size.
			if (szIcon.cx == 0) {
				szIcon.cx = icon->width();
				szIcon.cy = icon->height();
				static const SIZE req_szIcon = {32, 32};
				nearest_icon = rescaleImage(req_szIcon, szIcon);
			}

			// Get the animated icon data.
			const IconAnimData *const iconAnimData = romData->iconAnimData();
			if (iconAnimData) {
				// Convert the icons to GDI+ bitmaps.
				for (int i = iconAnimData->count-1; i >= 0; i--) {
					if (iconAnimData->frames[i] && iconAnimData->frames[i]->isValid()) {
						// Convert to HBITMAP using the window background color.
						hbmpIconFrames[i] = RpImageWin32::toHBITMAP(iconAnimData->frames[i], gdipBgColor, szIcon, nearest_icon);
					}
				}

				// Set up the IconAnimHelper.
				iconAnimHelper.setIconAnimData(iconAnimData);
				last_frame_number = iconAnimHelper.frameNumber();

				// Icon animation timer is set in startAnimTimer().
			} else {
				// Not an animated icon.
				last_frame_number = 0;
				iconAnimHelper.setIconAnimData(nullptr);

				// Convert to HBITMAP using the window background color.
				hbmpIconFrames[0] = RpImageWin32::toHBITMAP(icon, gdipBgColor, szIcon, nearest_icon);
			}
		}
	}
}

/**
 * Rescale an image to be as close to the required size as possible.
 * @param req_sz	[in] Required size.
 * @param sz		[in/out] Image size.
 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
 */
bool RP_ShellPropSheetExt_Private::rescaleImage(const SIZE &req_sz, SIZE &sz)
{
	// TODO: Adjust req_sz for DPI.
	if (sz.cx == req_sz.cx && sz.cy == req_sz.cy) {
		// No resize necessary.
		return true;
	}

	// Check if the image is too big.
	if (sz.cx >= req_sz.cx || sz.cy >= req_sz.cy) {
		// Image is too big. Shrink it.
		// FIXME: Assuming the icon is always a power of two.
		// Move TCreateThumbnail::rescale_aspect() into another file
		// and make use of that.
		sz.cx = 32;
		sz.cy = 32;
		return false;
	}

	// Image is too small.
	// TODO: Ensure dimensions don't exceed req_img_size.
	SIZE orig_sz = sz;
	do {
		// Increase by integer multiples until
		// the icon is at least 32x32.
		// TODO: Constrain to 32x32?
		sz.cx += orig_sz.cx;
		sz.cy += orig_sz.cy;
	} while (sz.cx < req_sz.cx && sz.cy < req_sz.cy);
	return true;
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
	if (!hDlg || !romData)
		return 0;

	// Total widget width.
	int total_widget_width = 0;

	// Label size.
	SIZE sz_lblSysInfo = {0, 0};

	// Font to use.
	// TODO: Handle these assertions in release builds.
	assert(hFontBold != nullptr);
	assert(hFontDlg != nullptr);
	const HFONT hFont = (hFontBold ? hFontBold : hFontDlg);

	// System name and file type.
	// TODO: System logo and/or game title?
	const char *systemName = romData->systemName(
		RomData::SYSNAME_TYPE_LONG | RomData::SYSNAME_REGION_ROM_LOCAL);
	const char *fileType = romData->fileType_string();
	assert(systemName != nullptr);
	assert(fileType != nullptr);
	if (!systemName) {
		systemName = C_("RomDataView", "(unknown system)");
	}
	if (!fileType) {
		fileType = C_("RomDataView", "(unknown filetype)");
	}

	const tstring tSysInfo =
		LibWin32Common::unix2dos(U82T_s(rp_sprintf_p(
			// tr: %1$s == system name, %2$s == file type
			C_("RomDataView", "%1$s\n%2$s"), systemName, fileType)));

	if (!tSysInfo.empty()) {
		// Determine the appropriate label size.
		int ret = LibWin32Common::measureTextSize(hDlg, hFont, tSysInfo, &sz_lblSysInfo);
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

	// Add the banner and icon widths.

	// Banner.
	// TODO: Spacing between banner and text?
	// Doesn't seem to be needed with Dreamcast saves...
	total_widget_width += szBanner.cx;

	// Icon.
	if (szIcon.cx > 0) {
		if (total_widget_width > 0) {
			total_widget_width += pt_start.x;
		}
		total_widget_width += szIcon.cx;
	}

	// Starting point.
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (sz_lblSysInfo.cx > 0 && sz_lblSysInfo.cy > 0) {
		lblSysInfo = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, tSysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			curPt.x, curPt.y,
			sz_lblSysInfo.cx, sz_lblSysInfo.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysInfo, hFont, false);
		curPt.x += sz_lblSysInfo.cx + pt_start.x;
	}

	// Banner.
	if (szBanner.cx > 0) {
		ptBanner = curPt;
		curPt.x += szBanner.cx + pt_start.x;
	}

	// Icon.
	if (szIcon.cx > 0) {
		SetRect(&rectIcon, curPt.x, curPt.y,
			curPt.x + szIcon.cx, curPt.y + szIcon.cy);
		curPt.x += szIcon.cx + pt_start.x;
	}

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
 * @param str		[in,opt] String data. (If nullptr, field data is used.)
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initString(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size,
	const RomFields::Field *field, LPCTSTR str)
{
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	if (!hDlg || !hWndTab || !field)
		return 0;

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Field object.
	int lf_count = 0;
	tstring str_nl;
	if (!str) {
		if (field->type != RomFields::RFT_STRING)
			return 0;

		// TODO: NULL string == empty string?
		if (field->data.str) {
			str_nl = LibWin32Common::unix2dos(U82T_s(*(field->data.str)), &lf_count);
		}
	} else {
		// Use the specified string.
		str_nl = LibWin32Common::unix2dos(str, &lf_count);
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
					SetWindowFont(hStatic, hFont, false);
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

		// FIXME: SysLink does not support ANSI.
		// Remove links from the text if this is an ANSI build.
#ifdef UNICODE
# define SYSLINK_CONTROL WC_LINK
#else /* !UNICODE */
# define SYSLINK_CONTROL WC_STATIC
#endif /* UNICODE */

		// Create a SysLink widget.
		// SysLink allows the user to click a link and
		// open a webpage. It does NOT allow highlighting.
		// TODO: SysLink + EDIT?
		// FIXME: Centered text alignment?
		// TODO: With subtabs:
		// - Verify behavior of LWS_TRANSPARENT.
		// - Show below subtabs.
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			SYSLINK_CONTROL, str_nl.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			0, 0, 0, 0,	// will be adjusted afterwards
			hWndTab, cId, nullptr, nullptr);
		// There should be a maximum of one STRF_CREDITS per RomData subclass.
		assert(hwndSysLinkControls.empty());
		hwndSysLinkControls.insert(hDlgItem);
		SetWindowFont(hDlgItem, hFont, false);

		// NOTE: We can't use LibWin32Common::measureTextSize() because
		// that includes the HTML markup, and LM_GETIDEALSIZE is Vista+ only.
		// Use a wrapper measureTextSizeLink() that removes HTML-like
		// tags and then calls measureTextSize().
		SIZE szText;
		LibWin32Common::measureTextSizeLink(hWndTab, hFont, str_nl, &szText);

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
			WC_EDIT, str_nl.c_str(), dwStyle,
			pt_start.x, pt_start.y,
			size.cx, field_cy,
			hWndTab, cId, nullptr, nullptr);
		SetWindowFont(hDlgItem, hFont, false);

		// Get the EDIT control margins.
		const DWORD dwMargins = (DWORD)SendMessage(hDlgItem, EM_GETMARGINS, 0, 0);

		// Adjust the window size to compensate for the margins not being clickable.
		// NOTE: Not adjusting the right margins.
		SetWindowPos(hDlgItem, nullptr, pt_start.x - LOWORD(dwMargins), pt_start.y,
			size.cx + LOWORD(dwMargins), field_cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		// Subclass multi-line EDIT controls to work around Enter/Escape issues.
		// We're also subclassing single-line EDIT controls to disable the
		// initial selection. (DLGC_HASSETSEL)
		// Reference:  http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
		// TODO: Error handling?
		SUBCLASSPROC proc = (dwStyle & ES_MULTILINE)
			? LibWin32Common::MultiLineEditProc
			: LibWin32Common::SingleLineEditProc;
		SetWindowSubclass(hDlgItem, proc,
			reinterpret_cast<UINT_PTR>(cId),
			reinterpret_cast<DWORD_PTR>(GetParent(hDlgSheet)));
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
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	assert(field->type == RomFields::RFT_BITFIELD);
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
	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32)
		count = 32;

	// Determine the available width for checkboxes.
	RECT rectDlg;
	GetClientRect(hWndTab, &rectDlg);
	const int max_width = rectDlg.right - pt_start.x;

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> tnames;
	tnames.reserve(count);
	std::for_each(bitfieldDesc.names->cbegin(), bitfieldDesc.names->cend(),
		[&tnames](const string &name) {
			if (name.empty()) {
				// Skip U82T_s() for empty strings.
				tnames.push_back(tstring());
			} else {
				tnames.push_back(U82T_s(name));
			}
		}
	);

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
			auto iter = tnames.cbegin();
			for (int j = 0; j < count; ++iter, j++) {
				const tstring &tname = *iter;
				if (tname.empty())
					continue;

				// Get the width of this specific entry.
				// TODO: Use LibWin32Common::measureTextSize()?
				SIZE textSize;
				GetTextExtentPoint32(hDC, tname.data(), (int)tname.size(), &textSize);
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
			const int total_width = std::accumulate(col_widths.cbegin(), col_widths.cend(), 0);

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
	auto iter = tnames.cbegin();
	for (int j = 0; j < count; ++iter, j++) {
		const tstring &tname = *iter;
		if (tname.empty())
			continue;

		// Get the text size.
		int chk_w;
		if (elemsPerRow == 0) {
			// Get the width of this specific entry.
			// TODO: Use LibWin32Common::measureTextSize()?
			SIZE textSize;
			GetTextExtentPoint32(hDC, tname.data(), (int)tname.size(), &textSize);
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
			WC_BUTTON, tname.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hWndTab, (HMENU)(INT_PTR)(IDC_RFT_BITFIELD(idx, j)),
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFontDlg, false);

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
 * Initialize a ListData field.
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a default ListView.
 * @param doResize	[in] If true, resize the ListView to accomodate rows_visible.
 * @param field		[in] RomFields::Field
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initListData(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size, bool doResize,
	const RomFields::Field *field)
{
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	assert(field->type == RomFields::RFT_LISTDATA);
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_LISTDATA)
		return 0;

	const auto &listDataDesc = field->desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	const auto list_data = field->data.list_data.data;
	assert(list_data != nullptr);

	// Validate flags.
	// Cannot have both checkboxes and icons.
	const bool hasCheckboxes = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_CHECKBOXES);
	const bool hasIcons = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_ICONS);
	assert(!(hasCheckboxes && hasIcons));
	if (hasCheckboxes && hasIcons) {
		// Both are set. This shouldn't happen...
		return 0;
	}

	if (hasIcons) {
		assert(field->data.list_data.mxd.icons != nullptr);
		if (!field->data.list_data.mxd.icons) {
			// No icons vector...
			return 0;
		}
	}

	// Create a ListView widget.
	// NOTE: Separate row option is handled by the caller.
	// TODO: Enable sorting?
	// TODO: Optimize by not using OR?
	DWORD lvsStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_ALIGNLEFT |
	                 LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER | LVS_OWNERDATA;
	if (!listDataDesc.names) {
		lvsStyle |= LVS_NOCOLUMNHEADER;
	}
	const unsigned int dlgId = IDC_RFT_LISTDATA(idx);
	HWND hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
		WC_LISTVIEW, nullptr, lvsStyle,
		pt_start.x, pt_start.y,
		size.cx, size.cy,
		hWndTab, (HMENU)(INT_PTR)(dlgId),
		nullptr, nullptr);
	SetWindowFont(hDlgItem, hFontDlg, false);
	hwndListViewControls.push_back(hDlgItem);

	// Set extended ListView styles.
	DWORD lvsExStyle = LVS_EX_FULLROWSELECT;
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		// Not RDP (or is RemoteFX): Enable double buffering.
		lvsExStyle |= LVS_EX_DOUBLEBUFFER;
	}
	if (hasCheckboxes) {
		lvsExStyle |= LVS_EX_CHECKBOXES;
	}
	ListView_SetExtendedListViewStyle(hDlgItem, lvsExStyle);

	// Insert columns.
	int col_count = 1;
	if (listDataDesc.names) {
		col_count = (int)listDataDesc.names->size();
	} else {
		// No column headers.
		// Use the first row.
		if (list_data && !list_data->empty()) {
			col_count = (int)list_data->at(0).size();
		}
	}

	// Column widths.
	// LVSCW_AUTOSIZE_USEHEADER doesn't work for entries with newlines.
	// TODO: Use ownerdraw instead? (WM_MEASUREITEM / WM_DRAWITEM)
	unique_ptr<int[]> col_width(new int[col_count]);

	LVCOLUMN lvColumn;
	if (listDataDesc.names) {
		// Format table.
		// All values are known to fit in uint8_t.
		static const uint8_t align_tbl[4] = {
			// Order: TXA_D, TXA_L, TXA_C, TXA_R
			LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_CENTER, LVCFMT_RIGHT
		};

		// NOTE: ListView header alignment matches data alignment.
		// We'll prefer the data alignment value.
		uint32_t align = listDataDesc.alignment.data;
		auto iter = listDataDesc.names->cbegin();
		for (int i = 0; i < col_count; ++iter, i++, align >>= 2) {
			lvColumn.mask = LVCF_TEXT | LVCF_FMT;
			lvColumn.fmt = align_tbl[align & 3];

			const string &str = *iter;
			if (!str.empty()) {
				// NOTE: pszText is LPTSTR, not LPCTSTR...
				const tstring tstr = U82T_s(str);
				lvColumn.pszText = const_cast<LPTSTR>(tstr.c_str());
				ListView_InsertColumn(hDlgItem, i, &lvColumn);
			} else {
				// Don't show this column.
				// FIXME: Zero-width column is a bad hack...
				lvColumn.pszText = _T("");
				lvColumn.mask |= LVCF_WIDTH;
				lvColumn.cx = 0;
				ListView_InsertColumn(hDlgItem, i, &lvColumn);
			}
			col_width[i] = LVSCW_AUTOSIZE_USEHEADER;
		}
	} else {
		lvColumn.mask = LVCF_FMT;
		lvColumn.fmt = LVCFMT_LEFT;
		for (int i = 0; i < col_count; i++) {
			ListView_InsertColumn(hDlgItem, i, &lvColumn);
			col_width[i] = LVSCW_AUTOSIZE_USEHEADER;
		}
	}

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	AutoGetDC hDC(hWndTab, hFontDlg);

	// Add the row data.
	if (list_data) {
		uint32_t checkboxes = 0, adj_checkboxes = 0;
		if (hasCheckboxes) {
			checkboxes = field->data.list_data.mxd.checkboxes;
		}

		// NOTE: We're converting the strings for use with
		// LVS_OWNERDATA.
		vector<vector<tstring> > lvStringData;
		lvStringData.reserve(list_data->size());

		vector<int> lvImageList;
		if (hasIcons) {
			lvImageList.reserve(list_data->size());
		}

		int lv_row_num = 0, data_row_num = 0;
		int nl_max = 0;	// Highest number of newlines in any string.
		for (auto iter = list_data->cbegin(); iter != list_data->cend(); ++iter, data_row_num++) {
			const vector<string> &data_row = *iter;
			// FIXME: Skip even if we don't have checkboxes?
			// (also check other UI frontends)
			if (hasCheckboxes) {
				if (data_row.empty()) {
					// Skip this row.
					checkboxes >>= 1;
					continue;
				} else {
					// Store the checkbox value for this row.
					if (checkboxes & 1) {
						adj_checkboxes |= (1 << lv_row_num);
					}
					checkboxes >>= 1;
				}
			}

			// Destination row.
			lvStringData.resize(lvStringData.size()+1);
			auto &lv_row_data = lvStringData.at(lvStringData.size()-1);
			lv_row_data.reserve(data_row.size());

			// String data.
			int col = 0;
			for (auto iter = data_row.cbegin(); iter != data_row.cend(); ++iter, col++) {
				tstring tstr = U82T_s(*iter);

				// Count newlines.
				size_t prev_nl_pos = 0;
				size_t cur_nl_pos;
				int nl = 0;
				// TODO: Actual padding value?
				static const int COL_WIDTH_PADDING = 8*2;
				while ((cur_nl_pos = tstr.find(_T('\n'), prev_nl_pos)) != tstring::npos) {
					// Measure the width, plus padding on both sides.
					//
					// LVSCW_AUTOSIZE_USEHEADER doesn't work for entries with newlines.
					// This allows us to set a good initial size, but it won't help if
					// someone double-clicks the column splitter, triggering an automatic
					// resize.
					//
					// TODO: Use ownerdraw instead? (WM_MEASUREITEM / WM_DRAWITEM)
					// NOTE: Not using LibWin32Common::measureTextSize()
					// because that does its own newline checks.
					// TODO: Verify the values here.
					if (col < col_count) {
						SIZE textSize;
						GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(cur_nl_pos - prev_nl_pos), &textSize);
						col_width[col] = std::max<int>(col_width[col], textSize.cx + COL_WIDTH_PADDING);
					}

					nl++;
					prev_nl_pos = cur_nl_pos + 1;
				}
				if (nl > 0 && col < col_count) {
					// Measure the last line.
					// TODO: Verify the values here.
					SIZE textSize;
					GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(tstr.size() - prev_nl_pos), &textSize);
					col_width[col] = std::max<int>(col_width[col], textSize.cx + COL_WIDTH_PADDING);
				}
				nl_max = std::max(nl_max, nl);

				// TODO: Store the icon index if necessary.
				lv_row_data.push_back(std::move(tstr));
			}

			// Next row.
			lv_row_num++;
		}

		// Icons.
		if (hasIcons) {
			// TODO: Ideal icon size?
			// Using 32x32 for now.
			// ImageList will resize the original icons to 32x32.

			// NOTE: LVS_REPORT doesn't allow variable row sizes,
			// at least not without using ownerdraw. Hence, we'll
			// use a hackish workaround: If any entry has more than
			// two newlines, increase the Imagelist icon size by
			// 16 pixels.
			// TODO: Handle this better.
			// FIXME: This only works if the RFT_LISTDATA has icons.
			SIZE szLstIcon = {32, 32};
			bool resizeNeeded = false;
			float factor = 1.0f;
			if (nl_max >= 2) {
				// Two or more newlines.
				// Add 16px per newline over 1.
				szLstIcon.cy += (16 * (nl_max - 1));
				resizeNeeded = true;
				factor = (float)szLstIcon.cy / 32.0f;
			}

			HIMAGELIST himl = ImageList_Create(szLstIcon.cx, szLstIcon.cy,
				ILC_COLOR32, static_cast<int>(list_data->size()), 0);
			assert(himl != nullptr);
			if (himl) {
				// NOTE: ListView uses LVSIL_SMALL for LVS_REPORT.
				// TODO: The row highlight doesn't surround the empty area
				// of the icon. LVS_OWNERDRAW is probably needed for that.
				ListView_SetImageList(hDlgItem, himl, LVSIL_SMALL);
				uint32_t lvBgColor[2];
				lvBgColor[0] = LibWin32Common::GetSysColor_ARGB32(COLOR_WINDOW);
				lvBgColor[1] = LibWin32Common::getAltRowColor_ARGB32();

				// Add icons.
				const auto &icons = field->data.list_data.mxd.icons;
				uint8_t rowColorIdx = 0;
				for (auto iter = icons->cbegin(); iter != icons->cend();
				     ++iter, rowColorIdx = !rowColorIdx)
				{
					int iImage = -1;
					const rp_image *const icon = *iter;
					if (!icon) {
						// No icon for this row.
						lvImageList.push_back(iImage);
						continue;
					}

					// Resize the icon, if necessary.
					rp_image *icon_resized = nullptr;
					if (resizeNeeded) {
						SIZE szResize = {icon->width(), icon->height()};
						szResize.cy = static_cast<LONG>(szResize.cy * factor);

						// If the original icon is CI8, it needs to be
						// converted to ARGB32 first. Otherwise, the
						// "empty" background area will be black.
						// NOTE: We still need to specify a background color,
						// since the ListView highlight won't show up on
						// alpha-transparent pixels.
						// TODO: Handle this in rp_image::resized()?
						// TODO: Handle theme changes?
						// TODO: Error handling.
						if (icon->format() != rp_image::FORMAT_ARGB32) {
							rp_image *const icon32 = icon->dup_ARGB32();
							if (icon32) {
								icon_resized = icon32->resized(szResize.cx, szResize.cy,
									rp_image::AlignVCenter, lvBgColor[rowColorIdx]);
								delete icon32;
							}
						}

						// If the icon wasn't in ARGB32 format, it was resized above.
						// If it was already in ARGB32 format, it will be resized here.
						if (!icon_resized) {
							icon_resized = icon->resized(szResize.cx, szResize.cy,
								rp_image::AlignVCenter, lvBgColor[rowColorIdx]);
						}
					}

					HICON hIcon;
					if (icon_resized) {
						hIcon = RpImageWin32::toHICON(icon_resized);
						delete icon_resized;
					} else {
						hIcon = RpImageWin32::toHICON(icon);
					}

					if (hIcon) {
						int idx = ImageList_AddIcon(himl, hIcon);
						if (idx >= 0) {
							// Icon added.
							iImage = idx;
						}
						// ImageList makes a copy of the icon.
						DestroyIcon(hIcon);
					}

					// Save the ImageList index for later.
					lvImageList.push_back(iImage);
				}
			}
		}

		// Adjusted checkboxes value.
		if (hasCheckboxes) {
			map_lvCheckboxes.insert(std::make_pair(dlgId, adj_checkboxes));
		}
		// ImageList indexes.
		if (hasIcons) {
			map_lvImageList.insert(std::make_pair(dlgId, std::move(lvImageList)));
		}

		// Set the virtual list item count.
		ListView_SetItemCountEx(hDlgItem, lv_row_num,
			LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

		// Save the double-vector for later.
		map_lvStringData.insert(std::make_pair(dlgId, std::move(lvStringData)));
	}

	// Resize all of the columns.
	// TODO: Do this on system theme change?
	// TODO: Add a flag for 'main data column' and adjust it to
	// not exceed the viewport.
	for (int i = 0; i < col_count; i++) {
		ListView_SetColumnWidth(hDlgItem, i, col_width[i]);
	}

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlg, &dlgMargin);

	// Increase the ListView height.
	// Default: 5 rows, plus the header.
	// FIXME: Might not work for LVS_OWNERDATA.
	int cy = 0;
	if (doResize && ListView_GetItemCount(hDlgItem) > 0) {
		if (listDataDesc.names) {
			// Get the header rect.
			HWND hHeader = ListView_GetHeader(hDlgItem);
			assert(hHeader != nullptr);
			if (hHeader) {
				RECT rectHeader;
				GetClientRect(hHeader, &rectHeader);
				cy = rectHeader.bottom;
			}
		}

		// Get an item rect.
		RECT rectItem;
		ListView_GetItemRect(hDlgItem, 0, &rectItem, LVIR_BOUNDS);
		const int item_cy = (rectItem.bottom - rectItem.top);
		if (item_cy > 0) {
			// Multiply by the requested number of visible rows.
			int rows_visible = field->desc.list_data.rows_visible;
			if (rows_visible == 0) {
				rows_visible = 5;
			}
			cy += (item_cy * rows_visible);
			// Add half of the dialog margin.
			// TODO Get the ListView border size?
			cy += (dlgMargin.top/2);
		} else {
			// TODO: Can't handle this case...
			cy = size.cy;
		}
	} else {
		// TODO: Can't handle this if no items are present.
		cy = size.cy;
	}

	// TODO: Skip this if cy == size.cy?
	SetWindowPos(hDlgItem, nullptr, 0, 0, size.cx, cy,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	return cy;
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
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	assert(field->type == RomFields::RFT_DATETIME);
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_DATETIME)
		return 0;

	if (field->data.date_time == -1) {
		// Invalid date/time.
		return initString(hDlg, hWndTab, pt_start, idx, size, field,
			U82T_c(C_("RomDataView", "Unknown")));
	}

	// Format the date/time using the system locale.
	TCHAR dateTimeStr[256];
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
		int ret;
		if (field->desc.flags & RomFields::RFT_DATETIME_NO_YEAR) {
			// TODO: Localize this.
			// TODO: Windows 10 has DATE_MONTHDAY.
			ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				0, &st, _T("MMM d"), &dateTimeStr[start_pos], cchBuf);
		} else {
			ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				DATE_SHORTDATE,
				&st, nullptr, &dateTimeStr[start_pos], cchBuf);
		}
		if (ret <= 0) {
			// Error!
			return 0;
		}

		// Adjust the buffer position.
		// NOTE: ret includes the NULL terminator.
		start_pos += (ret-1);
		cchBuf -= (ret-1);
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
		start_pos += (ret-1);
		cchBuf -= (ret-1);
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
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	assert(field->type == RomFields::RFT_AGE_RATINGS);
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_AGE_RATINGS)
		return 0;

	const RomFields::age_ratings_t *age_ratings = field->data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		return initString(hDlg, hWndTab, pt_start, idx, size, field,
			U82T_c(C_("RomDataView", "ERROR")));
	}

	// Convert the age ratings field to a string.
	string str = RomFields::ageRatingsDecode(age_ratings);
	// Initialize the string field.
	return initString(hDlg, hWndTab, pt_start, idx, size, field, U82T_s(str));
}

/**
 * Initialize a Dimensions field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param idx		[in] Field index.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDimensions(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, int idx, const SIZE &size,
	const RomFields::Field *field)
{
	assert(hDlg != nullptr);
	assert(hWndTab != nullptr);
	assert(field != nullptr);
	assert(field->type == RomFields::RFT_DIMENSIONS);
	if (!hDlg || !hWndTab || !field)
		return 0;
	if (field->type != RomFields::RFT_DIMENSIONS)
		return 0;

	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field->data.dimensions;
	TCHAR tbuf[64];
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			_sntprintf(tbuf, ARRAY_SIZE(tbuf), _T("%dx%dx%d"),
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			_sntprintf(tbuf, ARRAY_SIZE(tbuf), _T("%dx%d"),
				dimensions[0], dimensions[1]);
		}
	} else {
		_sntprintf(tbuf, ARRAY_SIZE(tbuf), _T("%d"), dimensions[0]);
	}

	// Initialize the string field.
	return initString(hDlg, hWndTab, pt_start, idx, size, field, tbuf);
}

/**
 * Initialize the bold font.
 * @param hFont Base font.
 */
void RP_ShellPropSheetExt_Private::initBoldFont(HFONT hFont)
{
	assert(hFont != nullptr);
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
	assert(hFont != nullptr);
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

		// Find a monospaced font.
		int ret = LibWin32Common::findMonospacedFont(&lfFontMono);
		if (ret != 0) {
			// Monospaced font not found.
			return;
		}
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
	std::for_each(hwndMonoControls.cbegin(), hwndMonoControls.cend(),
		[hFontMonoNew](HWND hWnd) { SetWindowFont(hWnd, hFontMonoNew, false); }
	);

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
	assert(hDlg != nullptr);
	assert(romData != nullptr);
	if (!hDlg || !romData) {
		// No dialog, or no ROM data loaded.
		return;
	}

	// Get the fields.
	const RomFields *fields = romData->fields();
	assert(fields != nullptr);
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
	if (!hFontDlg) {
		hFontDlg = GetWindowFont(hDlg);
	}
	AutoGetDC hDC(hDlg, hFontDlg);

	// Initialize the bold and monospaced fonts.
	initBoldFont(hFontDlg);
	initMonospacedFont(hFontDlg);

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> t_desc_text;
	t_desc_text.reserve(count);

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	int max_text_width = 0;
	SIZE textSize;

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");
	for (int i = 0; i < count; i++) {
		const RomFields::Field *field = fields->field(i);
		assert(field != nullptr);
		if (!field || !field->isValid) {
			t_desc_text.push_back(tstring());
			continue;
		} else if (field->name.empty()) {
			t_desc_text.push_back(tstring());
			continue;
		}

		const tstring desc_text = U82T_s(rp_sprintf(
			desc_label_fmt, field->name.c_str()));

		// Get the width of this specific entry.
		// TODO: Use measureTextSize()?
		if (field->desc.flags & RomFields::STRF_WARNING) {
			// Label is bold. Use hFontBold.
			HFONT hFontOrig = SelectFont(hDC, hFontBold);
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
			SelectFont(hDC, hFontOrig);
		} else {
			// Regular font.
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
		}

		if (textSize.cx > max_text_width) {
			max_text_width = textSize.cx;
		}

		// Save for later.
		t_desc_text.push_back(std::move(desc_text));
	}

	// Add additional spacing between the ':' and the field.
	// TODO: Use measureTextSize()?
	// TODO: Reduce to 1 space?
	GetTextExtentPoint32(hDC, _T("  "), 2, &textSize);
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
	int tabCount = fields->tabCount();
	if (tabCount > 1) {
		// Increase the tab widget width by half of the margin.
		InflateRect(&dlgRect, dlgMargin.left/2, 0);
		dlgSize.cx += dlgMargin.left - 1;
		// TODO: Do this regardless of tabs?
		// NOTE: Margin with this change on Win7 is now 9px left, 12px bottom.
		dlgRect.bottom = fullDlgRect.bottom - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;

		// Create the tab widget.
		tabs.resize(tabCount);
		hTabWidget = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_TABCONTROL, nullptr,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			dlgRect.left, dlgRect.top, dlgSize.cx, dlgSize.cy,
			hDlg, (HMENU)(INT_PTR)IDC_TAB_WIDGET,
			nullptr, nullptr);
		SetWindowFont(hTabWidget, hFontDlg, false);
		curTabIndex = 0;

		// Add tabs.
		// NOTE: Tabs must be added *before* calling TabCtrl_AdjustRect();
		// otherwise, the tab bar height won't be taken into account.
		TCITEM tcItem;
		tcItem.mask = TCIF_TEXT;
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *name = fields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}
			const tstring tstr = U82T_c(name);
			tcItem.pszText = const_cast<LPTSTR>(tstr.c_str());
			// FIXME: Does the index work correctly if a tab is skipped?
			TabCtrl_InsertItem(hTabWidget, i, &tcItem);
		}

		// Adjust the dialog size for subtabs.
		TabCtrl_AdjustRect(hTabWidget, false, &dlgRect);
		// Update dlgSize.
		dlgSize.cx = dlgRect.right - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;
		// Update dlg_value_width.
		// FIXME: Results in 9px left, 8px right margins for RFT_LISTDATA.
		dlg_value_width = dlgSize.cx - descSize.cx - dlgMargin.left - 1;

		// Create windows for each tab.
		DWORD swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW;
		for (int i = 0; i < tabCount; i++) {
			if (!fields->tabName(i)) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];

			// Create a child dialog for the tab.
			tab.hDlg = CreateDialog(HINST_THISCOMPONENT,
				MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
				hDlg, SubtabDlgProc);
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
		tabCount = 1;
		tabs.resize(1);
		auto &tab = tabs[0];
		tab.hDlg = hDlg;
		tab.curPt = headerPt;
	}

	for (int idx = 0; idx < count; idx++) {
		const RomFields::Field *const field = fields->field(idx);
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

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, t_desc_text.at(idx).c_str(),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			tab.curPt.x, tab.curPt.y, descSize.cx, descSize.cy,
			tab.hDlg, (HMENU)(INT_PTR)(IDC_STATIC_DESC(idx)),
			nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, false);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {tab.curPt.x + descSize.cx, tab.curPt.y};
		switch (field->type) {
			case RomFields::RFT_INVALID:
				// No data here.
				field_cy = 0;
				break;

			case RomFields::RFT_STRING: {
				// String data.
				const SIZE size = {dlg_value_width, field_cy};
				field_cy = initString(hDlg, tab.hDlg, pt_start, idx, size, field, nullptr);
				break;
			}

			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(hDlg, tab.hDlg, pt_start, idx, field);
				break;

			case RomFields::RFT_LISTDATA: {
				// Create a ListView control.
				SIZE size = {dlg_value_width, field_cy*6};
				POINT pt_ListData = pt_start;

				// Should the RFT_LISTDATA be placed on its own row?
				bool doVBox = false;
				if (field->desc.list_data.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
					// Separate row.
					size.cx = dlgSize.cx - 1;
					// NOTE: This varies depending on if we have subtabs.
					if (tabCount > 1) {
						// Subtract the dialog margin.
						size.cx -= dlgMargin.left;
					} else {
						// Subtract another pixel.
						size.cx--;
					}
					pt_ListData.x = tab.curPt.x;
					pt_ListData.y += (descSize.cy - (dlgMargin.top/3));

					// If this is the last RFT_LISTDATA in the tab,
					// extend it vertically.
					if (tabIdx + 1 == tabCount && idx == count-1) {
						// Last tab, and last field.
						doVBox = true;
					} else {
						// Check if the next field is on the next tab.
						const RomFields::Field *nextField = fields->field(idx+1);
						if (nextField && nextField->tabIdx != tabIdx) {
							// Next field is on the next tab.
							doVBox = true;
						}
					}

					if (doVBox) {
						// Extend it vertically.
						size.cy = dlgSize.cy - pt_ListData.y;
						if (tabCount > 1) {
							// FIXME: This seems a bit wonky...
							size.cy -= ((dlgMargin.top / 2) + 1);
						} else {
							// This also seems wonky...
							size.cy += dlgRect.top - 1;
						}
					}
				}

				field_cy = initListData(hDlg, tab.hDlg, pt_ListData, idx, size, !doVBox, field);
				if (field_cy > 0) {
					// Add the extra row if necessary.
					if (field->desc.list_data.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
						const int szAdj = descSize.cy - (dlgMargin.top/3);
						field_cy += szAdj;
						// Reduce the hStatic size slightly.
						SetWindowPos(hStatic, nullptr, 0, 0, descSize.cx, szAdj,
							SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
					}
				}
				break;
			}

			case RomFields::RFT_DATETIME: {
				// Date/Time in Unix format.
				const SIZE size = {dlg_value_width, field_cy};
				field_cy = initDateTime(hDlg, tab.hDlg, pt_start, idx, size, field);
				break;
			}

			case RomFields::RFT_AGE_RATINGS: {
				// Age Ratings field.
				const SIZE size = {dlg_value_width, field_cy};
				field_cy = initAgeRatings(hDlg, tab.hDlg, pt_start, idx, size, field);
				break;
			}

			case RomFields::RFT_DIMENSIONS: {
				// Dimensions field.
				const SIZE size = {dlg_value_width, field_cy};
				field_cy = initDimensions(hDlg, tab.hDlg, pt_start, idx, size, field);
				break;
			}

			default:
				// Unsupported data type.
				assert(!"Unsupported RomFields::RomFieldsType.");
				field_cy = 0;
				break;
		}

		if (field_cy > 0) {
			// Next row.
			tab.curPt.y += field_cy;
		} else /* if (field_cy == 0) */ {
			// Failed to initialize the widget.
			// Remove the description label.
			DestroyWindow(hStatic);
		}
	}

	// Register for WTS session notifications. (Remote Desktop)
	wts.registerSessionNotification(hDlg, NOTIFY_FOR_THIS_SESSION);

	// Window is fully initialized.
	isFullyInit = true;
}

/** RP_ShellPropSheetExt **/

RP_ShellPropSheetExt::RP_ShellPropSheetExt()
	: d_ptr(nullptr)
{
	// NOTE: d_ptr is not initialized until we receive a valid
	// ROM file. This reduces overhead in cases where there are
	// lots of files with ROM-like file extensions but aren't
	// actually supported by rom-properties.
}

RP_ShellPropSheetExt::~RP_ShellPropSheetExt()
{
	delete d_ptr;
}

/** IUnknown **/
// Reference: https://msdn.microsoft.com/en-us/library/office/cc839627.aspx

IFACEMETHODIMP RP_ShellPropSheetExt::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
#pragma warning(push)
#pragma warning(disable: 4365 4838)
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellPropSheetExt, IShellExtInit),
		QITABENT(RP_ShellPropSheetExt, IShellPropSheetExt),
		{ 0 }
	};
#pragma warning(pop)
	return LibWin32Common::pQISearch(this, rgqit, riid, ppvObj);
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

	HRESULT hr = E_FAIL;
	UINT nFiles, cchFilename;
	TCHAR *tfilename = nullptr;
	RpFile *file = nullptr;
	RomData *romData = nullptr;

	string u8filename;
	const Config *config;

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

	tfilename = (TCHAR*)malloc((cchFilename+1) * sizeof(TCHAR));
	if (!tfilename) {
		// Memory allocation failed.
		goto cleanup;
	}
	cchFilename = DragQueryFile(hDrop, 0, tfilename, cchFilename+1);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	// Convert the filename to UTF-8.
	u8filename = T2U8(tfilename, cchFilename);

	// Check for "bad" file systems.
	config = Config::instance();
	if (LibRpBase::FileSystem::isOnBadFS(u8filename.c_str(),
	    config->enableThumbnailOnNetworkFS()))
	{
		// This file is on a "bad" file system.
		goto cleanup;
	}

	// Open the file.
	file = new RpFile(u8filename.c_str(), RpFile::FM_OPEN_READ_GZ);
	if (!file->isOpen()) {
		// Unable to open the file.
		goto cleanup;
	}

	// Get the appropriate RomData class for this ROM.
	// file is dup()'d by RomData.
	romData = RomDataFactory::create(file);
	if (!romData) {
		// Could not open the RomData object.
		goto cleanup;
	}

	// Unreference the RomData object.
	// We only want to open the RomData if the "ROM Properties"
	// tab is clicked, because otherwise the file will be held
	// open and may block the user from changing attributes.
	romData->unref();

	// Save the filename in the private class for later.
	if (!d_ptr) {
		d_ptr = new RP_ShellPropSheetExt_Private(this);
	}
	d_ptr->filename = T2U8(tfilename, cchFilename);

	hr = S_OK;

cleanup:
	if (file) {
		file->unref();
	}
	GlobalUnlock(stm.hGlobal);
	ReleaseStgMedium(&stm);
	free(tfilename);

	// If any value other than S_OK is returned from the method, the property 
	// sheet is not displayed.
	return hr;
}

/** IShellPropSheetExt **/

IFACEMETHODIMP RP_ShellPropSheetExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	if (!d_ptr) {
		// Not initialized.
		return E_FAIL;
	}

	// tr: Tab title.
	const tstring tsTabTitle = U82T_c(C_("RomDataView", "ROM Properties"));

	// Create a property sheet page.
	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTY_SHEET);
	psp.pszIcon = nullptr;
	psp.pszTitle = tsTabTitle.c_str();
	psp.pfnDlgProc = RP_ShellPropSheetExt_Private::DlgProc;
	psp.pcRefParent = nullptr;
	psp.pfnCallback = RP_ShellPropSheetExt_Private::CallbackProc;
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
	RP_UNUSED(uPageID);
	RP_UNUSED(pfnReplaceWith);
	RP_UNUSED(lParam);
	return E_NOTIMPL;
}

/** Property sheet callback functions. **/

/**
 * ListView GetDispInfo function.
 * @param plvdi	[in/out] NMLVDISPINFO
 * @return TRUE if handled; FALSE if not.
 */
inline BOOL RP_ShellPropSheetExtPrivate::ListView_GetDispInfo(NMLVDISPINFO *plvdi)
{
	// TODO: Assertions for row/column indexes.
	LVITEM *const plvItem = &plvdi->item;
	const unsigned int idFrom = static_cast<unsigned int>(plvdi->hdr.idFrom);
	bool ret = false;

	if (plvItem->mask & LVIF_TEXT) {
		// Fill in text.

		// Get the double-vector of strings for this ListView.
		auto lvIdIter = map_lvStringData.find(idFrom);
		if (lvIdIter != map_lvStringData.end()) {
			const auto &vv_str = lvIdIter->second;

			// Is this row in range?
			if (plvItem->iItem >= 0 && plvItem->iItem < static_cast<int>(vv_str.size())) {
				// Get the row data.
				const auto &row_data = vv_str.at(plvItem->iItem);

				// Is the column in range?
				if (plvItem->iSubItem >= 0 && plvItem->iSubItem < static_cast<int>(row_data.size())) {
					// Return the string data.
					_tcscpy_s(plvItem->pszText, plvItem->cchTextMax, row_data[plvItem->iSubItem].c_str());
					ret = true;
				}
			}
		}
	}

	if (plvItem->mask & LVIF_IMAGE) {
		// Fill in the ImageList index.
		// NOTE: Only valid for the base item.
		if (plvItem->iSubItem == 0) {
			// Check for checkboxes first.
			// LVS_OWNERDATA prevents LVS_EX_CHECKBOXES from working correctly,
			// so we have to implement it here.
			// Reference: https://www.codeproject.com/Articles/29064/CGridListCtrlEx-Grid-Control-Based-on-CListCtrl
			auto lvChkIdIter = map_lvCheckboxes.find(idFrom);
			if (lvChkIdIter != map_lvCheckboxes.end()) {
				// Found checkboxes.
				const uint32_t checkboxes = lvChkIdIter->second;

				// Enable state handling.
				plvItem->mask |= LVIF_STATE;
				plvItem->stateMask = LVIS_STATEIMAGEMASK;
				plvItem->state = INDEXTOSTATEIMAGEMASK(
					((checkboxes & (1 << plvItem->iItem)) ? 2 : 1));
				ret = true;
			} else {
				// No checkboxes. Check for ImageList.
				auto lvImgIdIter = map_lvImageList.find(idFrom);
				if (lvImgIdIter != map_lvImageList.end()) {
					const auto &lvImageList = lvImgIdIter->second;

					// Is this row in range?
					if (plvItem->iItem >= 0 && plvItem->iItem < static_cast<int>(lvImageList.size())) {
						const int iImage = lvImageList[plvItem->iItem];
						if (iImage >= 0) {
							// Set the ImageList index.
							plvItem->iImage = iImage;
							ret = true;
						}
					}
				}
			}
		}
	}

	return ret;
}

/**
 * ListView CustomDraw function.
 * @param plvcd	[in/out] NMLVCUSTOMDRAW
 * @return Return value.
 */
inline int RP_ShellPropSheetExt_Private::ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd)
{
	int result = CDRF_DODEFAULT;
	switch (plvcd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			// Request notifications for individual ListView items.
			result = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT: {
			// Set the background color for alternating row colors.
			if (plvcd->nmcd.dwItemSpec % 2) {
				// NOTE: plvcd->clrTextBk is set to 0xFF000000 here,
				// not the actual default background color.
				// FIXME: On Windows 7:
				// - Standard row colors are 19px high.
				// - Alternate row colors are 17px high. (top and bottom lines ignored?)
				plvcd->clrTextBk = colorAltRow;
				result = CDRF_NEWFONT;
			}
			break;
		}

		default:
			break;
	}
	return result;
}

/**
 * WM_NOTIFY handler for the property sheet.
 * @param hDlg Dialog window.
 * @param pHdr NMHDR
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr)
{
	INT_PTR ret = false;

	switch (pHdr->code) {
		case PSN_SETACTIVE:
			startAnimTimer();
			break;

		case PSN_KILLACTIVE:
			stopAnimTimer();
			break;

#ifdef UNICODE
		case NM_CLICK:
		case NM_RETURN: {
			// Check if this is a SysLink control.
			// NOTE: SysLink control only supports Unicode.
			if (hwndSysLinkControls.find(pHdr->hwndFrom) !=
			    hwndSysLinkControls.end())
			{
				// It's a SysLink control.
				// Open the URL.
				PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pHdr);
				ShellExecute(nullptr, _T("open"), pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
			}
			break;
		}
#endif /* UNICODE */

		case TCN_SELCHANGE: {
			// Tab change. Make sure this is the correct WC_TABCONTROL.
			if (hTabWidget != nullptr && hTabWidget == pHdr->hwndFrom) {
				// Tab widget. Show the selected tab.
				int newTabIndex = TabCtrl_GetCurSel(hTabWidget);
				ShowWindow(tabs[curTabIndex].hDlg, SW_HIDE);
				curTabIndex = newTabIndex;
				ShowWindow(tabs[newTabIndex].hDlg, SW_SHOW);
			}
			break;
		}

		case LVN_GETDISPINFO: {
			// Get data for an LVS_OWNERDRAW ListView.
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			ret = ListView_GetDispInfo(reinterpret_cast<NMLVDISPINFO*>(pHdr));
			break;
		}

		case NM_CUSTOMDRAW: {
			// Custom drawing notification.
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			// NOTE: Since this is a DlgProc, we can't simply return
			// the CDRF code. It has to be set as DWLP_MSGRESULT.
			// References:
			// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
			// - https://stackoverflow.com/a/40552426
			const int result = ListView_CustomDraw(reinterpret_cast<NMLVCUSTOMDRAW*>(pHdr));
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
			ret = true;
			break;
		}

		case LVN_ITEMCHANGING: {
			// If the window is fully initialized,
			// disable modification of checkboxes.
			// Reference: https://groups.google.com/forum/embed/#!topic/microsoft.public.vc.mfc/e9cbkSsiImA
			if (!isFullyInit)
				break;
			if ((pHdr->idFrom & 0xFC00) != IDC_RFT_LISTDATA(0))
				break;

			NMLISTVIEW *const pnmlv = reinterpret_cast<NMLISTVIEW*>(pHdr);
			const unsigned int state = (pnmlv->uOldState ^ pnmlv->uNewState) & LVIS_STATEIMAGEMASK;
			// Set result to true if the state difference is non-zero (i.e. it's changed).
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (state != 0));
			ret = true;
			break;
		}

		default:
			break;
	}

	return ret;
}

/**
 * WM_PAINT handler for the property sheet.
 * @param hDlg Dialog window.
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_PAINT(HWND hDlg)
{
	if (!hbmpBanner && !hbmpIconFrames[0]) {
		// Nothing to draw...
		return false;
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hDlg, &ps);

	// Memory DC for BitBlt.
	HDC hdcMem = CreateCompatibleDC(hdc);

	// Draw the banner.
	if (hbmpBanner) {
		SelectBitmap(hdcMem, hbmpBanner);
		BitBlt(hdc, ptBanner.x, ptBanner.y,
			szBanner.cx, szBanner.cy,
			hdcMem, 0, 0, SRCCOPY);
	}

	// Draw the icon.
	if (hbmpIconFrames[last_frame_number]) {
		SelectBitmap(hdcMem, hbmpIconFrames[last_frame_number]);
		BitBlt(hdc, rectIcon.left, rectIcon.top,
			szIcon.cx, szIcon.cy,
			hdcMem, 0, 0, SRCCOPY);
	}

	DeleteDC(hdcMem);
	EndPaint(hDlg, &ps);

	return true;
}

//
//   FUNCTION: FilePropPageDlgProc
//
//   PURPOSE: Processes messages for the property page.
//
INT_PTR CALLBACK RP_ShellPropSheetExt_Private::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Based on CppShellExtPropSheetHandler.
	// https://code.msdn.microsoft.com/windowsapps/CppShellExtPropSheetHandler-d93b49b7
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is 
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return true;

			// Access the property sheet extension from property page.
			RP_ShellPropSheetExt *const pExt = reinterpret_cast<RP_ShellPropSheetExt*>(pPage->lParam);
			if (!pExt)
				return true;
			RP_ShellPropSheetExt_Private *const d = pExt->d_ptr;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP, static_cast<HANDLE>(d));
			// Save handles for later.
			d->hDlgSheet = hDlg;

			// Dialog initialization is postponed to WM_SHOWWINDOW,
			// since some other extension (e.g. HashTab) may be
			// resizing the dialog.

			// NOTE: We're using WM_SHOWWINDOW instead of WM_SIZE
			// because WM_SIZE isn't sent for block devices,
			// e.g. CD-ROM drives.
			return true;
		}

		// FIXME: FBI's age rating is cut off on Windows
		// if we don't adjust for WM_SHOWWINDOW.
		case WM_SHOWWINDOW: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			if (d->isFullyInit) {
				// Dialog is already initialized.
				break;
			}

			// Open the RomData object.
			RpFile *const file = new RpFile(d->filename, RpFile::FM_OPEN_READ_GZ);
			if (!file->isOpen()) {
				// Unable to open the file.
				file->unref();
				break;
			}

			d->romData = RomDataFactory::create(file);
			file->unref();
			if (!d->romData) {
				// Unable to get a RomData object.
				break;
			} else if (!d->romData->isOpen()) {
				// RomData is not open.
				d->romData->unref();
				d->romData = nullptr;
				break;
			}

			// Load the images.
			d->loadImages();
			// Initialize the dialog.
			d->initDialog(hDlg);
			// We can close the RomData's underlying IRpFile now.
			d->romData->close();

			// Start the animation timer.
			d->startAnimTimer();

			// Continue normal processing.
			break;
		}

		case WM_DESTROY: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (d) {
				// Stop the animation timer.
				d->stopAnimTimer();
			}

			// FIXME: Remove D_PTR_PROP from child windows.
			// NOTE: WM_DESTROY is sent *before* child windows are destroyed.
			// WM_NCDESTROY is sent *after*.

			// Remove the D_PTR_PROP property from the page. 
			// The D_PTR_PROP property stored the pointer to the 
			// RP_ShellPropSheetExt_Private object.
			RemoveProp(hDlg, RP_ShellPropSheetExtPrivate::D_PTR_PROP);
			return true;
		}

		case WM_NOTIFY: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_NOTIFY(hDlg, reinterpret_cast<NMHDR*>(lParam));
		}

		case WM_PAINT: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}
			return d->DlgProc_WM_PAINT(hDlg);
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			// Reload the images.
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			// Reload images in case the background color changed.
			d->loadImages();
			// Reinitialize the alternate row color.
			d->colorAltRow = LibWin32Common::getAltRowColor();
			// Invalidate the banner and icon rectangles.
			if (d->hbmpBanner) {
				const RECT rectBitmap = {
					d->ptBanner.x, d->ptBanner.y,
					d->ptBanner.x + d->szBanner.cx,
					d->ptBanner.y + d->szBanner.cy,
				};
				InvalidateRect(d->hDlgSheet, &rectBitmap, false);
			}
			if (d->szIcon.cx > 0) {
				InvalidateRect(d->hDlgSheet, &d->rectIcon, false);
			}
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (d) {
				if (!d->hFontDlg) {
					// Dialog font hasn't been obtained yet.
					d->hFontDlg = GetWindowFont(hDlg);
				}
				d->initMonospacedFont(d->hFontDlg);
			}
			break;
		}

		case WM_CTLCOLORSTATIC: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, RP_ShellPropSheetExt_Private::D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			if (d->hwndWarningControls.find(reinterpret_cast<HWND>(lParam)) !=
			    d->hwndWarningControls.end())
			{
				// Set the "Warning" color.
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, RGB(255, 0, 0));
			}
			break;
		}

		case WM_WTSSESSION_CHANGE: {
			RP_ShellPropSheetExt_Private *const d = static_cast<RP_ShellPropSheetExt_Private*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			// If RDP was connected, disable ListView double-buffering.
			// If console (or RemoteFX) was connected, enable ListView double-buffering.
			switch (wParam) {
				case WTS_CONSOLE_CONNECT:
					std::for_each(d->hwndListViewControls.cbegin(), d->hwndListViewControls.cend(),
						[](HWND hWnd) {
							DWORD dwExStyle = ListView_GetExtendedListViewStyle(hWnd);
							dwExStyle |= LVS_EX_DOUBLEBUFFER;
							ListView_SetExtendedListViewStyle(hWnd, dwExStyle);
						}
					);
					break;
				case WTS_REMOTE_CONNECT:
					std::for_each(d->hwndListViewControls.cbegin(), d->hwndListViewControls.cend(),
						[](HWND hWnd) {
							DWORD dwExStyle = ListView_GetExtendedListViewStyle(hWnd);
							dwExStyle &= ~LVS_EX_DOUBLEBUFFER;
							ListView_SetExtendedListViewStyle(hWnd, dwExStyle);
						}
					);
					break;
				default:
					break;
			}
			break;
		}

		default:
			break;
	}

	return false; // Let system deal with other messages
}


//
//   FUNCTION: FilePropPageCallbackProc
//
//   PURPOSE: Specifies an application-defined callback function that a property
//            sheet calls when a page is created and when it is about to be 
//            destroyed. An application can use this function to perform 
//            initialization and cleanup operations for the page.
//
UINT CALLBACK RP_ShellPropSheetExt_Private::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return true to enable the page to be created.
			return true;
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

	return false;
}

/**
 * Animated icon timer.
 * @param hWnd
 * @param uMsg
 * @param idEvent
 * @param dwTime
 */
void CALLBACK RP_ShellPropSheetExt_Private::AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
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
		InvalidateRect(d->hDlgSheet, &d->rectIcon, false);
	}

	// Update the timer.
	// TODO: Verify that this affects the next callback.
	SetTimer(hWnd, idEvent, delay, AnimTimerProc);
}

/**
 * Dialog procedure for subtabs.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK RP_ShellPropSheetExt_Private::SubtabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Propagate NM_CUSTOMDRAW to the parent dialog.
	if (uMsg == WM_NOTIFY) {
		const NMHDR *const pHdr = reinterpret_cast<const NMHDR*>(lParam);
		switch (pHdr->code) {
			case LVN_GETDISPINFO:
			case NM_CUSTOMDRAW:
			case LVN_ITEMCHANGING: {
				// NOTE: Since this is a DlgProc, we can't simply return
				// the CDRF code. It has to be set as DWLP_MSGRESULT.
				// References:
				// - https://stackoverflow.com/questions/40549962/c-winapi-listview-nm-customdraw-not-getting-cdds-itemprepaint
				// - https://stackoverflow.com/a/40552426
				INT_PTR result = SendMessage(GetParent(hDlg), uMsg, wParam, lParam);
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, result);
				return true;
			}

			default:
				break;
		}
	}

	// Dummy callback procedure that does nothing.
	return false; // Let system deal with other messages
}
