/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ShellPropSheetExt_p.hpp: IShellPropSheetExt implementation.          *
 * (Private class)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_P_HPP__

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

// librpbase
namespace LibRpBase {
	class RomData;
	class RomFields;
}

// TCHAR
#include "tcharx.h"

// C++ includes
#include <string>
#include <unordered_map>
#include <vector>

// Custom controls (pseudo-controls)
class DragImageLabel;
#include "FontHandler.hpp"


// ListView Data
#include "LvData.hpp"

/** RP_ShellPropSheetExt_Private **/
// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ShellPropSheetExtPrivate RP_ShellPropSheetExt_Private

class RP_ShellPropSheetExt_Private
{
	public:
		explicit RP_ShellPropSheetExt_Private(RP_ShellPropSheetExt *q, std::u8string &&filename);
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
		// ROM filename
		std::u8string filename;
		// ROM data (Not opened until the properties tab is shown.)
		LibRpBase::RomData *romData;

		// Useful window handles
		HWND hDlgSheet;		// Property sheet
		HWND hBtnOptions;	// Options button
		std::tstring ts_prevExportDir;

		// Font handler
		FontHandler fontHandler;

		// Header row widgets.
		HWND lblSysInfo;
		POINT ptSysInfo;
		RECT rectHeader;

		// wtsapi32.dll for Remote Desktop status. (WinXP and later)
		LibWin32UI::WTSSessionNotification wts;
		// ListView controls (for toggling LVS_EX_DOUBLEBUFFER)
		std::vector<HWND> hwndListViewControls;

		// ListView data.
		// - Key: ListView dialog ID
		// - Value: LvData.
		std::unordered_map<uint16_t, LvData> map_lvData;

		/**
		 * ListView GetDispInfo function.
		 * @param plvdi	[in/out] NMLVDISPINFO
		 * @return TRUE if handled; FALSE if not.
		 */
		inline BOOL ListView_GetDispInfo(NMLVDISPINFO *plvdi);

		/**
		 * ListView ColumnClick function.
		 * @param plv	[in] NMLISTVIEW (only iSubItem is significant)
		 * @return TRUE if handled; FALSE if not.
		 */
		inline BOOL ListView_ColumnClick(const NMLISTVIEW *plv);

		/**
		 * Header DividerDblClick function.
		 * @param phd	[in] NMHEADER
		 * @return TRUE if handled; FALSE if not.
		 */
		inline BOOL Header_DividerDblClick(const NMHEADER *phd);

		/**
		 * ListView CustomDraw function.
		 * @param plvcd	[in/out] NMLVCUSTOMDRAW
		 * @return Return value.
		 */
		inline int ListView_CustomDraw(NMLVCUSTOMDRAW *plvcd) const;

		// Banner and icon.
		DragImageLabel *lblBanner;
		DragImageLabel *lblIcon;

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
		std::vector<tab> tabs;
		int curTabIndex;

		// Sizes.
		int lblDescHeight;	// Description label height.
		SIZE dlgSize;		// Visible dialog size.

		// MessageWidget for ROM operation notifications.
		HWND hMessageWidget;
		int iTabHeightOrig;

		// Multi-language functionality.
		uint32_t def_lc;	// Default language code from RomFields.
		HWND cboLanguage;

		// RFT_STRING_MULTI value labels.
		typedef std::pair<HWND, const LibRpBase::RomFields::Field*> Data_StringMulti_t;
		std::vector<Data_StringMulti_t> vecStringMulti;

		// Is the UI locale right-to-left?
		// If so, this will be set to WS_EX_LAYOUTRTL.
		DWORD dwExStyleRTL;

		// Alternate row color.
		COLORREF colorAltRow;
		bool isFullyInit;		// True if the window is fully initialized.

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
		 * @param size		[in] Width and height for a single line label.
		 * @param field		[in] RomFields::Field
		 * @param fieldIdx	[in] Field index
		 * @param str		[in,opt] String data. (If nullptr, field data is used.)
		 * @param pOutHWND	[out,opt] Retrieves the control's HWND.
		 * @return Field height, in pixels.
		 */
		int initString(_In_ HWND hDlg, _In_ HWND hWndTab,
			_In_ const POINT &pt_start, _In_ const SIZE &size,
			_In_ const LibRpBase::RomFields::Field &field, _In_ int fieldIdx,
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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
			const LibRpBase::RomFields::Field &field, int fieldIdx);

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
		 * An "Options" menu button action was triggered.
		 * @param menuId Menu ID. (Options ID + IDM_OPTIONS_MENU_BASE)
		 */
		void btnOptions_action_triggered(int menuId);

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

#endif /* __ROMPROPERTIES_WIN32_RP_SHELLPROPSHEETEXT_P_HPP__ */

