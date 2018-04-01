/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ImageTypesTab.cpp: Image type priorities tab. (Part of ConfigDialog.)   *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "stdafx.h"
#include "ImageTypesTab.hpp"

// libwin32common
#include "libwin32common/WinUI.hpp"

// librpbase
#include "librpbase/config/Config.hpp"
using LibRpBase::Config;

// libi18n
#include "libi18n/i18n.h"

#include "resource.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
using std::wstring;

// TImageTypesConfig is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/config/TImageTypesConfig.cpp"
using LibRomData::TImageTypesConfig;

class ImageTypesTabPrivate : public TImageTypesConfig<HWND>
{
	public:
		ImageTypesTabPrivate();
		virtual ~ImageTypesTabPrivate();

	private:
		typedef TImageTypesConfig<HWND> super;
		RP_DISABLE_COPY(ImageTypesTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the ImageTypesTabPrivate object.
		static const wchar_t D_PTR_PROP[];

	protected:
		/** TImageTypesConfig functions. (protected) **/

		/**
		 * Create the labels in the grid.
		 */
		void createGridLabels(void) final;

		/**
		 * Create a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 */
		void createComboBox(unsigned int cbid) final;

		/**
		 * Add strings to a ComboBox in the grid.
		 * @param cbid ComboBox ID.
		 * @param max_prio Maximum priority value. (minimum is 1)
		 */
		void addComboBoxStrings(unsigned int cbid, int max_prio) final;

		/**
		 * Finish adding the ComboBoxes.
		 */
		void finishComboBoxes(void) final;

		/**
		 * Initialize the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveStart(void) final;

		/**
		 * Write an ImageType configuration entry.
		 * @param sysName System name.
		 * @param imageTypeList Image type list, comma-separated.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveWriteEntry(const char *sysName, const char *imageTypeList) final;

		/**
		 * Close the Save subsystem.
		 * This is needed on platforms where the configuration file
		 * must be opened with an appropriate writer class.
		 * @return 0 on success; negative POSIX error code on error.
		 */
		int saveFinish(void) final;

	protected:
		/** TImageTypesConfig functions. (public) **/

		/**
		 * Set a ComboBox's current index.
		 * This will not trigger cboImageType_priorityValueChanged().
		 * @param cbid ComboBox ID.
		 * @param prio New priority value. (0xFF == no)
		 */
		void cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio) final;

	public:
		/**
		 * Dialog procedure.
		 * @param hDlg
		 * @param uMsg
		 * @param wParam
		 * @param lParam
		 */
		static INT_PTR CALLBACK dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

		/**
		 * Property sheet callback procedure.
		 * @param hWnd
		 * @param uMsg
		 * @param ppsp
		 */
		static UINT CALLBACK callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp);

	public:
		// Property sheet.
		HPROPSHEETPAGE hPropSheetPage;
		HWND hWndPropSheet;

		// Grid parameters.
		POINT pt_cboImageType;	// Starting point for the ComboBoxes.
		SIZE sz_cboImageType;	// ComboBox size.
		unsigned int cy_cboImageType_list;	// ComboBox list height.

		// Last ComboBox added.
		// Needed in order to set the correct
		// tab order for the credits label.
		HWND cboImageType_lastAdded;

		// Temporary configuration filename.
		// Set by saveStart(); cleared by saveFinish().
		wchar_t *tmp_conf_filename;
};

// Control base ID.
#define IDC_IMAGETYPES_CBOIMAGETYPE_BASE 0x2000

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, cboImageType_lastAdded(nullptr)
	, tmp_conf_filename(nullptr)
{
	// Clear the grid parameters.
	pt_cboImageType.x = 0;
	pt_cboImageType.y = 0;
	sz_cboImageType.cx = 0;
	sz_cboImageType.cy = 0;
	cy_cboImageType_list = 0;
}

ImageTypesTabPrivate::~ImageTypesTabPrivate()
{
	// cboImageType_lastAdded should be nullptr.
	// (Cleared by finishComboBoxes().)
	assert(cboImageType_lastAdded == nullptr);

	// tmp_conf_filename should be nullptr,
	// since it's only used when saving.
	assert(tmp_conf_filename == nullptr);
	free(tmp_conf_filename);
}

// Property for "D pointer".
// This points to the ImageTypesTabPrivate object.
const wchar_t ImageTypesTabPrivate::D_PTR_PROP[] = L"ImageTypesTabPrivate";

/** TImageTypesConfig functions. (protected) **/

/**
 * Create the labels in the grid.
 */
void ImageTypesTabPrivate::createGridLabels(void)
{
	assert(hWndPropSheet != nullptr);
	assert(sz_cboImageType.cx == 0);
	if (!hWndPropSheet || sz_cboImageType.cx != 0)
		return;

	// Get the dialog margin.
	// 7x7 DLU margin is recommended by the Windows UX guidelines.
	// Reference: http://stackoverflow.com/questions/2118603/default-dialog-padding
	RECT dlgMargin = {7, 7, 8, 8};
	MapDialogRect(hWndPropSheet, &dlgMargin);

	// Get the font of the parent dialog.
	HFONT hFontDlg = GetWindowFont(GetParent(hWndPropSheet));
	assert(hFontDlg != nullptr);
	if (!hFontDlg) {
		// No font?!
		return;
	}

	// Get the dimensions of IDC_IMAGETYPES_DESC2.
	HWND lblDesc2 = GetDlgItem(hWndPropSheet, IDC_IMAGETYPES_DESC2);
	assert(lblDesc2 != nullptr);
	if (!lblDesc2) {
		// Label is missing...
		return;
	}
	RECT rect_lblDesc2;
	GetWindowRect(lblDesc2, &rect_lblDesc2);
	MapWindowPoints(HWND_DESKTOP, GetParent(lblDesc2), (LPPOINT)&rect_lblDesc2, 2);

	// Determine the size of the largest image type label.
	SIZE sz_lblImageType = {0, 0};
	for (int i = IMG_TYPE_COUNT-1; i >= 0; i--) {
		SIZE szCur;
		LibWin32Common::measureTextSize(hWndPropSheet, hFontDlg, U82W_c(imageTypeName(i)), &szCur);
		if (szCur.cx > sz_lblImageType.cx) {
			sz_lblImageType.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblImageType.cy) {
			sz_lblImageType.cy = szCur.cy;
		}
	}

	// Determine the size of the largest system name label.
	SIZE sz_lblSysName = {0, 0};
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		SIZE szCur;
		LibWin32Common::measureTextSize(hWndPropSheet, hFontDlg, U82W_c(sysName(sys)), &szCur);
		if (szCur.cx > sz_lblSysName.cx) {
			sz_lblSysName.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblSysName.cy) {
			sz_lblSysName.cy = szCur.cy;
		}
	}

	// Create a combo box in order to determine its actual vertical size.
	sz_cboImageType.cx = sz_lblImageType.cx;
	const unsigned int cbo_test_cy = sz_lblImageType.cy * 3;
	HWND cboTestBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
		WC_COMBOBOX, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		0, 0, sz_cboImageType.cx, cbo_test_cy,
		hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
	SetWindowFont(cboTestBox, hFontDlg, FALSE);

	RECT rect_cboTestBox;
	GetWindowRect(cboTestBox, &rect_cboTestBox);
	MapWindowPoints(HWND_DESKTOP, GetParent(cboTestBox), (LPPOINT)&rect_cboTestBox, 2);
	sz_cboImageType.cy = rect_cboTestBox.bottom;
	cy_cboImageType_list = rect_cboTestBox.bottom * 3;
	DestroyWindow(cboTestBox);

	// Create the image type labels.
	POINT curPt = {rect_lblDesc2.left + sz_lblSysName.cx + (dlgMargin.right/2),
		rect_lblDesc2.bottom + dlgMargin.bottom};
	for (unsigned int i = 0; i < IMG_TYPE_COUNT; i++) {
		HWND lblImageType = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, U82W_c(imageTypeName(i)),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_CENTER,
			curPt.x, curPt.y, sz_lblImageType.cx, sz_lblImageType.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblImageType, hFontDlg, FALSE);
		curPt.x += sz_lblImageType.cx;
	}

	// Determine the starting point.
	curPt.x = rect_lblDesc2.left;
	curPt.y += sz_lblImageType.cy + (dlgMargin.bottom / 2);
	int yadj_lblSysName = (rect_cboTestBox.bottom - sz_lblSysName.cy) / 2;
	if (yadj_lblSysName < 0) {
		yadj_lblSysName = 0;
	}

	// Save the ComboBox starting position for later.
	pt_cboImageType.x = curPt.x + sz_lblSysName.cx + (dlgMargin.right/2);
	pt_cboImageType.y = curPt.y;

	// Create the system name labels.
	curPt.y += yadj_lblSysName;
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// System name label.
		HWND lblSysName = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, U82W_c(sysName(sys)),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_RIGHT,
			curPt.x, curPt.y,
			sz_lblSysName.cx, sz_lblSysName.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysName, hFontDlg, FALSE);

		// Next row.
		curPt.y += rect_cboTestBox.bottom;
	}
}

/**
 * Create a ComboBox in the grid.
 * @param cbid ComboBox ID.
 */
void ImageTypesTabPrivate::createComboBox(unsigned int cbid)
{
	assert(hWndPropSheet != nullptr);
	assert(sz_cboImageType.cx != 0);
	if (!hWndPropSheet || sz_cboImageType.cx == 0)
		return;

	const unsigned int sys = sysFromCbid(cbid);
	const unsigned int imageType = imageTypeFromCbid(cbid);
	if (!validateSysImageType(sys, imageType))
		return;

	// Get the font of the parent dialog.
	// TODO: Cache this?
	HFONT hFontDlg = GetWindowFont(GetParent(hWndPropSheet));
	assert(hFontDlg != nullptr);
	if (!hFontDlg) {
		// No font?!
		return;
	}

	// Create the ComboBox.
	const POINT ptComboBox = {
		pt_cboImageType.x + (sz_cboImageType.cx * (LONG)imageType),
		pt_cboImageType.y + (sz_cboImageType.cy * (LONG)sys)
	};

	HWND hComboBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
		WC_COMBOBOX, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		ptComboBox.x, ptComboBox.y,
		sz_cboImageType.cx, cy_cboImageType_list,
		hWndPropSheet, (HMENU)(INT_PTR)(IDC_IMAGETYPES_CBOIMAGETYPE_BASE + cbid),
		nullptr, nullptr);
	SetWindowFont(hComboBox, hFontDlg, FALSE);
	cboImageType[sys][imageType] = hComboBox;

	SetWindowPos(hComboBox,
		cboImageType_lastAdded ? cboImageType_lastAdded : hWndPropSheet,
		0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	cboImageType_lastAdded = hComboBox;
}

/**
 * Add strings to a ComboBox in the grid.
 * @param cbid ComboBox ID.
 * @param max_prio Maximum priority value. (minimum is 1)
 */
void ImageTypesTabPrivate::addComboBoxStrings(unsigned int cbid, int max_prio)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;
	HWND cboImageType = GetDlgItem(hWndPropSheet, IDC_IMAGETYPES_CBOIMAGETYPE_BASE + cbid);
	assert(cboImageType != nullptr);
	if (!cboImageType)
		return;

	// Dropdown strings.
	// NOTE: One more string than the total number of image types,
	// since we have a string for "No".
	static const char s_values[][4] = {
		NOP_C_("ImageTypesTab|Values", "No"),
		NOP_C_("ImageTypesTab|Values", "1"),
		NOP_C_("ImageTypesTab|Values", "2"),
		NOP_C_("ImageTypesTab|Values", "3"),
		NOP_C_("ImageTypesTab|Values", "4"),
		NOP_C_("ImageTypesTab|Values", "5"),
		NOP_C_("ImageTypesTab|Values", "6"),
		NOP_C_("ImageTypesTab|Values", "7"),
		NOP_C_("ImageTypesTab|Values", "8"),
		NOP_C_("ImageTypesTab|Values", "9"),
	};
	static_assert(ARRAY_SIZE(s_values) == IMG_TYPE_COUNT+1, "s_values[] is the wrong size.");

	// NOTE: Need to add one more than the total number,
	// since "No" counts as an entry.
	for (int i = 0; i <= max_prio; i++) {
		ComboBox_AddString(cboImageType, U82W_c(
			dpgettext_expr(RP_I18N_DOMAIN, "ImageTypesTab|Values", s_values[i])));
	}
	ComboBox_SetCurSel(cboImageType, 0);
}

/**
 * Finish adding the ComboBoxes.
 */
void ImageTypesTabPrivate::finishComboBoxes(void)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	if (!cboImageType_lastAdded) {
		// Nothing to do here.
		return;
	}

	HWND lblCredits = GetDlgItem(hWndPropSheet, IDC_IMAGETYPES_CREDITS);
	assert(lblCredits != nullptr);
	if (!lblCredits)
		return;

	SetWindowPos(lblCredits,
		cboImageType_lastAdded ? cboImageType_lastAdded : hWndPropSheet,
		0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	cboImageType_lastAdded = nullptr;
}

/**
 * Initialize the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveStart(void)
{
	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();
	const char *const filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return -ENOENT;
	}

	// Store the configuration filename.
	assert(tmp_conf_filename == nullptr);
	if (tmp_conf_filename) {
		// Shouldn't be set here...
		free(tmp_conf_filename);
	}
	tmp_conf_filename = wcsdup(U82W_s(filename));
	return 0;
}

/**
 * Write an ImageType configuration entry.
 * @param sysName System name.
 * @param imageTypeList Image type list, comma-separated.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveWriteEntry(const char *sysName, const char *imageTypeList)
{
	assert(tmp_conf_filename != nullptr);
	if (!tmp_conf_filename) {
		return -ENOENT;
	}
	WritePrivateProfileString(L"ImageTypes", U82W_c(sysName), U82W_c(imageTypeList), tmp_conf_filename);
	return 0;
}

/**
 * Close the Save subsystem.
 * This is needed on platforms where the configuration file
 * must be opened with an appropriate writer class.
 * @return 0 on success; negative POSIX error code on error.
 */
int ImageTypesTabPrivate::saveFinish(void)
{
	// Clear the configuration filename.
	assert(tmp_conf_filename != nullptr);
	if (!tmp_conf_filename) {
		return -ENOENT;
	}
	free(tmp_conf_filename);
	tmp_conf_filename = nullptr;
	return 0;
}

/** TImageTypesConfig functions. (public) **/

/**
 * Set a ComboBox's current index.
 * This will not trigger cboImageType_priorityValueChanged().
 * @param cbid ComboBox ID.
 * @param prio New priority value. (0xFF == no)
 */
void ImageTypesTabPrivate::cboImageType_setPriorityValue(unsigned int cbid, unsigned int prio)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;
	HWND cboImageType = GetDlgItem(hWndPropSheet, IDC_IMAGETYPES_CBOIMAGETYPE_BASE + cbid);
	assert(cboImageType != nullptr);
	if (cboImageType) {
		ComboBox_SetCurSel(cboImageType, (prio < IMG_TYPE_COUNT ? prio+1 : 0));
	}
}

/** Other ImageTypesTabPrivate functions. **/

/**
 * Dialog procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
INT_PTR CALLBACK ImageTypesTabPrivate::dlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			// Get the pointer to the property sheet page object. This is
			// contained in the LPARAM of the PROPSHEETPAGE structure.
			LPPROPSHEETPAGE pPage = reinterpret_cast<LPPROPSHEETPAGE>(lParam);
			if (!pPage)
				return TRUE;

			// Get the pointer to the ImageTypesTabPrivate object.
			ImageTypesTabPrivate *const d = reinterpret_cast<ImageTypesTabPrivate*>(pPage->lParam);
			if (!d)
				return TRUE;

			assert(d->hWndPropSheet == nullptr);
			d->hWndPropSheet = hDlg;

			// Store the D object pointer with this particular page dialog.
			SetProp(hDlg, D_PTR_PROP, reinterpret_cast<HANDLE>(d));

			// Create the control grid.
			d->createGrid();
			return TRUE;
		}

		case WM_DESTROY: {
			// Remove the D_PTR_PROP property from the page.
			// The D_PTR_PROP property stored the pointer to the
			// ImageTypesTabPrivate object.
			RemoveProp(hDlg, D_PTR_PROP);
			return TRUE;
		}

		case WM_NOTIFY: {
			ImageTypesTabPrivate *const d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			NMHDR *pHdr = reinterpret_cast<NMHDR*>(lParam);
			switch (pHdr->code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed) {
						// TODO: Show an error message if this fails.
						d->save();
					}
					break;

				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					if (pHdr->idFrom == IDC_IMAGETYPES_CREDITS) {
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;

				case PSN_SETACTIVE:
					// Enable the "Defaults" button.
					RpPropSheet_EnableDefaults(GetParent(hDlg), true);
					break;

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			ImageTypesTabPrivate *const d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			if (HIWORD(wParam) != CBN_SELCHANGE)
				break;

			// NOTE: CBN_SELCHANGE is NOT sent in response to
			// CB_SETCURSEL, so we shouldn't need to "lock"
			// this handler when reset() is called.
			// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775821(v=vs.85).aspx
			unsigned int cbid = LOWORD(wParam);
			if (cbid < IDC_IMAGETYPES_CBOIMAGETYPE_BASE)
				break;
			cbid -= IDC_IMAGETYPES_CBOIMAGETYPE_BASE;

			const int idx = ComboBox_GetCurSel((HWND)lParam);
			const unsigned int prio = (unsigned int)(idx <= 0 ? 0xFF : idx-1);
			if (d->cboImageType_priorityValueChanged(cbid, prio)) {
				// Configuration has been changed.
				PropSheet_Changed(GetParent(d->hWndPropSheet), d->hWndPropSheet);
			}

			// Allow the message to be processed by the system.
			break;
		}

		case WM_RP_PROP_SHEET_RESET: {
			ImageTypesTabPrivate *const d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			// Reset the tab.
			d->reset();
			break;
		}

		case WM_RP_PROP_SHEET_DEFAULTS: {
			ImageTypesTabPrivate *const d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			// Load the defaults.
			bool bRet = d->loadDefaults();
			if (bRet) {
				// Configuration has been changed.
				PropSheet_Changed(GetParent(d->hWndPropSheet), d->hWndPropSheet);
			}
			break;
		}

		default:
			break;
	}

	return FALSE; // Let system deal with other messages
}

/**
 * Property sheet callback procedure.
 * @param hDlg
 * @param uMsg
 * @param wParam
 * @param lParam
 */
UINT CALLBACK ImageTypesTabPrivate::callbackProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
	switch (uMsg) {
		case PSPCB_CREATE: {
			// Must return TRUE to enable the page to be created.
			return TRUE;
		}

		case PSPCB_RELEASE: {
			// TODO: Do something here?
			break;
		}

		default:
			break;
	}

	return FALSE;
}

/** ImageTypesTab **/

ImageTypesTab::ImageTypesTab(void)
	: d_ptr(new ImageTypesTabPrivate())
{ }

ImageTypesTab::~ImageTypesTab()
{
	delete d_ptr;
}

/**
 * Create the HPROPSHEETPAGE for this tab.
 *
 * NOTE: This function can only be called once.
 * Subsequent invocations will return nullptr.
 *
 * @return HPROPSHEETPAGE.
 */
HPROPSHEETPAGE ImageTypesTab::getHPropSheetPage(void)
{
	RP_D(ImageTypesTab);
	assert(d->hPropSheetPage == nullptr);
	if (d->hPropSheetPage) {
		// Property sheet has already been created.
		return nullptr;
	}

	// tr: Tab title.
	const wstring wsTabTitle = U82W_c(C_("ImageTypesTab", "Image Types"));

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = HINST_THISCOMPONENT;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_IMAGETYPES);
	psp.pszIcon = nullptr;
	psp.pszTitle = wsTabTitle.c_str();
	psp.pfnDlgProc = ImageTypesTabPrivate::dlgProc;
	psp.lParam = reinterpret_cast<LPARAM>(d);
	psp.pcRefParent = nullptr;
	psp.pfnCallback = ImageTypesTabPrivate::callbackProc;

	d->hPropSheetPage = CreatePropertySheetPage(&psp);
	return d->hPropSheetPage;
}

/**
 * Reset the contents of this tab.
 */
void ImageTypesTab::reset(void)
{
	RP_D(ImageTypesTab);
	d->reset();
}

/**
 * Load the default configuration.
 * This does NOT save, and will only emit modified()
 * if it's different from the current configuration.
 */
void ImageTypesTab::loadDefaults(void)
{
	RP_D(ImageTypesTab);
	bool bRet = d->loadDefaults();
	if (bRet) {
		// Configuration has been changed.
		PropSheet_Changed(GetParent(d->hWndPropSheet), d->hWndPropSheet);
	}
}

/**
 * Save the contents of this tab.
 */
void ImageTypesTab::save(void)
{
	// TODO: Show an error message if this fails.
	RP_D(ImageTypesTab);
	if (d->changed) {
		d->save();
	}
}
