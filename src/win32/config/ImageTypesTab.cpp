/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * ImageTypesTab.cpp: Image type priorities tab. (Part of ConfigDialog.)   *
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
#include "ImageTypesTab.hpp"

#include "libromdata/RpWin32.hpp"
#include "libromdata/config/Config.hpp"
#include "libromdata/RomData.hpp"
using LibRomData::Config;
using LibRomData::RomData;

// RomData subclasses with images.
#include "libromdata/Amiibo.hpp"
#include "libromdata/DreamcastSave.hpp"
#include "libromdata/GameCube.hpp"
#include "libromdata/GameCubeSave.hpp"
#include "libromdata/NintendoDS.hpp"
#include "libromdata/Nintendo3DS.hpp"
#include "libromdata/PlayStationSave.hpp"
#include "libromdata/WiiU.hpp"

#include "WinUI.hpp"
#include "resource.h"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <string>
#include <sstream>
using std::wstring;
using std::wostringstream;

class ImageTypesTabPrivate
{
	public:
		ImageTypesTabPrivate();

	private:
		RP_DISABLE_COPY(ImageTypesTabPrivate)

	public:
		// Property for "D pointer".
		// This points to the ImageTypesTabPrivate object.
		static const wchar_t D_PTR_PROP[];

	public:
		// Image type data.
		static const rp_char *const imageTypeNames[RomData::IMG_EXT_MAX+1];

		// System data.
		typedef uint32_t (*pFnSupportedImageTypes)(void);
		struct SysData {
			const rp_char *name;			// System name.
			const char *classNameA;			// Class name in Config. (ASCII)
			const wchar_t *classNameW;		// Class name in Config. (Unicode)
			pFnSupportedImageTypes getTypes;	// Get supported image types.
		};
#define SysDataEntry(klass, name) \
	{name, #klass, L#klass, LibRomData::klass::supportedImageTypes_static}
		static const int SYS_COUNT = 8;
		static const SysData sysData[SYS_COUNT];

		/**
		 * Create the grid of static text and combo boxes.
		 */
		void createGrid(void);

		/**
		 * Reset the grid to the current configuration.
		 */
		void reset(void);

		/**
		 * Save the configuration.
		 */
		void save(void);

		/**
		 * Update imageTypes in response to a user change.
		 * @param id Control ID. (IDC_IMAGETYPES_CBOIMAGETYPE)
		 * @param prio New priority. (0xFF == No; others == priority)
		 */
		void updateImageType(unsigned int id, unsigned int prio);

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

		// Has the user changed anything?
		bool changed;

		// Image type dropdowns.
		// NOTE: This is a square array, but no system supports
		// *all* image types, so most of these will be nullptr.
		HWND cboImageType[SYS_COUNT][RomData::IMG_EXT_MAX+1];

		// Image types. (0xFF == No; others == priority)
		// NOTE: We need to store them here in order to handle
		// duplicate prevention, since CBN_SELCHANGE doesn't
		// include the "previous" index.
		uint8_t imageTypes[SYS_COUNT][RomData::IMG_EXT_MAX+1];

		// Number of valid image types per system.
		uint8_t validImageTypes[SYS_COUNT];

		// Which systems have the default configuration?
		// These ones will be saved with a blank value.
		bool sysIsDefault[SYS_COUNT];
};

// Control base IDs.
#define IDC_IMAGETYPES_CBOIMAGETYPE(sysName, imageType) (0x2000 + (((sysName) << 4) | (imageType)))

/** ImageTypesTabPrivate **/

ImageTypesTabPrivate::ImageTypesTabPrivate()
	: hPropSheetPage(nullptr)
	, hWndPropSheet(nullptr)
	, changed(false)
{
	// Clear the arrays.
	memset(cboImageType, 0, sizeof(cboImageType));
	memset(imageTypes, 0xFF, sizeof(imageTypes));
	memset(validImageTypes, 0, sizeof(validImageTypes));
	memset(sysIsDefault, 0, sizeof(sysIsDefault));
}

// Property for "D pointer".
// This points to the ImageTypesTabPrivate object.
const wchar_t ImageTypesTabPrivate::D_PTR_PROP[] = L"ImageTypesTabPrivate";

// Image type names.
const rp_char *const ImageTypesTabPrivate::imageTypeNames[RomData::IMG_EXT_MAX+1] = {
	_RP("Internal\nIcon"),
	_RP("Internal\nBanner"),
	_RP("Internal\nMedia"),
	_RP("External\nMedia"),
	_RP("External\nCover"),
	_RP("External\n3D Cover"),
	_RP("External\nFull Cover"),
	_RP("External\nBox"),
};

// System data.
const ImageTypesTabPrivate::SysData ImageTypesTabPrivate::sysData[SYS_COUNT] = {
	SysDataEntry(Amiibo,		_RP("amiibo")),
	SysDataEntry(DreamcastSave,	_RP("Dreamcast Saves")),
	SysDataEntry(GameCube,		_RP("GameCube / Wii")),
	SysDataEntry(GameCubeSave,	_RP("GameCube Saves")),
	SysDataEntry(NintendoDS,	_RP("Nintendo DS(i)")),
	SysDataEntry(Nintendo3DS,	_RP("Nintendo 3DS")),
	SysDataEntry(PlayStationSave,	_RP("PlayStation Saves")),
	SysDataEntry(WiiU,		_RP("Wii U")),
};

/**
 * Create the grid of static text and combo boxes.
 */
void ImageTypesTabPrivate::createGrid()
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
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
	for (int i = RomData::IMG_EXT_MAX; i >= 0; i--) {
		SIZE szCur;
		WinUI::measureTextSize(hWndPropSheet, hFontDlg, RP2W_c(imageTypeNames[i]), &szCur);
		if (szCur.cx > sz_lblImageType.cx) {
			sz_lblImageType.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblImageType.cy) {
			sz_lblImageType.cy = szCur.cy;
		}
	}

	// Determine the size of the largest system name label.
	// TODO: Height needs to match the combo boxes.
	SIZE sz_lblSysName = {0, 0};
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		SIZE szCur;
		WinUI::measureTextSize(hWndPropSheet, hFontDlg, RP2W_c(sysData[sys].name), &szCur);
		if (szCur.cx > sz_lblSysName.cx) {
			sz_lblSysName.cx = szCur.cx;
		}
		if (szCur.cy > sz_lblSysName.cy) {
			sz_lblSysName.cy = szCur.cy;
		}
	}

	// Create a combo box in order to determine its actual vertical size.
	SIZE szCbo = {sz_lblImageType.cx, sz_lblImageType.cy*3};
	HWND cboTestBox = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
		WC_COMBOBOX, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
		0, 0, szCbo.cx, szCbo.cy,
		hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
	SetWindowFont(cboTestBox, hFontDlg, FALSE);

	RECT rect_cboTestBox;
	GetWindowRect(cboTestBox, &rect_cboTestBox);
	MapWindowPoints(HWND_DESKTOP, GetParent(cboTestBox), (LPPOINT)&rect_cboTestBox, 2);
	szCbo.cy = rect_cboTestBox.bottom * 3;
	DestroyWindow(cboTestBox);

	// Create the image type labels.
	POINT curPt = {rect_lblDesc2.left + sz_lblSysName.cx + (dlgMargin.right/2),
		rect_lblDesc2.bottom + dlgMargin.bottom};
	for (unsigned int i = 0; i <= RomData::IMG_EXT_MAX; i++) {
		HWND lblImageType = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(imageTypeNames[i]),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_CENTER,
			curPt.x, curPt.y, sz_lblImageType.cx, sz_lblImageType.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblImageType, hFontDlg, FALSE);
		curPt.x += sz_lblImageType.cx;
	}

	// Dropdown strings.
	// NOTE: One more string than the total number of image types,
	// since we have a string for "No".
	static const wchar_t s_values[RomData::IMG_EXT_MAX+2][4] = {
		L"No", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8"
	};
	static_assert(ARRAY_SIZE(s_values) == RomData::IMG_EXT_MAX+2, "s_values[] is the wrong size.");

	// Create the system name labels and dropdown boxes.
	curPt.x = rect_lblDesc2.left;
	curPt.y += sz_lblImageType.cy + (dlgMargin.bottom / 2);
	int yadj_lblSysName = (rect_cboTestBox.bottom - sz_lblSysName.cy) / 2;
	if (yadj_lblSysName < 0) {
		yadj_lblSysName = 0;
	}
	const int cbo_x_start = curPt.x + sz_lblSysName.cx + (dlgMargin.right/2);
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// System name label.
		HWND lblSysName = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
			WC_STATIC, RP2W_c(sysData[sys].name),
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_RIGHT,
			curPt.x, curPt.y+yadj_lblSysName,
			sz_lblSysName.cx, sz_lblSysName.cy,
			hWndPropSheet, (HMENU)IDC_STATIC, nullptr, nullptr);
		SetWindowFont(lblSysName, hFontDlg, FALSE);

		// Get supported image types.
		uint32_t imgbf = sysData[sys].getTypes();
		assert(imgbf != 0);

		int cbo_x = cbo_x_start;
		validImageTypes[sys] = 0;
		HWND *p_cboImageType = &cboImageType[sys][0];
		for (unsigned int imageType = 0; imgbf != 0 && imageType <= RomData::IMG_EXT_MAX+1;
		     imageType++, p_cboImageType++, cbo_x += sz_lblImageType.cx, imgbf >>= 1)
		{
			if (!(imgbf & 1)) {
				// Current image type is not supported.
				*p_cboImageType = nullptr;
				continue;
			}

			// Create the dropdown box.
			*p_cboImageType = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
				WC_COMBOBOX, nullptr,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
				cbo_x, curPt.y, szCbo.cx, szCbo.cy,
				hWndPropSheet, (HMENU)(INT_PTR)IDC_IMAGETYPES_CBOIMAGETYPE(sys, imageType),
				nullptr, nullptr);
			SetWindowFont(*p_cboImageType, hFontDlg, FALSE);

			// Increment the valid image types counter.
			validImageTypes[sys]++;
		}

		// Add strings to the dropdowns.
		p_cboImageType = &cboImageType[sys][0];
		for (int imageType = RomData::IMG_EXT_MAX; imageType >= 0; imageType--, p_cboImageType++) {
			if (!*p_cboImageType) {
				// No dropdown box here.
				continue;
			}

			// NOTE: Need to add one more than the total number,
			// since "No" counts as an entry.
			for (int i = 0; i <= validImageTypes[sys]; i++) {
				assert(s_values[i] != nullptr);
				ComboBox_AddString(*p_cboImageType, s_values[i]);
			}
		}

		// Next row.
		curPt.y += rect_cboTestBox.bottom;
	}

	// Load the configuration.
	reset();
}

/**
 * Reset the grid to the current configuration.
 */
void ImageTypesTabPrivate::reset(void)
{
	// Reset all combo boxes first.
	HWND *p_cboImageType;
	for (p_cboImageType = &cboImageType[0][0];
	     p_cboImageType != &cboImageType[SYS_COUNT][0]; p_cboImageType++)
	{
		if (*p_cboImageType) {
			ComboBox_SetCurSel(*p_cboImageType, 0);
		}
	}

	const Config *const config = Config::instance();
	for (int sys = SYS_COUNT-1; sys >= 0; sys--) {
		p_cboImageType = &cboImageType[sys][0];

		// Get the image priority.
		Config::ImgTypePrio_t imgTypePrio;
		Config::ImgTypeResult res = config->getImgTypePrio(sysData[sys].classNameA, &imgTypePrio);
		bool no_thumbs = false;
		switch (res) {
			case Config::IMGTR_SUCCESS:
				// Image type priority received successfully.
				sysIsDefault[sys] = false;
				break;
			case Config::IMGTR_SUCCESS_DEFAULTS:
				// Image type priority received successfully.
				// IMGTR_SUCCESS_DEFAULTS indicates the returned
				// data is the default priority, since a custom
				// configuration was not found for this class.
				sysIsDefault[sys] = true;
				break;
			case Config::IMGTR_DISABLED:
				// Thumbnails are disabled for this class.
				no_thumbs = true;
				break;
			default:
				// Should not happen...
				assert(!"Invalid return value from Config::getImgTypePrio().");
				no_thumbs = true;
				break;
		}

		if (no_thumbs)
			continue;

		int nextPrio = 0;	// Next priority value to use.
		bool imageTypeSet[RomData::IMG_EXT_MAX+1];	// Element set to true once an image type priority is read.
		memset(imageTypeSet, 0, sizeof(imageTypeSet));

		p_cboImageType = &cboImageType[sys][0];
		for (unsigned int i = 0; i < imgTypePrio.length && nextPrio <= validImageTypes[sys]; i++)
		{
			uint8_t imageType = imgTypePrio.imgTypes[i];
			assert(imageType == 0xFF || imageType <= RomData::IMG_EXT_MAX);
			if (imageType > RomData::IMG_EXT_MAX && imageType != 0xFF) {
				// Invalid image type.
				continue;
			}
			if (p_cboImageType[imageType] && !imageTypeSet[imageType]) {
				// Set the image type.
				imageTypeSet[imageType] = true;
				if (imageType <= RomData::IMG_EXT_MAX) {
					imageTypes[sys][imageType] = nextPrio;
					// +1 because combobox index 0 is "No".
					ComboBox_SetCurSel(p_cboImageType[imageType], nextPrio+1);
					nextPrio++;
				}
			}
		}
	}

	// No longer changed.
	changed = false;
}

/**
 * Save the configuration.
 */
void ImageTypesTabPrivate::save(void)
{
	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();
	const rp_char *const filename = config->filename();
	if (!filename) {
		// No configuration filename...
		return;
	}

	// Image types are stored in the imageTypes[] array.
	const uint8_t *pImageTypes = imageTypes[0];

	wostringstream woss;
	for (unsigned int sys = 0; sys < SYS_COUNT; sys++) {
		// Is this system using the default configuration?
		if (sysIsDefault[sys]) {
			// Default configuration. Write an empty string.
			// NOTE: Passing NULL here deletes the entry.
			WritePrivateProfileString(L"ImageTypes", sysData[sys].classNameW, L"", RP2W_c(filename));
			pImageTypes += ARRAY_SIZE(imageTypes[0]);
			continue;
		}

		// Reset the wostringstream.
		woss.str(wstring());

		// Format of imageTypes[]:
		// - Index: Image type.
		// - Value: Priority.
		// We need to swap index and value.
		uint8_t imgTypePrio[RomData::IMG_EXT_MAX+1];
		memset(imgTypePrio, 0xFF, sizeof(imgTypePrio));
		for (unsigned int imageType = 0; imageType <= RomData::IMG_EXT_MAX;
		     imageType++, pImageTypes++)
		{
			if (*pImageTypes > RomData::IMG_EXT_MAX) {
				// Image type is either not valid for this system
				// or is set to "No".
				continue;
			}
			imgTypePrio[*pImageTypes] = imageType;
		}

		// Convert the image type priority to strings.
		// TODO: Export the string data from Config.
		static const wchar_t *const imageTypeNames[RomData::IMG_EXT_MAX+1] = {
			L"IntIcon",
			L"IntBanner",
			L"IntMedia",
			L"ExtMedia",
			L"ExtCover",
			L"ExtCover3D",
			L"ExtCoverFull",
			L"ExtBox",
		};
		static_assert(ARRAY_SIZE(imageTypeNames) == RomData::IMG_EXT_MAX+1, "imageTypeNames[] is the wrong size.");

		bool hasOne = false;
		for (unsigned int i = 0; i < ARRAY_SIZE(imgTypePrio); i++) {
			const uint8_t imageType = imgTypePrio[i];
			if (imageType <= RomData::IMG_EXT_MAX) {
				if (hasOne)
					woss << L',';
				hasOne = true;
				woss << imageTypeNames[imageType];
			}
		}

		if (hasOne) {
			// At least one image type is enabled.
			WritePrivateProfileString(L"ImageTypes", sysData[sys].classNameW, woss.str().c_str(), RP2W_c(filename));
		} else {
			// All image types are disabled.
			WritePrivateProfileString(L"ImageTypes", sysData[sys].classNameW, L"No", RP2W_c(filename));
		}
	}

	// No longer changed.
	changed = false;
}

/**
 * Update imageTypes in response to a user change.
 * @param id Control ID. (IDC_IMAGETYPES_CBOIMAGETYPE)
 * @param prio New priority. (0xFF == No; others == priority)
 */
void ImageTypesTabPrivate::updateImageType(unsigned int id, unsigned int prio)
{
	assert(hWndPropSheet != nullptr);
	if (!hWndPropSheet)
		return;

	// Base ID is 0x2000.
	// Image type is lower 4 bits.
	if (id < 0x2000)
		return;

	const unsigned int sys = (id - 0x2000) >> 4;
	if (sys >= SYS_COUNT)
		return;
	const unsigned int imageType = (id & 0x000F);
	if (imageType > RomData::IMG_EXT_MAX)
		return;

	if (prio != 0xFF) {
		// Check for any image types that have the new priority.
		const uint8_t prev_prio = imageTypes[sys][imageType];
		for (int i = RomData::IMG_EXT_MAX; i >= 0; i--) {
			if (i == imageType)
				continue;
			if (cboImageType[sys][i] != nullptr && imageTypes[sys][i] == prio) {
				// Found a match! Swap the priority.
				imageTypes[sys][i] = prev_prio;
				ComboBox_SetCurSel(cboImageType[sys][i], (prev_prio == 0xFF ? 0 : prev_prio+1));
				break;
			}
		}
	}

	// Save the image type ID.
	imageTypes[sys][imageType] = prio;

	// Mark this configuration as no longer being default.
	sysIsDefault[sys] = false;

	// Configuration has been changed.
	PropSheet_Changed(GetParent(hWndPropSheet), hWndPropSheet);
	changed = true;
}

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
			ImageTypesTabPrivate *d = static_cast<ImageTypesTabPrivate*>(
				GetProp(hDlg, D_PTR_PROP));
			if (!d) {
				// No ImageTypesTabPrivate. Can't do anything...
				return FALSE;
			}

			LPPSHNOTIFY lppsn = reinterpret_cast<LPPSHNOTIFY>(lParam);
			switch (lppsn->hdr.code) {
				case PSN_APPLY:
					// Save settings.
					if (d->changed) {
						d->save();
					}
					break;

				case NM_CLICK:
				case NM_RETURN:
					// SysLink control notification.
					if (lppsn->hdr.hwndFrom == GetDlgItem(hDlg, IDC_IMAGETYPES_CREDITS)) {
						// Open the URL.
						PNMLINK pNMLink = reinterpret_cast<PNMLINK>(lParam);
						ShellExecute(nullptr, L"open", pNMLink->item.szUrl, nullptr, nullptr, SW_SHOW);
					}
					break;

				default:
					break;
			}
			break;
		}

		case WM_COMMAND: {
			ImageTypesTabPrivate *d = static_cast<ImageTypesTabPrivate*>(
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
			int idx = ComboBox_GetCurSel((HWND)lParam);
			d->updateImageType(LOWORD(wParam), (idx <= 0 ? 0xFF : idx-1));

			// Allow the message to be processed by the system.
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

	extern HINSTANCE g_hInstance;

	PROPSHEETPAGE psp;
	psp.dwSize = sizeof(psp);	
	psp.dwFlags = PSP_USECALLBACK | PSP_USETITLE;
	psp.hInstance = g_hInstance;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_IMAGETYPES);
	psp.pszIcon = nullptr;
	psp.pszTitle = L"Image Types";
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
 * Save the contents of this tab.
 */
void ImageTypesTab::save(void)
{
	RP_D(ImageTypesTab);
	d->save();
}
