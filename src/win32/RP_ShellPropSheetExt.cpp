/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt.cpp: IShellPropSheetExt implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
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

#include "DragImageLabel.hpp"
#include "FontHandler.hpp"
#include "MessageWidget.hpp"

// libwin32common
#include "libwin32common/AutoGetDC.hpp"
#include "libwin32common/SubclassWindow.h"
using LibWin32Common::AutoGetDC;
using LibWin32Common::WTSSessionNotification;

// NOTE: Using "RomDataView" for the libi18n context, since that
// matches what's used for the KDE and GTK+ frontends.

// librpbase, librpfile, librptexture, libromdata
#include "librpbase/RomFields.hpp"
#include "librpbase/SystemRegion.hpp"
#include "librpbase/TextOut.hpp"
#include "librpbase/img/RpPng.hpp"
#include "librpfile/win32/RpFile_windres.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using LibRpTexture::rp_image;
using LibRomData::RomDataFactory;

// C++ STL classes.
#include <fstream>
#include <sstream>
using std::array;
using std::ofstream;
using std::ostringstream;
using std::set;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::wstring;
using std::vector;

// Windows 10 and later
#ifndef DATE_MONTHDAY
# define DATE_MONTHDAY 0x00000080
#endif /* DATE_MONTHDAY */

// CLSID
const CLSID CLSID_RP_ShellPropSheetExt =
	{0x2443C158, 0xDF7C, 0x4352, {0xB4, 0x35, 0xBC, 0x9F, 0x88, 0x5F, 0xFD, 0x52}};

// Control base IDs.
#define IDC_STATIC_BANNER		0x0100
#define IDC_STATIC_ICON			0x0101
#define IDC_TAB_WIDGET			0x0102
#define IDC_CBO_LANGUAGE		0x0103
#define IDC_MESSAGE_WIDGET		0x0104
#define IDC_TAB_PAGE(idx)		(0x0200 + (idx))
#define IDC_STATIC_DESC(idx)		(0x1000 + (idx))
#define IDC_RFT_STRING(idx)		(0x1400 + (idx))
#define IDC_RFT_LISTDATA(idx)		(0x1800 + (idx))

// Bitfield is last due to multiple controls per field.
#define IDC_RFT_BITFIELD(idx, bit)	(0x7000 + ((idx) * 32) + (bit))

// "Options" menu item.
#define IDM_OPTIONS_MENU_BASE		0x8000
#define IDM_OPTIONS_MENU_EXPORT_TEXT	(IDM_OPTIONS_MENU_BASE - 1)
#define IDM_OPTIONS_MENU_EXPORT_JSON	(IDM_OPTIONS_MENU_BASE - 2)
#define IDM_OPTIONS_MENU_COPY_TEXT	(IDM_OPTIONS_MENU_BASE - 3)
#define IDM_OPTIONS_MENU_COPY_JSON	(IDM_OPTIONS_MENU_BASE - 4)

/** RP_ShellPropSheetExt_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellPropSheetExtPrivate RP_ShellPropSheetExt_Private

class RP_ShellPropSheetExt_Private
{
	public:
		explicit RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q, string &&filename);
		~RP_ShellPropSheetExt_Private();

	private:
		RP_DISABLE_COPY(RP_ShellPropSheetExt_Private)
	private:
		RP_ShellPropSheetExt *const q_ptr;

	public:
		// Property for "tab pointer".
		// This points to the RP_ShellPropSheetExt_Private::tab object.
		static const TCHAR TAB_PTR_PROP[];

	public:
		// ROM filename.
		string filename;
		// ROM data. (Not opened until the properties tab is shown.)
		RomData *romData;

		// Useful window handles.
		HWND hDlgSheet;		// Property sheet.
		HWND hBtnOptions;	// Options button.
		HMENU hMenuOptions;	// Options menu.
		tstring ts_prevExportDir;

		// Fonts.
		HFONT hFontDlg;		// Main dialog font.
		HFONT hFontBold;	// Bold font.
		FontHandler fontHandler;

		// ListView controls. (for toggling LVS_EX_DOUBLEBUFFER)
		vector<HWND> hwndListViewControls;

		// Header row widgets.
		HWND lblSysInfo;
		POINT ptSysInfo;
		RECT rectHeader;

		// wtsapi32.dll for Remote Desktop status. (WinXP and later)
		WTSSessionNotification wts;

		// Alternate row color.
		COLORREF colorAltRow;
		bool isFullyInit;		// True if the window is fully initialized.

		// ListView data struct.
		// NOTE: Not making vImageList a pointer, since that adds
		// significantly more complexity.
		struct LvData_t {
			vector<vector<tstring> > vvStr;	// String data.
			vector<int> vImageList;		// ImageList indexes.
			uint32_t checkboxes;		// Checkboxes.
			bool hasCheckboxes;		// True if checkboxes are valid.

			// For RFT_LISTDATA_MULTI only!
			HWND hListView;
			const RomFields::Field *pField;

			LvData_t()
				: checkboxes(0), hasCheckboxes(false)
				, hListView(nullptr), pField(nullptr) { }
		};

		// ListView data.
		// - Key: ListView dialog ID
		// - Value: LvData_t.
		unordered_map<uint16_t, LvData_t> map_lvData;

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

		// Banner and icon.
		DragImageLabel *lblBanner;
		DragImageLabel *lblIcon;

		// Is the UI locale right-to-left?
		// If so, this will be set to WS_EX_LAYOUTRTL.
		DWORD dwExStyleRTL;

		// Tab layout.
		HWND tabWidget;
		struct tab {
			HWND hDlg;		// Tab child dialog.
			HWND lblCredits;	// Credits label.
			POINT curPt;		// Current point.
			int scrollPos;		// Scrolling position.

			tab() : hDlg(nullptr), lblCredits(nullptr), scrollPos(0) {
				curPt.x = 0; curPt.y = 0;
			}
		};
		vector<tab> tabs;
		int curTabIndex;

		// Sizes.
		int lblDescHeight;	// Description label height.
		SIZE dlgSize;		// Visible dialog size.

		// MessageWidget for ROM operation notifications.
		HWND hMessageWidget;
		int iTabHeightOrig;

		// Multi-language functionality.
		uint32_t def_lc;	// Default language code from RomFields.
		set<uint32_t> set_lc;	// Set of supported language codes.
		HWND cboLanguage;
		HIMAGELIST himglFlags;

		/**
		 * Get the selected language code.
		 * @return Selected language code, or 0 for none (default).
		 */
		inline uint32_t sel_lc(void) const
		{
			uint32_t lc = 0;
			if (!cboLanguage) {
				// No language dropdown...
				return lc;
			}

			const int sel_idx = ComboBox_GetCurSel(cboLanguage);
			if (sel_idx >= 0) {
				lc = static_cast<uint32_t>(ComboBox_GetItemData(cboLanguage, sel_idx));
			}
			return lc;
		}

		// RFT_STRING_MULTI value labels.
		typedef std::pair<HWND, const RomFields::Field*> Data_StringMulti_t;
		vector<Data_StringMulti_t> vecStringMulti;

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
		 * Check if we need to use WS_EX_LAYOUTRTL.
		 * TODO: Cache this value?
		 * @return WS_EX_LAYOUTRTL if process is RTL; otherwise 0.
		 */
		static inline DWORD checkLayoutRTL(void)
		{
			// Set WS_EX_LAYOUTRTL if the process is RTL.
			DWORD dwDefaultLayout;
			const BOOL bRet = GetProcessDefaultLayout(&dwDefaultLayout);
			return (unlikely(bRet && dwDefaultLayout == LAYOUT_RTL))
				? WS_EX_LAYOUTRTL
				: 0;
		}

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
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @param str		[in,opt] String data. (If nullptr, field data is used.)
		 * @param pOutHWND	[out,opt] Retrieves the control's HWND.
		 * @return Field height, in pixels.
		 */
		int initString(_In_ HWND hDlg, _In_ HWND hWndTab,
			_In_ const POINT &pt_start, _In_ const SIZE &size,
			_In_ const RomFields::Field &field, _In_ int fieldIdx,
			_In_ LPCTSTR str = nullptr, _Outptr_opt_ HWND *pOutHWND = nullptr);

		/**
		 * Initialize a bitfield layout.
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initBitfield(HWND hDlg, HWND hWndTab,
			const POINT &pt_start,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Measure the width of a ListData string.
		 * This function handles newlines.
		 * @param hDC           [in] HDC for text measurement.
		 * @param tstr          [in] String to measure.
		 * @param pNlCount      [out,opt] Newline count.
		 * @return Width.
		 */
		static int measureListDataString(HDC hDC, const tstring &tstr, int *pNlCount = nullptr);

		/**
		 * Initialize a ListData field.
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a default ListView.
		 * @param doResize	[in] If true, resize the ListView to accomodate rows_visible.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initListData(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, const SIZE &size, bool doResize,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a Date/Time field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initDateTime(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, const SIZE &size,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize an Age Ratings field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initAgeRatings(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, const SIZE &size,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a Dimensions field.
		 * This function internally calls initString().
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initDimensions(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, const SIZE &size,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Initialize a multi-language string field.
		 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
		 * @param hWndTab	[in] Tab window. (for the actual control)
		 * @param pt_start	[in] Starting position, in pixels.
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @return Field height, in pixels.
		 */
		int initStringMulti(HWND hDlg, HWND hWndTab,
			const POINT &pt_start, const SIZE &size,
			const RomFields::Field &field, int fieldIdx);

		/**
		 * Build the cboLanguage image list.
		 */
		void buildCboLanguageImageList(void);

		/**
		 * Update all multi-language fields.
		 * @param user_lc User-specified language code.
		 */
		void updateMulti(uint32_t user_lc);

		/**
		 * Update a field's value.
		 * This is called after running a ROM operation.
		 * @param fieldIdx Field index.
		 * @return 0 on success; non-zero on error.
		 */
		int updateField(int fieldIdx);

		/**
		 * Initialize the bold font.
		 * @param hFont Base font.
		 */
		void initBoldFont(HFONT hFont);

	public:
		/**
		 * Initialize the dialog. (hDlgSheet)
		 * Called by WM_INITDIALOG.
		 */
		void initDialog(void);

		/**
		 * Adjust tabs for the message widget.
		 * Message widget must have been created first.
		 * Only run this after the message widget visibiliy has changed!
		 * @param bVisible True for visible; false for not.
		 */
		void adjustTabsForMessageWidgetVisibility(bool bVisible);

		/**
		 * Show the message widget.
		 * Message widget must have been created first.
		 * @param messageType Message type.
		 * @param lpszMsg Message.
		 */
		void showMessageWidget(unsigned int messageType, const TCHAR *lpszMsg);

		/**
		 * An "Options" menu action was triggered.
		 * @param menuId Menu ID. (Options ID + IDM_OPTIONS_MENU_BASE)
		 */
		void menuOptions_action_triggered(int menuId);

		/**
		 * Dialog subclass procedure to intercept WM_COMMAND for the "Options" button.
		 * @param hWnd		Dialog handle
		 * @param uMsg		Message
		 * @param wParam	WPARAM
		 * @param lParam	LPARAM
		 * @param uIdSubclass	Subclass ID (usually the control ID)
		 * @param dwRefData	RP_ShellPropSheetExt_Private*
		 */
		static LRESULT CALLBACK MainDialogSubclassProc(
			HWND hWnd, UINT uMsg,
			WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

		/**
		 * Create the "Options" button in the parent window.
		 * Called by WM_INITDIALOG.
		 */
		void createOptionsButton(void);

	private:
		// Internal functions used by the callback functions.
		INT_PTR DlgProc_WM_NOTIFY(HWND hDlg, NMHDR *pHdr);
		INT_PTR DlgProc_WM_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam);
		INT_PTR DlgProc_WM_PAINT(HWND hDlg);

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
};

/** RP_ShellPropSheetExt_Private **/

// Property for "tab pointer".
// This points to the RP_ShellPropSheetExt_Private::tab object.
const TCHAR RP_ShellPropSheetExt_Private::TAB_PTR_PROP[] = _T("RP_ShellPropSheetExt_Private::tab");

RP_ShellPropSheetExt_Private::RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q, string &&filename)
	: q_ptr(q)
	, filename(std::move(filename))
	, romData(nullptr)
	, hDlgSheet(nullptr)
	, hBtnOptions(nullptr)
	, hMenuOptions(nullptr)
	, hFontDlg(nullptr)
	, hFontBold(nullptr)
	, fontHandler(nullptr)
	, lblSysInfo(nullptr)
	, colorAltRow(0)
	, isFullyInit(false)
	, lblBanner(nullptr)
	, lblIcon(nullptr)
	, dwExStyleRTL(0)
	, tabWidget(nullptr)
	, curTabIndex(0)
	, lblDescHeight(0)
	, hMessageWidget(nullptr)
	, iTabHeightOrig(0)
	, def_lc(0)
	, cboLanguage(nullptr)
	, himglFlags(nullptr)
{
	// Initialize the alternate row color.
	colorAltRow = LibWin32Common::getAltRowColor();

	// Initialize other structs.
	dlgSize.cx = 0;
	dlgSize.cy = 0;

	// Check for RTL.
	// NOTE: Windows Explorer on Windows 7 seems to return 0 from GetProcessDefaultLayout(),
	// even if an RTL language is in use. We'll check the taskbar layout instead.
	// References:
	// - https://stackoverflow.com/questions/10391669/how-to-detect-if-a-windows-installation-is-rtl
	// - https://stackoverflow.com/a/10393376
	HWND hTaskBar = FindWindow(_T("Shell_TrayWnd"), nullptr);
	assert(hTaskBar != nullptr);
	if (hTaskBar) {
		dwExStyleRTL = static_cast<DWORD>(GetWindowLongPtr(hTaskBar, GWL_EXSTYLE)) & WS_EX_LAYOUTRTL;
	}
}

RP_ShellPropSheetExt_Private::~RP_ShellPropSheetExt_Private()
{
	// Delete the banner and icon frames.
	delete lblBanner;
	delete lblIcon;

	// Delete the popup menu.
	if (hMenuOptions) {
		DestroyMenu(hMenuOptions);
	}

	// Unreference the RomData object.
	UNREF(romData);

	// Destroy the flags ImageList.
	if (cboLanguage) {
		SendMessage(cboLanguage, CBEM_SETIMAGELIST, 0, (LPARAM)nullptr);
	}
	if (himglFlags) {
		ImageList_Destroy(himglFlags);
	}

	// Delete the fonts.
	if (hFontBold) {
		DeleteFont(hFontBold);
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
	// Supported image types.
	const uint32_t imgbf = romData->supportedImageTypes();

	// Banner.
	bool ok = false;
	if (imgbf & RomData::IMGBF_INT_BANNER) {
		// Get the banner.
		const rp_image *const banner = romData->image(RomData::IMG_INT_BANNER);
		if (banner && banner->isValid()) {
			if (!lblBanner) {
				lblBanner = new DragImageLabel(hDlgSheet);
				// TODO: Required size? For now, disabling scaling.
				lblBanner->setRequiredSize(0, 0);
			}

			ok = lblBanner->setRpImage(banner);
		}
	}
	if (!ok) {
		// No banner, or unable to load the banner.
		// Delete the DragImageLabel if it was created previously.
		delete lblBanner;
		lblBanner = nullptr;
	}

	// Icon.
	ok = false;
	if (imgbf & RomData::IMGBF_INT_ICON) {
		// Get the icon.
		const rp_image *const icon = romData->image(RomData::IMG_INT_ICON);
		if (icon && icon->isValid()) {
			if (!lblIcon) {
				lblIcon = new DragImageLabel(hDlgSheet);
			}

			// Is this an animated icon?
			ok = lblIcon->setIconAnimData(romData->iconAnimData());
			if (!ok) {
				// Not an animated icon, or invalid icon data.
				// Set the static icon.
				ok = lblIcon->setRpImage(icon);
			}
		}
	}
	if (!ok) {
		// No icon, or unable to load the icon.
		// Delete the DragImageLabel if it was created previously.
		delete lblBanner;
		lblBanner = nullptr;
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
	SIZE size_lblSysInfo = {0, 0};

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

	const tstring ts_sysInfo =
		LibWin32Common::unix2dos(U82T_s(rp_sprintf_p(
			// tr: %1$s == system name, %2$s == file type
			C_("RomDataView", "%1$s\n%2$s"), systemName, fileType)));

	if (!ts_sysInfo.empty()) {
		// Determine the appropriate label size.
		if (!LibWin32Common::measureTextSize(hDlg, hFont, ts_sysInfo, &size_lblSysInfo)) {
			// Start the total_widget_width.
			total_widget_width = size_lblSysInfo.cx;
		} else {
			// Error determining the label size.
			// Don't draw the label.
			size_lblSysInfo.cx = 0;
			size_lblSysInfo.cy = 0;
		}
	}

	// Add the banner and icon widths.

	// Banner.
	// TODO: Spacing between banner and text?
	// Doesn't seem to be needed with Dreamcast saves...
	const int banner_width = (lblBanner ? lblBanner->actualSize().cx : 0);
	total_widget_width += banner_width;

	// Icon.
	const int icon_width = (lblIcon ? lblIcon->actualSize().cx : 0);
	if (icon_width > 0) {
		if (total_widget_width > 0) {
			total_widget_width += pt_start.x;
		}
		total_widget_width += icon_width;
	}

	// Starting point.
	POINT curPt = {
		((size.cx - total_widget_width) / 2) + pt_start.x,
		pt_start.y
	};

	// lblSysInfo
	if (size_lblSysInfo.cx > 0 && size_lblSysInfo.cy > 0) {
		ptSysInfo.x = curPt.x;
		ptSysInfo.y = curPt.y;

		lblSysInfo = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, ts_sysInfo.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			ptSysInfo.x, ptSysInfo.y,
			size_lblSysInfo.cx, size_lblSysInfo.cy,
			hDlg, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysInfo, hFont, false);
		curPt.x += size_lblSysInfo.cx + pt_start.x;
	}

	// Banner.
	if (banner_width > 0) {
		lblBanner->setPosition(curPt);
		curPt.x += banner_width + pt_start.x;
	}

	// Icon.
	if (icon_width > 0) {
		lblIcon->setPosition(curPt);
		curPt.x += icon_width + pt_start.x;
	}

	// Return the label height and some extra padding.
	// TODO: Icon/banner height?
	return size_lblSysInfo.cy + (pt_start.y * 5 / 8);
}

/**
 * Initialize a string field. (Also used for Date/Time.)
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @param str		[in,opt] String data. (If nullptr, field data is used.)
 * @param pOutHWND	[out,opt] Retrieves the control's HWND.
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initString(_In_ HWND hDlg, _In_ HWND hWndTab,
	_In_ const POINT &pt_start, _In_ const SIZE &size,
	_In_ const RomFields::Field &field, _In_ int fieldIdx,
	_In_ LPCTSTR str, _Outptr_opt_ HWND *pOutHWND)
{
	if (pOutHWND) {
		// Clear the output HWND initially.
		*pOutHWND = nullptr;
	}

	// NOTE: libromdata uses Unix-style newlines.
	// For proper display on Windows, we have to
	// add carriage returns.

	// If string data wasn't specified, get the RFT_STRING data
	// from the RomFields::Field object.
	int lf_count = 0;
	tstring str_nl;
	if (!str) {
		if (field.type != RomFields::RFT_STRING)
			return 0;

		// NULL string == empty string
		if (field.data.str) {
			str_nl = LibWin32Common::unix2dos(U82T_s(*(field.data.str)), &lf_count);
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

	// Get the default font.
	HFONT hFont = hFontDlg;

	// Check for any formatting options.
	bool isWarning = false, isMonospace = false;
	if (field.type == RomFields::RFT_STRING) {
		// FIXME: STRF_MONOSPACE | STRF_WARNING is not supported.
		// Preferring STRF_WARNING.
		assert((field.desc.flags &
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING)) !=
			(RomFields::STRF_MONOSPACE | RomFields::STRF_WARNING));

		if (field.desc.flags & RomFields::STRF_WARNING) {
			// "Warning" font.
			if (hFontBold) {
				hFont = hFontBold;
				isWarning = true;
				// Set the font of the description control.
				HWND hStatic = GetDlgItem(hWndTab, IDC_STATIC_DESC(fieldIdx));
				if (hStatic) {
					SetWindowFont(hStatic, hFont, false);
					SetWindowLongPtr(hStatic, GWLP_USERDATA, static_cast<LONG_PTR>(RGB(255, 0, 0)));
				}
			}
		} else if (field.desc.flags & RomFields::STRF_MONOSPACE) {
			// Monospaced font.
			isMonospace = true;
		}
	}

	// Dialog item.
	const uint16_t cId = IDC_RFT_STRING(fieldIdx);
	HWND hDlgItem;

	if (field.type == RomFields::RFT_STRING &&
	    (field.desc.flags & RomFields::STRF_CREDITS))
	{
		// Align to the bottom of the dialog and center-align the text.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT tmpRect = {7, 7, 8, 8};
		MapDialogRect(hWndTab, &tmpRect);
		RECT winRect;
		GetClientRect(hWndTab, &winRect);
		// NOTE: We need to move left by 1px.
		OffsetRect(&winRect, -1, 0);

		// There should be a maximum of one STRF_CREDITS per tab.
		auto &tab = tabs[field.tabIdx];
		assert(tab.lblCredits == nullptr);
		if (tab.lblCredits != nullptr) {
			// Duplicate credits label.
			return 0;
		}

		// Create a SysLink widget.
		// SysLink allows the user to click a link and
		// open a webpage. It does NOT allow highlighting.
		// TODO: SysLink + EDIT?
		// FIXME: Centered text alignment?
		// TODO: With subtabs:
		// - Verify behavior of LWS_TRANSPARENT.
		// - Show below subtabs.
#ifdef UNICODE
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_LINK, str_nl.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			0, 0, 0, 0,	// will be adjusted afterwards
			hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
		if (!hDlgItem)
#endif /* UNICODE */
		{
			// Unable to create a SysLink control
			// This might happen if this is an ANSI build
			// or if we're running on Windows 2000.

			// FIXME: Remove links from the text before creating
			// a plain-old WC_EDIT control.

			// Create a read-only EDIT control.
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

			hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
				WC_EDIT, str_nl.c_str(), dwStyle,
				0, 0, 0, 0,	// will be adjusted afterwards
				hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);

			// Subclass multi-line EDIT controls to work around Enter/Escape issues.
			// We're also subclassing single-line EDIT controls to disable the
			// initial selection. (DLGC_HASSETSEL)
			// Reference: http://blogs.msdn.com/b/oldnewthing/archive/2007/08/20/4470527.aspx
			// TODO: Error handling?
			SUBCLASSPROC proc = (dwStyle & ES_MULTILINE)
				? LibWin32Common::MultiLineEditProc
				: LibWin32Common::SingleLineEditProc;
			SetWindowSubclass(hDlgItem, proc,
				static_cast<UINT_PTR>(cId),
				reinterpret_cast<DWORD_PTR>(GetParent(hDlgSheet)));
		}

		tab.lblCredits = hDlgItem;
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
		// Create a read-only EDIT control.
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
		hDlgItem = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_EDIT, str_nl.c_str(), dwStyle,
			pt_start.x, pt_start.y,
			size.cx, field_cy,
			hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
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
			static_cast<UINT_PTR>(cId),
			reinterpret_cast<DWORD_PTR>(GetParent(hDlgSheet)));
	}

	// Save the control in the appropriate container, if necessary.
	if (isWarning) {
		SetWindowLongPtr(hDlgItem, GWLP_USERDATA, static_cast<LONG_PTR>(RGB(255, 0, 0)));
	}
	if (isMonospace) {
		fontHandler.addMonoControl(hDlgItem);
	}

	// Return the HWND if requested.
	if (pOutHWND) {
		*pOutHWND = hDlgItem;
	}

	return field_cy;
}

/**
 * Initialize a bitfield layout.
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initBitfield(HWND hDlg, HWND hWndTab,
	const POINT &pt_start,
	const RomFields::Field &field, int fieldIdx)
{
	// Checkbox size.
	// Reference: http://stackoverflow.com/questions/1164868/how-to-get-size-of-check-and-gap-in-check-box
	RECT rect_chkbox = {0, 0, 12+4, 11};
	MapDialogRect(hDlg, &rect_chkbox);

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	AutoGetDC hDC(hWndTab, hFontDlg);

	// Create a grid of checkboxes.
	const auto &bitfieldDesc = field.desc.bitfield;
	int count = (int)bitfieldDesc.names->size();
	assert(count <= 32);
	if (count > 32)
		count = 32;

	// Determine the available width for checkboxes.
	RECT rectDlg;
	GetClientRect(hWndTab, &rectDlg);
	// NOTE: We need to move left by 1px.
	OffsetRect(&rectDlg, -1, 0);
	const int max_width = rectDlg.right - pt_start.x;

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> tnames;
	tnames.reserve(count);
	std::for_each(bitfieldDesc.names->cbegin(), bitfieldDesc.names->cend(),
		[&tnames](const string &name) {
			if (name.empty()) {
				// Skip U82T_s() for empty strings.
				tnames.emplace_back(tstring());
			} else {
				tnames.emplace_back(U82T_s(name));
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
		col_widths.emplace_back(max_width);
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

	// WS_EX_LAYOUTRTL
	const DWORD dwExStyleRTL = checkLayoutRTL();

	row = 0; col = 0;
	auto iter = tnames.cbegin();
	uint32_t bitfield = field.data.bitfield;
	for (int bit = 0; bit < count; ++iter, bit++, bitfield >>= 1) {
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
		const uint16_t cId = IDC_RFT_BITFIELD(fieldIdx, bit);
		HWND hCheckBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_BUTTON, tname.c_str(),
			WS_CHILD | WS_TABSTOP | WS_VISIBLE | BS_CHECKBOX,
			pt.x, pt.y, chk_w, rect_chkbox.bottom,
			hWndTab, (HMENU)(INT_PTR)cId,
			nullptr, nullptr);
		SetWindowFont(hCheckBox, hFontDlg, false);

		// Set the checkbox state.
		Button_SetCheck(hCheckBox, (bitfield & 1) ? BST_CHECKED : BST_UNCHECKED);

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
 * Measure the width of a ListData string.
 * This function handles newlines.
 * @param hDC          [in] HDC for text measurement.
 * @param tstr         [in] String to measure.
 * @param pNlCount     [out,opt] Newline count.
 * @return Width.
 */
int RP_ShellPropSheetExt_Private::measureListDataString(HDC hDC, const tstring &tstr, int *pNlCount)
{
	// TODO: Actual padding value?
	static const int COL_WIDTH_PADDING = 8*2;

	// Measured width.
	int width = 0;

	// Count newlines.
	size_t prev_nl_pos = 0;
	size_t cur_nl_pos;
	int nl = 0;
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
		SIZE textSize;
		GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(cur_nl_pos - prev_nl_pos), &textSize);
		width = std::max<int>(width, textSize.cx + COL_WIDTH_PADDING);

		nl++;
		prev_nl_pos = cur_nl_pos + 1;
	}

	if (nl > 0) {
		// Measure the last line.
		// TODO: Verify the values here.
		SIZE textSize;
		GetTextExtentPoint32(hDC, &tstr[prev_nl_pos], (int)(tstr.size() - prev_nl_pos), &textSize);
		width = std::max<int>(width, textSize.cx + COL_WIDTH_PADDING);
	}

	if (pNlCount) {
		*pNlCount = nl;
	}

	// FIXME: Don't use LVSCW_AUTOSIZE_USEHEADER.
	// LVS_OWNERDATA doesn't handle this properly. (only gets what's onscreen)
	// TODO: Figure out the correct padding so the columns aren't truncated.
	return (nl > 0 ? width : LVSCW_AUTOSIZE_USEHEADER);
}

/**
 * Initialize a ListData field.
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a default ListView.
 * @param doResize	[in] If true, resize the ListView to accomodate rows_visible.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initListData(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, const SIZE &size, bool doResize,
	const RomFields::Field &field, int fieldIdx)
{
	const auto &listDataDesc = field.desc.list_data;
	// NOTE: listDataDesc.names can be nullptr,
	// which means we don't have any column headers.

	// Single language ListData_t.
	// For RFT_LISTDATA_MULTI, this is only used for row and column count.
	const RomFields::ListData_t *list_data;
	const bool isMulti = !!(listDataDesc.flags & RomFields::RFT_LISTDATA_MULTI);
	if (isMulti) {
		// Multiple languages.
		const auto *const multi = field.data.list_data.data.multi;
		assert(multi != nullptr);
		assert(!multi->empty());
		if (!multi || multi->empty()) {
			// No data...
			return 0;
		}

		list_data = &multi->cbegin()->second;
	} else {
		// Single language.
		list_data = field.data.list_data.data.single;
	}

	assert(list_data != nullptr);
	assert(!list_data->empty());
	if (!list_data || list_data->empty()) {
		// No list data...
		return 0;
	}

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
		assert(field.data.list_data.mxd.icons != nullptr);
		if (!field.data.list_data.mxd.icons) {
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
	const uint16_t cId = IDC_RFT_LISTDATA(fieldIdx);
	HWND hListView = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE | dwExStyleRTL,
		WC_LISTVIEW, nullptr, lvsStyle,
		pt_start.x, pt_start.y, size.cx, size.cy,
		hWndTab, (HMENU)(INT_PTR)cId, nullptr, nullptr);
	SetWindowFont(hListView, hFontDlg, false);
	hwndListViewControls.emplace_back(hListView);

	// Set extended ListView styles.
	DWORD lvsExStyle = LVS_EX_FULLROWSELECT;
	if (!GetSystemMetrics(SM_REMOTESESSION)) {
		// Not RDP (or is RemoteFX): Enable double buffering.
		lvsExStyle |= LVS_EX_DOUBLEBUFFER;
	}
	if (hasCheckboxes) {
		lvsExStyle |= LVS_EX_CHECKBOXES;
	}
	ListView_SetExtendedListViewStyle(hListView, lvsExStyle);

	// Insert columns.
	int colCount = 1;
	if (listDataDesc.names) {
		colCount = (int)listDataDesc.names->size();
	} else {
		// No column headers.
		// Use the first row.
		colCount = (int)list_data->at(0).size();
	}

	// Column widths.
	// LVSCW_AUTOSIZE_USEHEADER doesn't work for entries with newlines.
	// TODO: Use ownerdraw instead? (WM_MEASUREITEM / WM_DRAWITEM)
	unique_ptr<int[]> col_width(new int[colCount]);

	// Format table.
	// All values are known to fit in uint8_t.
	static const uint8_t align_tbl[4] = {
		// Order: TXA_D, TXA_L, TXA_C, TXA_R
		LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_CENTER, LVCFMT_RIGHT
	};

	// NOTE: ListView header alignment matches data alignment.
	// We'll prefer the data alignment value.
	uint32_t align = listDataDesc.alignment.data;

	LVCOLUMN lvColumn;
	if (listDataDesc.names) {
		auto iter = listDataDesc.names->cbegin();
		for (int i = 0; i < colCount; ++iter, i++, align >>= 2) {
			lvColumn.mask = LVCF_TEXT | LVCF_FMT;
			lvColumn.fmt = align_tbl[align & 3];

			const string &str = *iter;
			if (!str.empty()) {
				// NOTE: pszText is LPTSTR, not LPCTSTR...
				const tstring tstr = U82T_s(str);
				lvColumn.pszText = const_cast<LPTSTR>(tstr.c_str());
				ListView_InsertColumn(hListView, i, &lvColumn);
			} else {
				// Don't show this column.
				// FIXME: Zero-width column is a bad hack...
				lvColumn.pszText = _T("");
				lvColumn.mask |= LVCF_WIDTH;
				lvColumn.cx = 0;
				ListView_InsertColumn(hListView, i, &lvColumn);
			}
			col_width[i] = LVSCW_AUTOSIZE_USEHEADER;
		}
	} else {
		lvColumn.mask = LVCF_FMT;
		for (int i = 0; i < colCount; i++, align >>= 2) {
			lvColumn.fmt = align_tbl[align & 3];
			ListView_InsertColumn(hListView, i, &lvColumn);
			col_width[i] = LVSCW_AUTOSIZE_USEHEADER;
		}
	}

	// Dialog font and device context.
	// NOTE: Using the parent dialog's font.
	AutoGetDC hDC(hListView, hFontDlg);

	// Add the row data.
	uint32_t checkboxes = 0;
	if (hasCheckboxes) {
		checkboxes = field.data.list_data.mxd.checkboxes;
	}

	// NOTE: We're converting the strings for use with
	// LVS_OWNERDATA.
	vector<vector<tstring> > lvStringData;
	lvStringData.reserve(list_data->size());
	LvData_t lvData;
	lvData.vvStr.reserve(list_data->size());
	lvData.hasCheckboxes = hasCheckboxes;
	if (hasIcons) {
		lvData.vImageList.reserve(list_data->size());
	}

	int lv_row_num = 0, data_row_num = 0;
	int nl_max = 0;	// Highest number of newlines in any string.
	const auto list_data_cend = list_data->cend();
	for (auto iter = list_data->cbegin(); iter != list_data_cend; ++iter, data_row_num++) {
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
					lvData.checkboxes |= (1U << lv_row_num);
				}
				checkboxes >>= 1;
			}
		}

		// Destination row.
		lvData.vvStr.resize(lvData.vvStr.size()+1);
		auto &lv_row_data = lvData.vvStr.at(lvData.vvStr.size()-1);
		lv_row_data.reserve(data_row.size());

		// String data.
		if (isMulti) {
			// RFT_LISTDATA_MULTI. Allocate space for the strings,
			// but don't initialize them here.
			lv_row_data.resize(data_row.size());

			// Check newline counts in all strings to find nl_max.
			const auto *const multi = field.data.list_data.data.multi;
			const auto multi_cend = multi->cend();
			for (auto iter_m = multi->cbegin(); iter_m != multi_cend; ++iter_m) {
				const RomFields::ListData_t &ld = iter_m->second;
				const auto ld_cend = ld.cend();
				for (auto iter_row = ld.cbegin(); iter_row != ld_cend; ++iter_row) {
					const auto &data_row = *iter_row;
					const auto data_row_cend = data_row.cend();
					for (auto iter_col = data_row.cbegin(); iter_col != data_row_cend; ++iter_col) {
						size_t prev_nl_pos = 0;
						size_t cur_nl_pos;
						int nl = 0;
						while ((cur_nl_pos = iter_col->find('\n', prev_nl_pos)) != tstring::npos) {
							nl++;
							prev_nl_pos = cur_nl_pos + 1;
						}
						nl_max = std::max(nl_max, nl);
					}
				}
			}
		} else {
			// Single language.
			int col = 0;
			const auto data_row_cend = data_row.cend();
			for (auto iter = data_row.cbegin(); iter != data_row_cend; ++iter, col++) {
				tstring tstr = U82T_s(*iter);

				int nl_count;
				int width = measureListDataString(hDC, tstr, &nl_count);
				if (col < colCount) {
					col_width[col] = std::max(col_width[col], width);
				}
				nl_max = std::max(nl_max, nl_count);

				// TODO: Store the icon index if necessary.
				lv_row_data.emplace_back(std::move(tstr));
			}
		}

		// Next row.
		lv_row_num++;
	}

	// Icons.
	if (hasIcons) {
		// Icon size is 32x32, adjusted for DPI. (TODO: WM_DPICHANGED)
		// ImageList will resize the original icons to 32x32.

		// NOTE: LVS_REPORT doesn't allow variable row sizes,
		// at least not without using ownerdraw. Hence, we'll
		// use a hackish workaround: If any entry has more than
		// two newlines, increase the Imagelist icon size by
		// 16 pixels.
		// TODO: Handle this better.
		// FIXME: This only works if the RFT_LISTDATA has icons.
		const int px = rp_AdjustSizeForDpi(32, rp_GetDpiForWindow(hDlg));
		SIZE sizeListIcon = {px, px};
		bool resizeNeeded = false;
		float factor = 1.0f;
		if (nl_max >= 2) {
			// Two or more newlines.
			// Add half of the icon size per newline over 1.
			sizeListIcon.cy += ((px/2) * (nl_max - 1));
			resizeNeeded = true;
			factor = (float)sizeListIcon.cy / (float)px;
		}

		HIMAGELIST himl = ImageList_Create(sizeListIcon.cx, sizeListIcon.cy,
			ILC_COLOR32, static_cast<int>(list_data->size()), 0);
		assert(himl != nullptr);
		if (himl) {
			// NOTE: ListView uses LVSIL_SMALL for LVS_REPORT.
			// TODO: The row highlight doesn't surround the empty area
			// of the icon. LVS_OWNERDRAW is probably needed for that.
			ListView_SetImageList(hListView, himl, LVSIL_SMALL);
			uint32_t lvBgColor[2];
			lvBgColor[0] = LibWin32Common::GetSysColor_ARGB32(COLOR_WINDOW);
			lvBgColor[1] = LibWin32Common::getAltRowColor_ARGB32();

			// Add icons.
			uint8_t rowColorIdx = 0;
			const auto &icons = field.data.list_data.mxd.icons;
			const auto icons_cend = icons->cend();
			for (auto iter = icons->cbegin(); iter != icons_cend;
			     ++iter, rowColorIdx = !rowColorIdx)
			{
				bool needsUnref = false;
				int iImage = -1;
				const rp_image *icon = *iter;
				if (!icon) {
					// No icon for this row.
					lvData.vImageList.emplace_back(iImage);
					continue;
				}

				if (dwExStyleRTL != 0) {
					// WS_EX_LAYOUTRTL will flip bitmaps in the ListView.
					// ILC_MIRROR mirrors the bitmaps if the process is mirrored,
					// but we can't rely on that being the case, and this option
					// was first introduced in Windows XP.
					// We'll flip the image here to counteract it.
					const rp_image *const flipimg = icon->flip(rp_image::FLIP_H);
					assert(flipimg != nullptr);
					if (flipimg) {
						icon = flipimg;
						needsUnref = true;
					}
				}

				// Resize the icon, if necessary.
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
					if (icon->format() != rp_image::Format::ARGB32) {
						const rp_image *const icon32 = icon->dup_ARGB32();
						if (icon32) {
							if (needsUnref) {
								icon->unref();
							}
							icon = icon32;
							needsUnref = true;
						}
					}

					// Resize the icon.
					const rp_image *const icon_resized = icon->resized(
						szResize.cx, szResize.cy,
						rp_image::AlignVCenter, lvBgColor[rowColorIdx]);
					assert(icon_resized != nullptr);
					if (icon_resized) {
						if (needsUnref) {
							icon->unref();
						}
						icon = icon_resized;
						needsUnref = true;
					}
				}

				HICON hIcon = RpImageWin32::toHICON(icon);
				if (needsUnref) {
					icon->unref();
				}

				assert(hIcon != nullptr);
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
				lvData.vImageList.emplace_back(iImage);
			}
		}
	}

	if (isMulti) {
		lvData.hListView = hListView;
		lvData.pField = &field;
	}

	// Save the LvData_t.
	// TODO: Verify that std::move() works here.
	map_lvData.insert(std::make_pair(cId, std::move(lvData)));

	// Set the virtual list item count.
	ListView_SetItemCountEx(hListView, lv_row_num,
		LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	if (!isMulti) {
		// Resize all of the columns.
		// TODO: Do this on system theme change?
		// TODO: Add a flag for 'main data column' and adjust it to
		// not exceed the viewport.
		for (int i = 0; i < colCount; i++) {
			ListView_SetColumnWidth(hListView, i, col_width[i]);
		}
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
	if (doResize && ListView_GetItemCount(hListView) > 0) {
		if (listDataDesc.names) {
			// Get the header rect.
			HWND hHeader = ListView_GetHeader(hListView);
			assert(hHeader != nullptr);
			if (hHeader) {
				RECT rectListViewHeader;
				GetClientRect(hHeader, &rectListViewHeader);
				cy = rectListViewHeader.bottom;
			}
		}

		// Get an item rect.
		RECT rectItem;
		ListView_GetItemRect(hListView, 0, &rectItem, LVIR_BOUNDS);
		const int item_cy = (rectItem.bottom - rectItem.top);
		if (item_cy > 0) {
			// Multiply by the requested number of visible rows.
			int rows_visible = field.desc.list_data.rows_visible;
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
	SetWindowPos(hListView, nullptr, 0, 0, size.cx, cy,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	return cy;
}

/**
 * Initialize a Date/Time field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDateTime(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, const SIZE &size,
	const RomFields::Field &field, int fieldIdx)
{
	if (field.data.date_time == -1) {
		// Invalid date/time.
		return initString(hDlg, hWndTab, pt_start, size, field, fieldIdx,
			U82T_c(C_("RomDataView", "Unknown")));
	}

	// Format the date/time using the system locale.
	TCHAR dateTimeStr[256];
	int start_pos = 0;
	int cchBuf = _countof(dateTimeStr);

	// Convert from Unix time to Win32 SYSTEMTIME.
	SYSTEMTIME st;
	UnixTimeToSystemTime(field.data.date_time, &st);

	// At least one of Date and/or Time must be set.
	assert((field.desc.flags &
		(RomFields::RFT_DATETIME_HAS_DATE | RomFields::RFT_DATETIME_HAS_TIME)) != 0);

	if (!(field.desc.flags & RomFields::RFT_DATETIME_IS_UTC)) {
		// Convert to the current timezone.
		SYSTEMTIME st_utc = st;
		BOOL ret = SystemTimeToTzSpecificLocalTime(nullptr, &st_utc, &st);
		if (!ret) {
			// Conversion failed.
			return 0;
		}
	}

	if (field.desc.flags & RomFields::RFT_DATETIME_HAS_DATE) {
		// Format the date.
		int ret;
		if (field.desc.flags & RomFields::RFT_DATETIME_NO_YEAR) {
			// Try Windows 10's DATE_MONTHDAY first.
			ret = GetDateFormat(
				MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
				DATE_MONTHDAY,
				&st, nullptr, &dateTimeStr[start_pos], cchBuf);
			if (ret == 0) {
				// DATE_MONTHDAY failed.
				// Fall back to a hard-coded format string.
				// TODO: Localization.
				ret = GetDateFormat(
					MAKELCID(LOCALE_USER_DEFAULT, SORT_DEFAULT),
					0, &st, _T("MMM d"), &dateTimeStr[start_pos], cchBuf);
			}
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

	if (field.desc.flags & RomFields::RFT_DATETIME_HAS_TIME) {
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
	return initString(hDlg, hWndTab, pt_start, size, field, fieldIdx, dateTimeStr);
}

/**
 * Initialize an Age Ratings field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initAgeRatings(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, const SIZE &size,
	const RomFields::Field &field, int fieldIdx)
{
	const RomFields::age_ratings_t *const age_ratings = field.data.age_ratings;
	assert(age_ratings != nullptr);
	if (!age_ratings) {
		// No age ratings data.
		return initString(hDlg, hWndTab, pt_start, size, field, fieldIdx,
			U82T_c(C_("RomDataView", "ERROR")));
	}

	// Convert the age ratings field to a string.
	string str = RomFields::ageRatingsDecode(age_ratings);
	// Initialize the string field.
	return initString(hDlg, hWndTab, pt_start, size, field, fieldIdx, U82T_s(str));
}

/**
 * Initialize a Dimensions field.
 * This function internally calls initString().
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initDimensions(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, const SIZE &size,
	const RomFields::Field &field, int fieldIdx)
{
	// TODO: 'x' or '×'? Using 'x' for now.
	const int *const dimensions = field.data.dimensions;
	TCHAR tbuf[64];
	if (dimensions[1] > 0) {
		if (dimensions[2] > 0) {
			_sntprintf(tbuf, _countof(tbuf), _T("%dx%dx%d"),
				dimensions[0], dimensions[1], dimensions[2]);
		} else {
			_sntprintf(tbuf, _countof(tbuf), _T("%dx%d"),
				dimensions[0], dimensions[1]);
		}
	} else {
		_sntprintf(tbuf, _countof(tbuf), _T("%d"), dimensions[0]);
	}

	// Initialize the string field.
	return initString(hDlg, hWndTab, pt_start, size, field, fieldIdx, tbuf);
}

/**
 * Initialize a multi-language string field.
 * @param hDlg		[in] Parent dialog window. (for dialog unit mapping)
 * @param hWndTab	[in] Tab window. (for the actual control)
 * @param pt_start	[in] Starting position, in pixels.
 * @param size		[in] Width and height for a single line label.
 * @param field		[in] RomFields::Field
 * @param fieldIdx	[in] Field index
 * @return Field height, in pixels.
 */
int RP_ShellPropSheetExt_Private::initStringMulti(HWND hDlg, HWND hWndTab,
	const POINT &pt_start, const SIZE &size,
	const RomFields::Field &field, int fieldIdx)
{
	// Mutli-language string.
	// NOTE: The string contents won't be initialized here.
	// They will be initialized separately, since the user will
	// be able to change the displayed language.
	HWND lblStringMulti = nullptr;
	int field_cy = initString(hDlg, hWndTab, pt_start, size, field, fieldIdx,
		_T(""), &lblStringMulti);
	if (lblStringMulti) {
		vecStringMulti.emplace_back(std::make_pair(lblStringMulti, &field));
	}
	return field_cy;
}

/**
 * Build the cboLanguage image list.
 */
void RP_ShellPropSheetExt_Private::buildCboLanguageImageList(void)
{
	if (cboLanguage) {
		// Removing the existing ImageList first.
		SendMessage(cboLanguage, CBEM_SETIMAGELIST, 0, (LPARAM)nullptr);
	}
	if (himglFlags) {
		// Deleting the existing ImageList first.
		ImageList_Destroy(himglFlags);
		himglFlags = nullptr;
	}

	if (vecStringMulti.empty() || set_lc.size() <= 1) {
		// No multi-language string fields, or not enough
		// languages for cboLanguage.
		return;
	}

	// Get the icon size for the current DPI.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
	// TODO: Adjust cboLanguage if necessary?
	const UINT dpi = rp_GetDpiForWindow(hDlgSheet);
	unsigned int iconSize;
	uint16_t flagResource;
	if (dpi < 120) {
		// [96,120) dpi: Use 16x16.
		iconSize = 16;
		flagResource = IDP_FLAGS_16x16;
	} else if (dpi <= 144) {
		// [120,144] dpi: Use 24x24.
		// TODO: Maybe needs to be slightly higher?
		iconSize = 24;
		flagResource = IDP_FLAGS_24x24;
	} else {
		// >144dpi: Use 32x32.
		iconSize = 32;
		flagResource = IDP_FLAGS_32x32;
	}

	// Load the flags sprite sheet.
	// TODO: Is premultiplied alpha needed?
	// Reference: https://stackoverflow.com/questions/307348/how-to-draw-32-bit-alpha-channel-bitmaps
	RpFile_windres *const f_res = new RpFile_windres(HINST_THISCOMPONENT, MAKEINTRESOURCE(flagResource), MAKEINTRESOURCE(RT_PNG));
	if (!f_res->isOpen()) {
		// Unable to open the resource.
		f_res->unref();
		return;
	}

	rp_image *imgFlagsSheet = RpPng::loadUnchecked(f_res);
	f_res->unref();
	if (!imgFlagsSheet) {
		// Unable to load the flags sprite sheet.
		return;
	}
	const int flagStride = imgFlagsSheet->stride() / sizeof(uint32_t);

	// Make sure the bitmap has the expected size.
	assert(imgFlagsSheet->width() == (iconSize * SystemRegion::FLAGS_SPRITE_SHEET_COLS));
	assert(imgFlagsSheet->height() == (iconSize * SystemRegion::FLAGS_SPRITE_SHEET_ROWS));
	if (imgFlagsSheet->width() != (iconSize * SystemRegion::FLAGS_SPRITE_SHEET_COLS) ||
	    imgFlagsSheet->height() != (iconSize * SystemRegion::FLAGS_SPRITE_SHEET_ROWS))
	{
		// Incorrect size. We can't use it.
		imgFlagsSheet->unref();
		return;
	}

	if (dwExStyleRTL != 0) {
		// WS_EX_LAYOUTRTL will flip bitmaps in the dropdown box.
		// ILC_MIRROR mirrors the bitmaps if the process is mirrored,
		// but we can't rely on that being the case, and this option
		// was first introduced in Windows XP.
		// We'll flip the image here to counteract it.
		rp_image *const flipimg = imgFlagsSheet->flip(rp_image::FLIP_H);
		assert(flipimg != nullptr);
		if (flipimg) {
			imgFlagsSheet->unref();
			imgFlagsSheet = flipimg;
		}
	}

	// Create the image list.
	himglFlags = ImageList_Create(iconSize, iconSize, ILC_COLOR32, 13, 16);
	assert(himglFlags != nullptr);
	if (!himglFlags) {
		// Unable to create the ImageList.
		imgFlagsSheet->unref();
		return;
	}

	const BITMAPINFOHEADER bmihDIBSection = {
		sizeof(BITMAPINFOHEADER),	// biSize
		(LONG)iconSize,			// biWidth
		-(LONG)iconSize,		// biHeight (negative for right-side up)
		1,				// biPlanes
		32,				// biBitCount
		BI_RGB,				// biCompression
		0,				// biSizeImage
		(LONG)dpi,			// biXPelsPerMeter
		(LONG)dpi,			// biYPelsPerMeter
		0,				// biClrUsed
		0,				// biClrImportant
	};

	HDC hdcIcon = GetDC(nullptr);
	const auto set_lc_cend = set_lc.cend();
	for (auto iter = set_lc.cbegin(); iter != set_lc_cend; ++iter) {
		int col, row;
		int ret = SystemRegion::getFlagPosition(*iter, &col, &row);
		assert(ret == 0);
		if (ret != 0) {
			// Icon not found. Use a blank icon to prevent issues.
			col = 3;
			row = 3;
		}

		if (dwExStyleRTL != 0) {
			// Flag sprite sheet is flipped for RTL.
			col = 3 - col;
		}

		// Create a DIB section for the sub-icon.
		void *pvBits;
		HBITMAP hbmIcon = CreateDIBSection(
			hdcIcon,	// hdc
			reinterpret_cast<const BITMAPINFO*>(&bmihDIBSection),	// pbmi
			DIB_RGB_COLORS,	// usage
			&pvBits,	// ppvBits
			nullptr,	// hSection
			0);		// offset

		GdiFlush();	// TODO: Not sure if needed here...
		assert(hbmIcon != nullptr);
		if (hbmIcon) {
			// Copy the icon from the sprite sheet.
			const size_t rowBytes = iconSize * sizeof(uint32_t);
			const uint32_t *pSrc = static_cast<const uint32_t*>(imgFlagsSheet->scanLine(row * iconSize));
			pSrc += (col * iconSize);
			uint32_t *pDest = static_cast<uint32_t*>(pvBits);
			for (UINT bmRow = iconSize; bmRow > 0; bmRow--) {
				memcpy(pDest, pSrc, rowBytes);
				pDest += iconSize;
				pSrc += flagStride;
			}

			// Add the icon to the ImageList.
			GdiFlush();
			ImageList_Add(himglFlags, hbmIcon, nullptr);
			DeleteBitmap(hbmIcon);
		}
	}
	ReleaseDC(nullptr, hdcIcon);
	imgFlagsSheet->unref();

	if (cboLanguage) {
		// Set the new ImageList.
		SendMessage(cboLanguage, CBEM_SETIMAGELIST, 0, (LPARAM)himglFlags);
	}
}

/**
 * Update all multi-language fields.
 * @param user_lc User-specified language code.
 */
void RP_ShellPropSheetExt_Private::updateMulti(uint32_t user_lc)
{
	// RFT_STRING_MULTI
	const auto vecStringMulti_cend = vecStringMulti.cend();
	for (auto iter = vecStringMulti.cbegin();
	     iter != vecStringMulti_cend; ++iter)
	{
		const HWND lblString = iter->first;
		const RomFields::Field *const pField = iter->second;
		const auto *const pStr_multi = pField->data.str_multi;
		assert(pStr_multi != nullptr);
		assert(!pStr_multi->empty());
		if (!pStr_multi || pStr_multi->empty()) {
			// Invalid multi-string...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			const auto pStr_multi_cend = pStr_multi->cend();
			for (auto iter_sm = pStr_multi->cbegin();
			     iter_sm != pStr_multi_cend; ++iter_sm)
			{
				set_lc.insert(iter_sm->first);
			}
		}

		// Get the string and update the text.
		const string *const pStr = RomFields::getFromStringMulti(pStr_multi, def_lc, user_lc);
		assert(pStr != nullptr);
		if (pStr != nullptr) {
			SetWindowText(lblString, U82T_s(*pStr));
		} else {
			SetWindowText(lblString, _T(""));
		}
	}

	// RFT_LISTDATA_MULTI
	const auto map_lvData_end = map_lvData.end();
	for (auto iter = map_lvData.begin(); iter != map_lvData_end; ++iter) {
		LvData_t &lvData = iter->second;
		if (!lvData.hListView) {
			// Not an RFT_LISTDATA_MULTI.
			continue;
		}

		const HWND hListView = lvData.hListView;
		const RomFields::Field *const pField = lvData.pField;
		const auto *const pListData_multi = pField->data.list_data.data.multi;
		assert(pListData_multi != nullptr);
		assert(!pListData_multi->empty());
		if (!pListData_multi || pListData_multi->empty()) {
			// Invalid RFT_LISTDATA_MULTI...
			continue;
		}

		if (!cboLanguage) {
			// Need to add all supported languages.
			// TODO: Do we need to do this for all of them, or just one?
			const auto pListData_multi_cend = pListData_multi->cend();
			for (auto iter_sm = pListData_multi->cbegin();
			     iter_sm != pListData_multi_cend; ++iter_sm)
			{
				set_lc.insert(iter_sm->first);
			}
		}

		// Get the ListData_t.
		const auto *const pListData = RomFields::getFromListDataMulti(pListData_multi, def_lc, user_lc);
		assert(pListData != nullptr);
		if (pListData != nullptr) {
			// Column count.
			const auto &listDataDesc = pField->desc.list_data;
			int colCount = 1;
			if (listDataDesc.names) {
				colCount = (int)listDataDesc.names->size();
			} else {
				// No column headers.
				// Use the first row.
				assert(!pListData->empty());
				if (!pListData->empty()) {
					colCount = (int)pListData->at(0).size();
				}
			}

			// Column widths.
			// LVSCW_AUTOSIZE_USEHEADER doesn't work for entries with newlines.
			// TODO: Use ownerdraw instead? (WM_MEASUREITEM / WM_DRAWITEM)
			unique_ptr<int[]> col_width(new int[colCount]);
			for (int i = 0; i < colCount; i++) {
				col_width[i] = LVSCW_AUTOSIZE_USEHEADER;
			}

			// Dialog font and device context.
			// NOTE: Using the parent dialog's font.
			AutoGetDC hDC(hListView, hFontDlg);

			// Get the ListView data vector for LVS_OWNERDATA.
			vector<vector<tstring> > &vvStr = lvData.vvStr;

			auto iter_ld_row = pListData->cbegin();
			auto iter_vvStr_row = vvStr.begin();
			const auto pListData_cend = pListData->cend();
			const auto vvStr_end = vvStr.end();
			for (; iter_ld_row != pListData_cend && iter_vvStr_row != vvStr_end;
			     ++iter_ld_row, ++iter_vvStr_row)
			{
				const vector<string> &src_data_row = *iter_ld_row;
				vector<tstring> &dest_data_row = *iter_vvStr_row;

				int col = 0;
				auto iter_sdr = src_data_row.cbegin();
				auto iter_ddr = dest_data_row.begin();
				const auto src_data_row_cend = src_data_row.cend();
				const auto dest_data_row_end = dest_data_row.end();
				for (; iter_sdr != src_data_row_cend && iter_ddr != dest_data_row_end;
				     ++iter_sdr, ++iter_ddr, col++)
				{
					tstring tstr = U82T_s(*iter_sdr);
					int width = measureListDataString(hDC, tstr);
					if (col < colCount) {
						col_width[col] = std::max(col_width[col], width);
					}
					*iter_ddr = std::move(tstr);
				}
			}

			// Resize the columns to fit the contents.
			// NOTE: Only done on first load.
			// TODO: Need to measure text...
			if (!cboLanguage) {
				// TODO: Do this on system theme change?
				// TODO: Add a flag for 'main data column' and adjust it to
				// not exceed the viewport.
				for (int i = colCount-1; i >= 0; i--) {
					ListView_SetColumnWidth(hListView, i, col_width[i]);
				}
			}

			// Redraw all items.
			ListView_RedrawItems(hListView, 0, static_cast<int>(vvStr.size()));
		}
	}

	if (!cboLanguage && set_lc.size() > 1) {
		// Create the language combobox.

		// Get the language strings and determine the
		// maximum width.
		SIZE maxSize = {0, 0};
		vector<tstring> vec_lc_str;
		vec_lc_str.reserve(set_lc.size());
		const auto set_lc_cend = set_lc.cend();
		for (auto iter = set_lc.cbegin(); iter != set_lc_cend; ++iter) {
			const uint32_t lc = *iter;
			const char *lc_str = SystemRegion::getLocalizedLanguageName(lc);
			if (lc_str) {
				vec_lc_str.emplace_back(U82T_c(lc_str));
			} else {
				// Invalid language code.
				tstring s_lc;
				s_lc.reserve(4);
				for (uint32_t tmp_lc = lc; tmp_lc != 0; tmp_lc <<= 8) {
					TCHAR chr = (TCHAR)(tmp_lc >> 24);
					if (chr != 0) {
						s_lc += chr;
					}
				}
				vec_lc_str.emplace_back(std::move(s_lc));
			}

			const tstring &tstr = vec_lc_str.at(vec_lc_str.size()-1);
			SIZE size;
			if (!LibWin32Common::measureTextSize(hDlgSheet, hFontDlg, tstr.c_str(), &size)) {
				maxSize.cx = std::max(maxSize.cx, size.cx);
				maxSize.cy = std::max(maxSize.cy, size.cy);
			}
		}

		// TODO:
		// - Per-monitor DPI scaling (both v1 and v2)
		// - Handle WM_DPICHANGED.
		// Reference: https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
		const UINT dpi = rp_GetDpiForWindow(hDlgSheet);
		unsigned int iconSize;
		unsigned int iconMargin;
		if (dpi < 120) {
			// [96,120) dpi: Use 16x16.
			iconSize = 16;
			iconMargin = 2;
		} else if (dpi <= 144) {
			// [120,144] dpi: Use 24x24.
			// TODO: Maybe needs to be slightly higher?
			iconSize = 24;
			iconMargin = 3;
		} else {
			// >144dpi: Use 32x32.
			iconSize = 32;
			iconMargin = 4;
		}

		// Add iconSize + iconMargin for the icon.
		maxSize.cx += iconSize + iconMargin;

		// Add vertical scrollbar width and CXEDGE.
		// Reference: http://ntcoder.com/2013/10/07/mfc-resize-ccombobox-drop-down-list-based-on-contents/
		maxSize.cx += rp_GetSystemMetricsForDpi(SM_CXVSCROLL, dpi);
		maxSize.cx += (rp_GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 4);

		// Create the combobox.
		// FIXME: May need to create this after the header row
		// in order to preserve tab order. Need to check the
		// KDE and GTK+ versions, too.
		// ComboBoxEx was introduced in MSIE 3.0.
		// NOTE: Height is based on icon size.
		cboLanguage = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
			WC_COMBOBOXEX, nullptr,
			CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			rectHeader.right - maxSize.cx, rectHeader.top,
			maxSize.cx, iconSize*(8+1) + maxSize.cy - (maxSize.cy / 8),
			hDlgSheet, (HMENU)(INT_PTR)IDC_CBO_LANGUAGE, nullptr, nullptr);
		SetWindowFont(cboLanguage, hFontDlg, false);
		SendMessage(cboLanguage, CBEM_SETIMAGELIST, 0, (LPARAM)himglFlags);

		// Add the strings.
		auto iter_str = vec_lc_str.cbegin();
		auto iter_lc = set_lc.cbegin();
		int sel_idx = -1;
		COMBOBOXEXITEM cbItem;
		cbItem.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
		cbItem.iItem = 0;
		int iImage = 0;
		const auto vec_lc_str_cend = vec_lc_str.cend();
		for (; iter_str != vec_lc_str_cend; ++iter_str, ++iter_lc, cbItem.iItem++, iImage++) {
			const uint32_t lc = *iter_lc;
			cbItem.pszText = const_cast<LPTSTR>(iter_str->c_str());
			cbItem.cchTextMax = static_cast<int>(iter_str->size());
			cbItem.lParam = static_cast<LPARAM>(lc);
			cbItem.iImage = iImage;
			cbItem.iSelectedImage = iImage;

			// Insert the item.
			SendMessage(cboLanguage, CBEM_INSERTITEM, 0, (LPARAM)&cbItem);

			// Save the default index:
			// - ROM-default language code.
			// - English if it's not available.
			if (lc == def_lc) {
				// Select this item.
				sel_idx = static_cast<int>(cbItem.iItem);
			} else if (lc == 'en') {
				// English. Select this item if def_lc hasn't been found yet.
				if (sel_idx < 0) {
					sel_idx = static_cast<int>(cbItem.iItem);
				}
			}
		}

		// Build the ImageList.
		buildCboLanguageImageList();

		// Set the current index.
		ComboBox_SetCurSel(cboLanguage, sel_idx);

		// Get the dialog margin.
		// 7x7 DLU margin is recommended by the Windows UX guidelines.
		// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
		RECT dlgMargin = { 7, 7, 8, 8 };
		MapDialogRect(hDlgSheet, &dlgMargin);

		// Adjust the header row.
		const int adj = (maxSize.cx + dlgMargin.left) / 2;
		if (lblSysInfo) {
			ptSysInfo.x -= adj;
			SetWindowPos(lblSysInfo, nullptr, ptSysInfo.x, ptSysInfo.y, 0, 0,
				SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
		}
		if (lblBanner) {
			POINT pos = lblBanner->position();
			pos.x -= adj;
			lblBanner->setPosition(pos);
		}
		if (lblIcon) {
			POINT pos = lblIcon->position();
			pos.x -= adj;
			lblIcon->setPosition(pos);
		}
	}
}

/**
 * Update a field's value.
 * This is called after running a ROM operation.
 * @param fieldIdx Field index.
 * @return 0 on success; non-zero on error.
 */
int RP_ShellPropSheetExt_Private::updateField(int fieldIdx)
{
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return 1;
	}

	assert(fieldIdx >= 0);
	assert(fieldIdx < pFields->count());
	if (fieldIdx < 0 || fieldIdx >= pFields->count())
		return 2;

	const RomFields::Field *const field = pFields->at(fieldIdx);
	assert(field != nullptr);
	if (!field)
		return 3;

	// Get the tab dialog control the control is in.
	assert(field->tabIdx >= 0);
	assert(field->tabIdx < tabs.size());
	if (field->tabIdx < 0 || field->tabIdx >= tabs.size())
		return 4;
	HWND hDlg = tabs[field->tabIdx].hDlg;

	// Update the value widget(s).
	int ret;
	switch (field->type) {
		case RomFields::RFT_INVALID:
			assert(!"Cannot update an RFT_INVALID field.");
			ret = 5;
			break;
		default:
			assert(!"Unsupported field type.");
			ret = 6;
			break;

		case RomFields::RFT_STRING: {
			// HWND is a STATIC control.
			HWND hLabel = GetDlgItem(hDlg, IDC_RFT_STRING(fieldIdx));
			assert(hLabel != nullptr);
			if (!hLabel) {
				ret = 7;
				break;
			}

			if (field->data.str && !field->data.str->empty()) {
				const tstring ts_text = LibWin32Common::unix2dos(U82T_s(*field->data.str));
				SetWindowText(hLabel, ts_text.c_str());
			} else {
				SetWindowText(hLabel, _T(""));
			}
			ret = 0;
			break;
		}

		case RomFields::RFT_BITFIELD: {
			// Multiple checkboxes with unique dialog IDs.

			// Bits with a blank name aren't included, so we'll need to iterate
			// over the bitfield description.
			const auto &bitfieldDesc = field->desc.bitfield;
			int count = (int)bitfieldDesc.names->size();
			assert(count <= 32);
			if (count > 32)
				count = 32;

			// Unlike GTK+ and KDE, we don't need to check bitfieldDesc.names to determine
			// if a checkbox is present, since GetDlgItem() will return nullptr in that case.
			uint32_t bitfield = field->data.bitfield;
			int id = IDC_RFT_BITFIELD(fieldIdx, 0);
			for (; count >= 0; count--, id++, bitfield >>= 1) {
				HWND hCheckBox = GetDlgItem(hDlg, id);
				if (!hCheckBox)
					continue;

				// Set the checkbox.
				Button_SetCheck(hCheckBox, (bitfield & 1) ? BST_CHECKED : BST_UNCHECKED);
			}
			ret = 0;
			break;
		}
	}

	return ret;
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
 * Initialize the dialog. (hDlgSheet)
 * Called by WM_INITDIALOG.
 */
void RP_ShellPropSheetExt_Private::initDialog(void)
{
	assert(hDlgSheet != nullptr);
	assert(romData != nullptr);
	if (!hDlgSheet || !romData) {
		// No dialog, or no ROM data loaded.
		return;
	}

	// Set the dialog to allow automatic right-to-left adjustment
	// if the system is using an RTL language.
	if (dwExStyleRTL != 0) {
		LONG_PTR lpExStyle = GetWindowLongPtr(hDlgSheet, GWL_EXSTYLE);
		lpExStyle |= WS_EX_LAYOUTRTL;
		SetWindowLongPtr(hDlgSheet, GWL_EXSTYLE, lpExStyle);
	}

	// Get the fields.
	const RomFields *const pFields = romData->fields();
	assert(pFields != nullptr);
	if (!pFields) {
		// No fields.
		// TODO: Show an error?
		return;
	}
	const int count = pFields->count();

	// Make sure we have all required window classes available.
	// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775507(v=vs.85).aspx
	INITCOMMONCONTROLSEX initCommCtrl;
	initCommCtrl.dwSize = sizeof(initCommCtrl);
	initCommCtrl.dwICC = ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS |
	                     ICC_TAB_CLASSES | ICC_USEREX_CLASSES;
	// TODO: Also ICC_STANDARD_CLASSES on XP+?
	InitCommonControlsEx(&initCommCtrl);

	// Dialog font and device context.
	if (!hFontDlg) {
		hFontDlg = GetWindowFont(hDlgSheet);
	}
	AutoGetDC hDC(hDlgSheet, hFontDlg);

	// Initialize the fonts.
	initBoldFont(hFontDlg);
	fontHandler.setWindow(hDlgSheet);

	// Convert the bitfield description names to the
	// native Windows encoding once.
	vector<tstring> t_desc_text;
	t_desc_text.reserve(count);

	// Tab count.
	int tabCount = pFields->tabCount();
	if (tabCount < 1) {
		tabCount = 1;
	}

	// Determine the maximum length of all field names.
	// TODO: Line breaks?
	unique_ptr<int[]> a_max_text_width(new int[tabCount]());

	// tr: Field description label.
	const char *const desc_label_fmt = C_("RomDataView", "%s:");
	const auto pFields_cend = pFields->cend();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter) {
		const RomFields::Field &field = *iter;
		if (!field.isValid) {
			t_desc_text.emplace_back(tstring());
			continue;
		} else if (field.name.empty()) {
			t_desc_text.emplace_back(tstring());
			continue;
		}

		tstring desc_text = U82T_s(rp_sprintf(
			desc_label_fmt, field.name.c_str()));

		// Get the width of this specific entry.
		// TODO: Use measureTextSize()?
		SIZE textSize;
		if (field.type == RomFields::RFT_STRING &&
		    field.desc.flags & RomFields::STRF_WARNING)
		{
			// Label is bold. Use hFontBold.
			HFONT hFontOrig = SelectFont(hDC, hFontBold);
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
			SelectFont(hDC, hFontOrig);
		}
		else
		{
			// Regular font.
			GetTextExtentPoint32(hDC, desc_text.data(),
				static_cast<int>(desc_text.size()), &textSize);
		}

		assert(field.tabIdx >= 0);
		assert(field.tabIdx < tabCount);
		if (field.tabIdx >= 0 && field.tabIdx < tabCount) {
			if (textSize.cx > a_max_text_width[field.tabIdx]) {
				a_max_text_width[field.tabIdx] = textSize.cx;
			}
		}

		// Save for later.
		t_desc_text.emplace_back(std::move(desc_text));
	}

	// Add additional spacing between the ':' and the field.
	// TODO: Use measureTextSize()?
	// TODO: Reduce to 1 space?
	SIZE textSize;
	GetTextExtentPoint32(hDC, _T("  "), 2, &textSize);
	for (int i = tabCount - 1; i >= 0; i--) {
		a_max_text_width[i] += textSize.cx;
	}

	// Create the ROM field widgets.
	// Each static control is max_text_width pixels wide
	// and 8 DLUs tall, plus 4 vertical DLUs for spacing.
	RECT tmpRect = {0, 0, 0, 8+4};
	MapDialogRect(hDlgSheet, &tmpRect);
	SIZE descSize = {0, tmpRect.bottom};
	this->lblDescHeight = descSize.cy;

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hDlgSheet, &dlgMargin);

	// Get the dialog size.
	// - fullDlgRect: Full dialog size
	// - dlgRect: Adjusted dialog size.
	// FIXME: Vertical height is off by 3px on Win7...
	// Verified with WinSpy++: expected 341x408, got 341x405.
	RECT fullDlgRect, dlgRect;
	GetClientRect(hDlgSheet, &fullDlgRect);
	dlgRect = fullDlgRect;
	// Adjust the rectangle for margins.
	InflateRect(&dlgRect, -dlgMargin.left, -dlgMargin.top);
	// Calculate the size for convenience purposes.
	dlgSize.cx = dlgRect.right - dlgRect.left;
	dlgSize.cy = dlgRect.bottom - dlgRect.top;

	// Increase the total widget width by half of the margin.
	InflateRect(&dlgRect, dlgMargin.left/2, 0);
	dlgSize.cx += dlgMargin.left - 1;

	// Current position.
	POINT headerPt = {dlgRect.left, dlgRect.top};
	int dlg_value_width_base = dlgSize.cx - 1;

	// Create the header row.
	const SIZE header_size = {dlgSize.cx, descSize.cy};
	const int headerH = createHeaderRow(hDlgSheet, headerPt, header_size);
	// Save the header rect for later.
	rectHeader.left = headerPt.x;
	rectHeader.top = headerPt.y;
	rectHeader.right = headerPt.x + dlgSize.cx;
	rectHeader.bottom = headerPt.y + headerH;

	// Adjust values for the tabs.
	// TODO: Ratio instead of absolute 2px?
	dlgRect.top += (headerH + 2);
	dlgSize.cy -= (headerH + 2);
	headerPt.y += headerH;

	// Do we need to create a tab widget?
	if (tabCount > 1) {
		// TODO: Do this regardless of tabs?
		// NOTE: Margin with this change on Win7 is now 9px left, 12px bottom.
		dlgRect.bottom = fullDlgRect.bottom - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;

		// Create the tab widget.
		tabs.resize(tabCount);
		tabWidget = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
			WC_TABCONTROL, nullptr,
			WS_CHILD | WS_TABSTOP | WS_VISIBLE,
			dlgRect.left, dlgRect.top, dlgSize.cx, dlgSize.cy,
			hDlgSheet, (HMENU)(INT_PTR)IDC_TAB_WIDGET, nullptr, nullptr);
		SetWindowFont(tabWidget, hFontDlg, false);

		// Add tabs.
		// NOTE: Tabs must be added *before* calling TabCtrl_AdjustRect();
		// otherwise, the tab bar height won't be taken into account.
		TCITEM tcItem;
		tcItem.mask = TCIF_TEXT;
		for (int i = 0; i < tabCount; i++) {
			// Create a tab.
			const char *name = pFields->tabName(i);
			if (!name) {
				// Skip this tab.
				continue;
			}
			const tstring tstr = U82T_c(name);
			tcItem.pszText = const_cast<LPTSTR>(tstr.c_str());
			// FIXME: Does the index work correctly if a tab is skipped?
			TabCtrl_InsertItem(tabWidget, i, &tcItem);
		}

		// Adjust the dialog size for subtabs.
		TabCtrl_AdjustRect(tabWidget, false, &dlgRect);
		// Update dlgSize.
		dlgSize.cx = dlgRect.right - dlgRect.left;
		dlgSize.cy = dlgRect.bottom - dlgRect.top;
		iTabHeightOrig = dlgSize.cy;	// for MessageWidget
		// Update dlg_value_width.
		// FIXME: Results in 9px left, 8px right margins for RFT_LISTDATA.
		dlg_value_width_base = dlgSize.cx - dlgMargin.left - 1;

		// Create windows for each tab.
		DWORD swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW;
		for (int i = 0; i < tabCount; i++) {
			if (!pFields->tabName(i)) {
				// Skip this tab.
				continue;
			}

			auto &tab = tabs[i];

			// Create a child dialog for the tab.
			tab.hDlg = CreateDialog(HINST_THISCOMPONENT,
				MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
				hDlgSheet, SubtabDlgProc);
			SetWindowPos(tab.hDlg, nullptr,
				dlgRect.left, dlgRect.top,
				dlgSize.cx, dlgSize.cy,
				swpFlags);
			// Hide subsequent tabs.
			swpFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_HIDEWINDOW;

			// Store the D object pointer with this particular tab dialog.
			SetWindowLongPtr(tab.hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			// Store the D object pointer with this particular tab dialog.
			SetProp(tab.hDlg, TAB_PTR_PROP, static_cast<HANDLE>(&tab));

			// Current point should be equal to the margins.
			// FIXME: On both WinXP and Win7, ths ends up with an
			// 8px left margin, and 6px top/right margins.
			// (Bottom margin is 6px on WinXP, 7px on Win7.)
			tab.curPt.x = dlgMargin.left/2;
			tab.curPt.y = dlgMargin.top/2;
		}
	} else {
		// No tabs.
		// Don't create a WC_TABCONTROL, but do create a
		// child dialog in order to handle scrolling.
		tabs.resize(1);
		auto &tab = tabs[0];

		// for MessageWidget
		iTabHeightOrig = dlgSize.cy;

		// Create a child dialog.
		tab.hDlg = CreateDialog(HINST_THISCOMPONENT,
			MAKEINTRESOURCE(IDD_SUBTAB_CHILD_DIALOG),
			hDlgSheet, SubtabDlgProc);
		SetWindowPos(tab.hDlg, nullptr,
			dlgRect.left, dlgRect.top,
			dlgSize.cx, dlgSize.cy,
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);

		// Store the D object pointer with this particular tab dialog.
		SetWindowLongPtr(tab.hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		// Store the D object pointer with this particular tab dialog.
		SetProp(tab.hDlg, TAB_PTR_PROP, static_cast<HANDLE>(&tab));

		// We already have margins on the dialog position,
		// so we shouldn't have margins here.
		tab.curPt.x = 0;
		tab.curPt.y = 0;
	}

	int fieldIdx = 0;	// needed for control IDs
	auto iter_desc = t_desc_text.cbegin();
	for (auto iter = pFields->cbegin(); iter != pFields_cend; ++iter, ++iter_desc, fieldIdx++) {
		assert(iter_desc != t_desc_text.cend());
		const RomFields::Field &field = *iter;
		if (!field.isValid)
			continue;

		// Verify the tab index.
		const int tabIdx = field.tabIdx;
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

		// Description width is based on the tab index.
		descSize.cx = a_max_text_width[tabIdx];

		// Create the static text widget. (FIXME: Disable mnemonics?)
		HWND hStatic = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, iter_desc->c_str(),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_LEFT,
			tab.curPt.x, tab.curPt.y, descSize.cx, descSize.cy,
			tab.hDlg, (HMENU)(INT_PTR)IDC_STATIC_DESC(fieldIdx), nullptr, nullptr);
		SetWindowFont(hStatic, hFontDlg, false);

		// Create the value widget.
		int field_cy = descSize.cy;	// Default row size.
		const POINT pt_start = {tab.curPt.x + descSize.cx, tab.curPt.y};
		SIZE size = {dlg_value_width_base - descSize.cx, field_cy};
		switch (field.type) {
			case RomFields::RFT_INVALID:
				// No data here.
				field_cy = 0;
				break;

			case RomFields::RFT_STRING:
				// String data.
				field_cy = initString(hDlgSheet, tab.hDlg, pt_start, size, field, fieldIdx, nullptr);
				break;
			case RomFields::RFT_BITFIELD:
				// Create checkboxes starting at the current point.
				field_cy = initBitfield(hDlgSheet, tab.hDlg, pt_start, field, fieldIdx);
				break;
			case RomFields::RFT_LISTDATA: {
				// Create a ListView control.
				size.cy *= 6;	// TODO: Is this needed?
				POINT pt_ListData = pt_start;

				// Should the RFT_LISTDATA be placed on its own row?
				bool doVBox = false;
				if (field.desc.list_data.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
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
					if (tabIdx + 1 == tabCount && fieldIdx == count-1) {
						// Last tab, and last field.
						doVBox = true;
					} else {
						// Check if the next field is on the next tab.
						RomFields::const_iterator nextIter = iter;
						++nextIter;
						if (nextIter != pFields_cend && nextIter->tabIdx != tabIdx) {
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

				field_cy = initListData(hDlgSheet, tab.hDlg, pt_ListData, size, !doVBox, field, fieldIdx);
				if (field_cy > 0) {
					// Add the extra row if necessary.
					if (field.desc.list_data.flags & RomFields::RFT_LISTDATA_SEPARATE_ROW) {
						const int szAdj = descSize.cy - (dlgMargin.top/3);
						field_cy += szAdj;
						// Reduce the hStatic size slightly.
						SetWindowPos(hStatic, nullptr, 0, 0, descSize.cx, szAdj,
							SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
					}
				}
				break;
			}

			case RomFields::RFT_DATETIME:
				// Date/Time in Unix format.
				field_cy = initDateTime(hDlgSheet, tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_AGE_RATINGS:
				// Age Ratings field.
				field_cy = initAgeRatings(hDlgSheet, tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_DIMENSIONS:
				// Dimensions field.
				field_cy = initDimensions(hDlgSheet, tab.hDlg, pt_start, size, field, fieldIdx);
				break;
			case RomFields::RFT_STRING_MULTI:
				// Multi-language string field.
				field_cy = initStringMulti(hDlgSheet, tab.hDlg, pt_start, size, field, fieldIdx);
				break;

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

	// Update scrollbar settings.
	// TODO: If a VScroll bar is added, adjust widths of RFT_LISTDATA.
	// TODO: HScroll bar?
	const auto tabs_cend = tabs.cend();
	for (auto iter = tabs.cbegin(); iter != tabs_cend; ++iter) {
		// Set scroll info.
		// FIXME: Separate child dialog for no tabs.
		const auto &tab = *iter;

		// VScroll bar
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = tab.curPt.y - 2;	// max is exclusive
		si.nPage = dlgSize.cy;
		si.nPos = 0;
		SetScrollInfo(tab.hDlg, SB_VERT, &si, TRUE);

		// HScroll bar
		// NOTE: ReactOS 0.4.13 is showing an HScroll bar, even though
		// the child dialog doesn't have WS_HSCROLL set.
		si.nMin = 0;
		si.nMax = 1;
		si.nPage = 2;
		si.nPos = 0;
		SetScrollInfo(tab.hDlg, SB_HORZ, &si, TRUE);
	}

	// Initial update of RFT_MULTI_STRING fields.
	if (!vecStringMulti.empty()) {
		def_lc = pFields->defaultLanguageCode();
		updateMulti(0);
	}

	// Check for "viewed" achievements.
	romData->checkViewedAchievements();

	// Register for WTS session notifications. (Remote Desktop)
	wts.registerSessionNotification(hDlgSheet, NOTIFY_FOR_THIS_SESSION);

	// Window is fully initialized.
	isFullyInit = true;
}

/**
 * Adjust tabs for the message widget.
 * Message widget must have been created first.
 * @param bVisible True for visible; false for not.
 */
void RP_ShellPropSheetExt_Private::adjustTabsForMessageWidgetVisibility(bool bVisible)
{
	// NOTE: IsWindowVisible(hMessageWidget) doesn't seem to be
	// correct when this function is called, so we have to take
	// the visibility as a parameter instead.
	RECT rectMsgw;
	GetClientRect(hMessageWidget, &rectMsgw);
	int tab_h = iTabHeightOrig;
	if (bVisible) {
		tab_h -= rectMsgw.bottom;
	}

	std::for_each(tabs.cbegin(), tabs.cend(),
		[tab_h](const tab &tab) {
			RECT tabRect;
			GetClientRect(tab.hDlg, &tabRect);
			if (tabRect.bottom != tab_h) {
				SetWindowPos(tab.hDlg, nullptr, 0, 0, tabRect.right, tab_h,
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			}
		}
	);
}

/**
 * Show the message widget.
 * Message widget must have been created first.
 * @param messageType Message type.
 * @param lpszMsg Message.
 */
void RP_ShellPropSheetExt_Private::showMessageWidget(unsigned int messageType, const TCHAR *lpszMsg)
{
	assert(hMessageWidget != nullptr);
	if (!hMessageWidget)
		return;

	// Set the message widget stuff.
	SendMessage(hMessageWidget, WM_MSGW_SET_MESSAGE_TYPE, messageType, 0);
	SetWindowText(hMessageWidget, lpszMsg);

	adjustTabsForMessageWidgetVisibility(true);
	ShowWindow(hMessageWidget, SW_SHOW);
}

/**
 * An "Options" menu action was triggered.
 * @param menuId Menu ID. (Options ID + IDM_OPTIONS_MENU_BASE)
 */
void RP_ShellPropSheetExt_Private::menuOptions_action_triggered(int menuId)
{
	if (menuId < IDM_OPTIONS_MENU_BASE) {
		// Export to text or JSON.
		const char *const rom_filename = romData->filename();
		if (!rom_filename)
			return;

		bool toClipboard;
		tstring ts_title;
		const TCHAR *ts_default_ext = nullptr;
		const char *s_filter = nullptr;
		switch (menuId) {
			case IDM_OPTIONS_MENU_EXPORT_TEXT:
				toClipboard = false;
				ts_title = U82T_c(C_("RomDataView", "Export to Text File"));
				ts_default_ext = _T(".txt");
				// tr: Text files filter. (RP format)
				s_filter = C_("RomDataView", "Text Files|*.txt|text/plain|All Files|*.*|-");
				break;
			case IDM_OPTIONS_MENU_EXPORT_JSON:
				toClipboard = false;
				ts_title = U82T_c(C_("RomDataView", "Export to JSON File"));
				ts_default_ext = _T(".json");
				// tr: JSON files filter. (RP format)
				s_filter = C_("RomDataView", "JSON Files|*.json|application/json|All Files|*.*|-");
				break;
			case IDM_OPTIONS_MENU_COPY_TEXT:
			case IDM_OPTIONS_MENU_COPY_JSON:
				toClipboard = true;
				break;
			default:
				assert(!"Invalid action ID.");
				return;
		}

		ofstream ofs;
		tstring ts_out;

		if (!toClipboard) {
			if (ts_prevExportDir.empty()) {
				ts_prevExportDir = U82T_c(rom_filename);

				// Remove the filename portion.
				size_t bspos = ts_prevExportDir.rfind(_T('\\'));
				if (bspos != string::npos) {
					if (bspos > 2) {
						ts_prevExportDir.resize(bspos);
					} else if (bspos == 2) {
						ts_prevExportDir.resize(3);
					}
				}
			}

			tstring defaultFileName = ts_prevExportDir;
			if (!defaultFileName.empty() && defaultFileName.at(defaultFileName.size()-1) != _T('\\')) {
				defaultFileName += _T('\\');
			}

			// Get the base name of the ROM.
			tstring rom_basename;
			const char *const bspos = strrchr(rom_filename, '\\');
			if (bspos) {
				rom_basename = U82T_c(bspos+1);
			} else {
				rom_basename = U82T_c(rom_filename);
			}
			// Remove the extension, if present.
			size_t extpos = rom_basename.rfind(_T('.'));
			if (extpos != string::npos) {
				rom_basename.resize(extpos);
			}
			defaultFileName += rom_basename + ts_default_ext;

			const tstring tfilename = LibWin32Common::getSaveFileName(hDlgSheet,
				ts_title.c_str(), s_filter, defaultFileName.c_str());
			if (tfilename.empty())
				return;

			// Save the previous export directory.
			ts_prevExportDir = tfilename;
			size_t bspos2 = ts_prevExportDir.rfind(_T('\\'));
			if (bspos2 != tstring::npos && bspos2 > 3) {
				ts_prevExportDir.resize(bspos2);
			}

#ifdef __GNUC__
			// FIXME: MinGW doesn't have wchar_t overloads.
			ofs.open(T2U8(tfilename).c_str(), ofstream::out);
#else /* !__GNUC__ */
			ofs.open(tfilename.c_str(), ofstream::out);
#endif /* __GNUC__ */
			if (ofs.fail())
				return;
		}

		// TODO: Optimize this such that we can pass ofstream or ostringstream
		// to a factored-out function.

		switch (menuId) {
			case IDM_OPTIONS_MENU_EXPORT_TEXT: {
				ofs << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << '\n';
				ROMOutput ro(romData, sel_lc());
				ofs << ro;
				ofs.flush();
				break;
			}
			case IDM_OPTIONS_MENU_EXPORT_JSON: {
				JSONROMOutput jsro(romData);
				ofs << jsro << '\n';
				ofs.flush();
				break;
			}
			case IDM_OPTIONS_MENU_COPY_TEXT: {
				// NOTE: Some fields may have embedded newlines,
				// so we'll need to convert everything afterwards.
				ostringstream oss;
				oss << "== " << rp_sprintf(C_("RomDataView", "File: '%s'"), rom_filename) << '\n';
				ROMOutput ro(romData, sel_lc());
				oss << ro;
				oss.flush();
				ts_out = LibWin32Common::unix2dos(U82T_s(oss.str()));
				break;
			}
			case IDM_OPTIONS_MENU_COPY_JSON: {
				ostringstream oss;
				JSONROMOutput jsro(romData);
				jsro.setCrlf(true);
				oss << jsro << '\n';
				oss.flush();
				ts_out = U82T_s(oss.str());
				break;
			}
			default:
				assert(!"Invalid action ID.");
				return;
		}

		if (toClipboard) {
			if (OpenClipboard(hDlgSheet)) {
				EmptyClipboard();
				HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (ts_out.size() + 1) * sizeof(TCHAR));
				if (hglbCopy) {
					LPTSTR lpszCopy = static_cast<LPTSTR>(GlobalLock(hglbCopy));
					memcpy(lpszCopy, ts_out.data(), ts_out.size() * sizeof(TCHAR));
					lpszCopy[ts_out.size()] = _T('\0');
					GlobalUnlock(hglbCopy);
					SetClipboardData(CF_UNICODETEXT, hglbCopy);
				}
				CloseClipboard();
			}
		}
		return;
	}

	// Run a ROM operation.
	// TODO: Don't keep rebuilding this vector...
	vector<RomData::RomOp> ops = romData->romOps();
	const int id = menuId - IDM_OPTIONS_MENU_BASE;
	assert(id < (int)ops.size());
	if (id >= (int)ops.size()) {
		// ID is out of range.
		return;
	}

	string s_save_filename;
	RomData::RomOpParams params;
	const RomData::RomOp *op = &ops[id];
	if (op->flags & RomData::RomOp::ROF_SAVE_FILE) {
		// Add the "All Files" filter.
		string filter = op->sfi.filter;
		if (!filter.empty()) {
			// Make sure the last field isn't empty.
			if (filter.at(filter.size()-1) == '|') {
				filter += '-';
			}
			filter += '|';
		}
		// tr: "All Files" filter (RP format)
		filter += C_("RomData", "All Files|*.*|-");

		// Initial file and directory, based on the current file.
		string initialFile = FileSystem::replace_ext(romData->filename(), op->sfi.ext);

		// Prompt for a save file.
		tstring tstr = LibWin32Common::getSaveFileName(hDlgSheet,
			U82T_c(op->sfi.title), filter.c_str(), U82T_s(initialFile));
		if (tstr.empty())
			return;
		s_save_filename = T2U8(tstr);
		params.save_filename = s_save_filename.c_str();
	}

	int ret = romData->doRomOp(id, &params);
	unsigned int messageType;
	if (ret == 0) {
		// ROM operation completed.
		messageType = MB_ICONINFORMATION;

		// Update fields.
		std::for_each(params.fieldIdx.cbegin(), params.fieldIdx.cend(),
			[this](int fieldIdx) {
				this->updateField(fieldIdx);
			}
		);

		// Update the RomOp menu entry in case it changed.
		// NOTE: Assuming the RomOps vector order hasn't changed.
		ops = romData->romOps();
		assert(id < (int)ops.size());
		if (id < (int)ops.size()) {
			const RomData::RomOp &op = ops[id];

			UINT uFlags;
			if (!(op.flags & RomData::RomOp::ROF_ENABLED)) {
				uFlags = MF_BYCOMMAND | MF_STRING | MF_DISABLED;
			} else {
				uFlags = MF_BYCOMMAND | MF_STRING;
			}
			ModifyMenu(hMenuOptions, menuId, uFlags, menuId, U82T_c(op.desc));
		}
	} else {
		// An error occurred...
		// TODO: Show an error message.
		messageType = MB_ICONWARNING;
	}

	MessageBeep(messageType);
	if (!params.msg.empty()) {
		if (!hMessageWidget) {
			// FIXME: Make sure this works if multiple tabs are present.
			MessageWidgetRegister();

			// Align to the bottom of the dialog and center-align the text.
			// 7x7 DLU margin is recommended by the Windows UX guidelines.
			// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
			RECT tmpRect = {7, 7, 8, 8};
			MapDialogRect(hDlgSheet, &tmpRect);
			RECT winRect;
			GetClientRect(hDlgSheet, &winRect);
			// NOTE: We need to move left by 1px.
			OffsetRect(&winRect, -1, 0);

			// Determine the position.
			// TODO: Update on DPI change.
			const int cySmIcon = GetSystemMetrics(SM_CYSMICON);
			POINT ptMsgw; SIZE szMsgw;
			szMsgw.cy = cySmIcon + 8;
			ptMsgw.x = winRect.left;
			ptMsgw.y = winRect.bottom - szMsgw.cy;
			szMsgw.cx = winRect.right - winRect.left;
			if (tabs.size() > 1) {
				ptMsgw.x += tmpRect.left;
				ptMsgw.y -= tmpRect.top;
				szMsgw.cx -= (tmpRect.left * 2);
			} else {
				ptMsgw.x += (tmpRect.left / 2);
				ptMsgw.y -= (tmpRect.top / 2);
				szMsgw.cx -= tmpRect.left;
			}

			hMessageWidget = CreateWindowEx(
				WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT | dwExStyleRTL,
				WC_MESSAGEWIDGET, nullptr,
				WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
				ptMsgw.x, ptMsgw.y, szMsgw.cx, szMsgw.cy,
				hDlgSheet, (HMENU)IDC_MESSAGE_WIDGET,
				HINST_THISCOMPONENT, nullptr);
			SetWindowFont(hMessageWidget, hFontDlg, false);
		}

		showMessageWidget(messageType, U82T_s(params.msg));
	}
}

/**
 * Dialog subclass procedure to intercept WM_COMMAND for the "Options" button.
 * @param hWnd		Dialog handle
 * @param uMsg		Message
 * @param wParam	WPARAM
 * @param lParam	LPARAM
 * @param uIdSubclass	Subclass ID (usually the control ID)
 * @param dwRefData	RP_ShellPropSheetExt_Private*
 */
LRESULT CALLBACK RP_ShellPropSheetExt_Private::MainDialogSubclassProc(
	HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg) {
		case WM_NCDESTROY:
			// Remove the window subclass.
			// Reference: https://blogs.msdn.microsoft.com/oldnewthing/20031111-00/?p=41883
			RemoveWindowSubclass(hWnd, MainDialogSubclassProc, uIdSubclass);
			break;

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_RP_OPTIONS) {
				// Pop up the menu.
				auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(dwRefData);
				assert(d->hBtnOptions != nullptr);
				assert(d->hMenuOptions != nullptr);
				if (!d->hBtnOptions || !d->hMenuOptions)
					break;

				// Get the absolute position of the "Options" button.
				RECT rect_btnOptions;
				GetWindowRect(d->hBtnOptions, &rect_btnOptions);
				int id = TrackPopupMenu(d->hMenuOptions,
					TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_VERNEGANIMATION |
						TPM_NONOTIFY | TPM_RETURNCMD,
					rect_btnOptions.left, rect_btnOptions.top, 0,
					hWnd, nullptr);
				if (id != 0) {
					d->menuOptions_action_triggered(id);
				}
				return TRUE;
			}
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/**
 * Create the "Options" button in the parent window.
 * Called by WM_INITDIALOG.
 */
void RP_ShellPropSheetExt_Private::createOptionsButton(void)
{
	assert(hDlgSheet != nullptr);
	assert(romData != nullptr);
	if (!hDlgSheet || !romData) {
		// No dialog, or no ROM data loaded.
		return;
	}

	HWND hWndParent = GetParent(hDlgSheet);
	assert(hWndParent != nullptr);
	if (!hWndParent) {
		// No parent window...
		return;
	}

	// is the "Options" button already present?
	if (GetDlgItem(hWndParent, IDC_RP_OPTIONS) != nullptr) {
		assert(!"IDC_RP_OPTIONS is already created.");
		return;
	}

	// TODO: Verify RTL positioning.
	HWND hBtnOK = GetDlgItem(hWndParent, IDOK);
	HWND hTabControl = PropSheet_GetTabControl(hWndParent);
	if (!hBtnOK || !hTabControl) {
		return;
	}

	RECT rect_btnOK, rect_tabControl;
	GetWindowRect(hBtnOK, &rect_btnOK);
	GetWindowRect(hTabControl, &rect_tabControl);
	MapWindowPoints(HWND_DESKTOP, hWndParent, (LPPOINT)&rect_btnOK, 2);
	MapWindowPoints(HWND_DESKTOP, hWndParent, (LPPOINT)&rect_tabControl, 2);

	// Create the "Options" button.
	POINT ptBtn = {rect_tabControl.left, rect_btnOK.top};
	const SIZE szBtn = {
		rect_btnOK.right - rect_btnOK.left,
		rect_btnOK.bottom - rect_btnOK.top
	};

	const bool isComCtl32_v610 = LibWin32Common::isComCtl32_v610();

	tstring ts_caption;
	LONG lStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_PUSHBUTTON | BS_CENTER;
	if (isComCtl32_v610) {
		// COMCTL32 is v6.10 or later. Use BS_SPLITBUTTON.
		// (Windows Vista or later)
		lStyle |= BS_SPLITBUTTON;
		// tr: "Options" button.
		ts_caption = U82T_c(C_("RomDataView", "Op&tions"));
	} else {
		// COMCTL32 is older than v6.10. Use a regular button.
		// NOTE: The Unicode down arrow doesn't show on on Windows XP.
		// Maybe we *should* use ownerdraw...
		// tr: "Options" button. (WinXP version, with ellipsis.)
		ts_caption = U82T_c(C_("RomDataView", "Op&tions..."));
	}

	hBtnOptions = CreateWindowEx(dwExStyleRTL, WC_BUTTON,
		ts_caption.c_str(), lStyle,
		ptBtn.x, ptBtn.y, szBtn.cx, szBtn.cy,
		hWndParent, (HMENU)IDC_RP_OPTIONS, nullptr, nullptr);
	SetWindowFont(hBtnOptions, hFontDlg, FALSE);

	if (isComCtl32_v610) {
		BUTTON_SPLITINFO bsi;
		bsi.mask = BCSIF_STYLE;
		bsi.uSplitStyle = BCSS_NOSPLIT;
		Button_SetSplitInfo(hBtnOptions, &bsi);
	}

	// Fix up the tab order. ("Options" should be after "Apply".)
	HWND hBtnApply = GetDlgItem(hWndParent, IDC_APPLY_BUTTON);
	if (hBtnApply) {
		SetWindowPos(hBtnOptions, hBtnApply,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	// Subclass the parent dialog so we can intercept WM_COMMAND.
	SetWindowSubclass(hWndParent, MainDialogSubclassProc,
		static_cast<UINT_PTR>(IDC_RP_OPTIONS),
		reinterpret_cast<DWORD_PTR>(this));

	// Create the menu.
	hMenuOptions = CreatePopupMenu();

	/** Standard actions. **/
	static const struct {
		const char *desc;
		unsigned int id;
	} stdacts[] = {
		{NOP_C_("RomDataView|Options", "Export to Text..."),	IDM_OPTIONS_MENU_EXPORT_TEXT},
		{NOP_C_("RomDataView|Options", "Export to JSON..."),	IDM_OPTIONS_MENU_EXPORT_JSON},
		{NOP_C_("RomDataView|Options", "Copy as Text"),		IDM_OPTIONS_MENU_COPY_TEXT},
		{NOP_C_("RomDataView|Options", "Copy as JSON"),		IDM_OPTIONS_MENU_COPY_JSON},
		{nullptr, 0}
	};

	for (const auto *p = stdacts; p->desc != nullptr; p++) {
		AppendMenu(hMenuOptions, MF_STRING, p->id,
			U82T_c(dpgettext_expr(RP_I18N_DOMAIN, "RomDataView|Options", p->desc)));
	}

	/** ROM operations. **/
	const vector<RomData::RomOp> ops = romData->romOps();
	if (!ops.empty()) {
		AppendMenu(hMenuOptions, MF_SEPARATOR, 0, nullptr);

		unsigned int i = IDM_OPTIONS_MENU_BASE;
		const auto ops_end = ops.cend();
		for (auto iter = ops.cbegin(); iter != ops_end; ++iter, i++) {
			UINT uFlags;
			if (!(iter->flags & RomData::RomOp::ROF_ENABLED)) {
				uFlags = MF_STRING | MF_DISABLED;
			} else {
				uFlags = MF_STRING;
			}
			AppendMenu(hMenuOptions, uFlags, i, U82T_c(iter->desc));
		}
	}
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
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4365 4838)
#endif /* _MSC_VER */
	static const QITAB rgqit[] = {
		QITABENT(RP_ShellPropSheetExt, IShellExtInit),
		QITABENT(RP_ShellPropSheetExt, IShellPropSheetExt),
		{ 0, 0 }
	};
#ifdef _MSC_VER
# pragma warning(pop)
#endif /* _MSC_VER */
	return LibWin32Common::rp_QISearch(this, rgqit, riid, ppvObj);
}

/** IShellExtInit **/

/** IShellPropSheetExt **/
// References:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/bb775094(v=vs.85).aspx
IFACEMETHODIMP RP_ShellPropSheetExt::Initialize(
	LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	((void)pidlFolder);
	((void)hKeyProgID);

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

	tfilename = new TCHAR[cchFilename+1];
	cchFilename = DragQueryFile(hDrop, 0, tfilename, cchFilename+1);
	if (cchFilename == 0) {
		// No filename.
		goto cleanup;
	}

	// Convert the filename to UTF-8.
	u8filename = T2U8(tfilename, cchFilename);

	// Check for "bad" file systems.
	config = Config::instance();
	if (FileSystem::isOnBadFS(u8filename.c_str(),
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
		d_ptr = new RP_ShellPropSheetExt_Private(this, std::move(u8filename));
	}

	hr = S_OK;

cleanup:
	UNREF(file);
	GlobalUnlock(stm.hGlobal);
	ReleaseStgMedium(&stm);
	delete[] tfilename;

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

	auto iter_lvData = map_lvData.find(idFrom);
	if (iter_lvData == map_lvData.end()) {
		// ListView data not found...
		return ret;
	}
	const LvData_t &lvData = iter_lvData->second;

	if (plvItem->mask & LVIF_TEXT) {
		// Fill in text.
		const auto &vvStr = lvData.vvStr;

		// Is this row in range?
		if (plvItem->iItem >= 0 && plvItem->iItem < static_cast<int>(vvStr.size())) {
			// Get the row data.
			const auto &row_data = vvStr.at(plvItem->iItem);

			// Is the column in range?
			if (plvItem->iSubItem >= 0 && plvItem->iSubItem < static_cast<int>(row_data.size())) {
				// Return the string data.
				_tcscpy_s(plvItem->pszText, plvItem->cchTextMax, row_data[plvItem->iSubItem].c_str());
				ret = true;
			}
		}
	}

	if (plvItem->mask & LVIF_IMAGE) {
		// Fill in the ImageList index.
		// NOTE: Only valid for the base item.
		if (plvItem->iSubItem == 0) {
			// LVS_OWNERDATA prevents LVS_EX_CHECKBOXES from working correctly,
			// so we have to implement it here.
			// Reference: https://www.codeproject.com/Articles/29064/CGridListCtrlEx-Grid-Control-Based-on-CListCtrl
			if (lvData.hasCheckboxes) {
				// We have checkboxes.
				// Enable state handling.
				plvItem->mask |= LVIF_STATE;
				plvItem->stateMask = LVIS_STATEIMAGEMASK;
				plvItem->state = INDEXTOSTATEIMAGEMASK(
					((lvData.checkboxes & (1U << plvItem->iItem)) ? 2 : 1));
				ret = true;
			} else if (!lvData.vImageList.empty()) {
				// We have an ImageList.
				// Is this row in range?
				if (plvItem->iItem >= 0 && plvItem->iItem < static_cast<int>(lvData.vImageList.size())) {
					const int iImage = lvData.vImageList.at(plvItem->iItem);
					if (iImage >= 0) {
						// Set the ImageList index.
						plvItem->iImage = iImage;
						ret = true;
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
			if (lblIcon) {
				lblIcon->startAnimTimer();
			}
			if (hBtnOptions) {
				ShowWindow(hBtnOptions, SW_SHOW);
			}
			break;

		case PSN_KILLACTIVE:
			if (lblIcon) {
				lblIcon->stopAnimTimer();
			}
			if (hBtnOptions) {
				ShowWindow(hBtnOptions, SW_HIDE);
			}
			break;

#ifdef UNICODE
		case NM_CLICK:
		case NM_RETURN: {
			// Check if this is a SysLink control.
			// NOTE: SysLink control only supports Unicode.
			// NOTE: Linear search...
			const HWND hwndFrom = pHdr->hwndFrom;
			const bool isSysLink = std::any_of(tabs.cbegin(), tabs.cend(), [hwndFrom](const tab& tab) {
				return (tab.lblCredits == hwndFrom);
			});
			if (isSysLink) {
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
			if (!tabWidget || tabWidget != pHdr->hwndFrom)
				break;

			// Tab widget. Show the selected tab.
			int newTabIndex = TabCtrl_GetCurSel(tabWidget);
			ShowWindow(tabs[curTabIndex].hDlg, SW_HIDE);
			curTabIndex = newTabIndex;
			auto &newTab = tabs[newTabIndex];
			ShowWindow(newTab.hDlg, SW_SHOW);
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

		case MSGWN_CLOSED: {
			// MessageWidget Close button was clicked.
			adjustTabsForMessageWidgetVisibility(false);
			ret = true;
			break;
		}

		default:
			break;
	}

	return ret;
}

/**
 * WM_COMMAND handler for the property sheet.
 * @param hDlg Dialog window.
 * @param wParam
 * @param lParam
 * @return Return value.
 */
INT_PTR RP_ShellPropSheetExt_Private::DlgProc_WM_COMMAND(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	((void)hDlg);
	((void)lParam);
	INT_PTR ret = false;

	switch (HIWORD(wParam)) {
		case CBN_SELCHANGE: {
			// The user may be changing the selected language
			// for RFT_STRING_MULTI.
			if (LOWORD(wParam) != IDC_CBO_LANGUAGE)
				break;

			// NOTE: lParam also has the ComboBox HWND.
			const uint32_t lc = sel_lc();
			if (lc != 0) {
				updateMulti(lc);
			}
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
	if (!lblBanner && !lblIcon) {
		// Nothing to draw...
		return false;
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hDlg, &ps);

	// TODO: Check paint rectangles?
	// TODO: Share the memory DC between lblBanner and lblIcon?

	// Draw the banner.
	if (lblBanner) {
		lblBanner->draw(hdc);
	}

	// Draw the icon.
	if (lblIcon) {
		lblIcon->draw(hdc);
	}

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
			SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
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
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
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
				UNREF_AND_NULL_NOCHK(d->romData);
				break;
			}

			// Load the images.
			d->loadImages();
			// Initialize the dialog.
			d->initDialog();
			// We can close the RomData's underlying IRpFile now.
			d->romData->close();

			// Create the "Options" button in the parent window.
			d->createOptionsButton();

			// Start the icon animation timer.
			if (d->lblIcon) {
				d->lblIcon->startAnimTimer();
			}

			// Continue normal processing.
			break;
		}

		case WM_DESTROY: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d && d->lblIcon) {
				// Stop the animation timer.
				d->lblIcon->stopAnimTimer();
			}
			return true;
		}

		case WM_NOTIFY: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_NOTIFY(hDlg, reinterpret_cast<NMHDR*>(lParam));
		}

		case WM_COMMAND: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			return d->DlgProc_WM_COMMAND(hDlg, wParam, lParam);
		}

		case WM_PAINT: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}
			return d->DlgProc_WM_PAINT(hDlg);
		}

		case WM_SYSCOLORCHANGE:
		case WM_THEMECHANGED: {
			// Reload the images.
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return false;
			}

			// Did the background color change?
			// NOTE: Assuming the main background color changed if
			// the alternate row color changed.
			COLORREF colorAltRow = LibWin32Common::getAltRowColor();
			if (colorAltRow != d->colorAltRow) {
				// Alternate row color changed.
				d->colorAltRow = colorAltRow;

				// Reload images with the new row color.
				d->loadImages();

				// Invalidate the banner and icon rectangles.
				if (d->lblBanner) {
					d->lblBanner->invalidateRect();
				}
				if (d->lblIcon) {
					d->lblIcon->invalidateRect();
				}

				// TODO: Check for RFT_LISTDATA with icons and reinitialize
				// the icons if the background color changed.
				// Alternatively, maybe store them as ARGB32 bitmaps?
				// That method works for ComboBoxEx...
			}

			// Update the fonts.
			d->fontHandler.updateFonts(true);
			break;
		}

		case WM_NCPAINT: {
			// Update the monospaced font.
			// NOTE: This should be WM_SETTINGCHANGE with
			// SPI_GETFONTSMOOTHING or SPI_GETFONTSMOOTHINGTYPE,
			// but that message isn't sent when previewing changes
			// for ClearType. (It's sent when applying the changes.)
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (d) {
				// Update the fonts.
				d->fontHandler.updateFonts();
			}
			break;
		}

		case WM_WTSSESSION_CHANGE: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
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

		case WM_MOUSEWHEEL: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			if (!d) {
				// No RP_ShellPropSheetExt_Private. Can't do anything...
				return FALSE;
			}

			// Forward the message to the active child dialog.
			SendMessage(d->tabs[d->curTabIndex].hDlg, uMsg, wParam, lParam);
			return TRUE;
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
UINT CALLBACK RP_ShellPropSheetExt_Private::CallbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	((void)hWnd);	// TODO: Validate this?

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
			auto *const pExt = reinterpret_cast<RP_ShellPropSheetExt*>(ppsp->lParam);
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
 * Dialog procedure for subtabs.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK RP_ShellPropSheetExt_Private::SubtabDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_DESTROY: {
			// Remove the TAB_PTR_PROP property from the page.
			// The TAB_PTR_PROP property stored the pointer to the 
			// RP_ShellPropSheetExt_Private::tab object.
			RemoveProp(hDlg, RP_ShellPropSheetExtPrivate::TAB_PTR_PROP);
			break;
		}

		case WM_NOTIFY: {
			// Propagate NM_CUSTOMDRAW to the parent dialog.
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
					return TRUE;
				}

				default:
					break;
			}
			break;
		}

		case WM_CTLCOLORSTATIC: {
			const COLORREF color = static_cast<COLORREF>(GetWindowLongPtr(
				reinterpret_cast<HWND>(lParam), GWLP_USERDATA));
			if (color != 0) {
				// Set the specified color.
				HDC hdc = reinterpret_cast<HDC>(wParam);
				SetTextColor(hdc, color);
			}
			break;
		}

		case WM_VSCROLL: {
			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			auto *const tab = static_cast<RP_ShellPropSheetExt_Private::tab*>(GetProp(hDlg, TAB_PTR_PROP));
			if (!d || !tab) {
				// No RP_ShellPropSheetExt_Private or tab. Can't do anything...
				break;
			}

			// Check the operation and scroll there.
			int deltaY = 0;
			switch (LOWORD(wParam)) {
				case SB_TOP:
					deltaY = -tab->scrollPos;
					break;
				case SB_BOTTOM:
					deltaY = tab->curPt.y - tab->scrollPos;
					break;

				case SB_LINEUP:
					deltaY = -d->lblDescHeight;
					break;
				case SB_LINEDOWN:
					deltaY = d->lblDescHeight;
					break;

				case SB_PAGEUP:
					deltaY = -d->dlgSize.cy;
					break;
				case SB_PAGEDOWN:
					deltaY = d->dlgSize.cy;
					break;

				case SB_THUMBTRACK:
				case SB_THUMBPOSITION:
					deltaY = HIWORD(wParam) - tab->scrollPos;
					break;
			}

			// Make sure this doesn't go out of range.
			int scrollPos = tab->scrollPos + deltaY;
			if (scrollPos < 0) {
				deltaY -= scrollPos;
			} else if (scrollPos + d->dlgSize.cy > tab->curPt.y) {
				deltaY -= (scrollPos + 1) - (tab->curPt.y - d->dlgSize.cy);
			}

			tab->scrollPos += deltaY;
			SetScrollPos(hDlg, SB_VERT, tab->scrollPos, TRUE);
			ScrollWindow(hDlg, 0, -deltaY, nullptr, nullptr);
			return TRUE;
		}

		case WM_MOUSEWHEEL: {
			// NOTE: All code paths return TRUE to prevent a stack overflow.
			if (LOWORD(wParam) != 0) {
				// Some hotkey or mouse button is held down.
				// Don't scroll the scrollbar.
				return TRUE;
			}

			const int16_t wheel = HIWORD(wParam);
			if (wheel == 0) {
				// No movement...
				return TRUE;
			}

			auto *const d = reinterpret_cast<RP_ShellPropSheetExt_Private*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
			auto *const tab = static_cast<RP_ShellPropSheetExt_Private::tab*>(GetProp(hDlg, TAB_PTR_PROP));
			if (!d || !tab) {
				// No RP_ShellPropSheetExt_Private or tab. Can't do anything...
				return TRUE;
			}

			if (tab->curPt.y <= d->dlgSize.cy) {
				// Tab height is less than the dialog size.
				// No scrolling.
				return TRUE;
			}

			// TODO: Handle multiples of WHEEL_DELTA?
			int deltaY = d->lblDescHeight;
			if (wheel > 0) {
				deltaY = -deltaY;
			}

			// Make sure this doesn't go out of range.
			int scrollPos = tab->scrollPos + deltaY;
			if (scrollPos < 0) {
				deltaY -= scrollPos;
			} else if (scrollPos + d->dlgSize.cy > tab->curPt.y) {
				deltaY -= (scrollPos + 1) - (tab->curPt.y - d->dlgSize.cy);
			}

			tab->scrollPos += deltaY;
			SetScrollPos(hDlg, SB_VERT, tab->scrollPos, TRUE);
			ScrollWindow(hDlg, 0, -deltaY, nullptr, nullptr);
			return TRUE;
		}
	}

	// Dummy callback procedure that does nothing.
	return DefSubclassProc(hDlg, uMsg, wParam, lParam);
}
