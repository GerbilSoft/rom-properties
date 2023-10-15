/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LanguageComboBox.cpp: Language ComboBoxEx superclass.                   *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "LanguageComboBox.hpp"
#include "FlagSpriteSheet.hpp"

// Other rom-properties libraries
#include "librpbase/SystemRegion.hpp"
using namespace LibRpBase;

// C++ STL classes
using std::shared_ptr;
using std::tstring;
using std::vector;

static ATOM atom_languageComboBox;
static WNDPROC pfnComboBoxExWndProc;

class LanguageComboBoxPrivate
{
	public:
		explicit LanguageComboBoxPrivate(HWND hWnd)
			: hWnd(hWnd)
			, himglFlags(nullptr)
			, dwExStyleRTL(LibWin32UI::isSystemRTL())
			, forcePAL(false)
		{
			minSize.cx = 0;
			minSize.cy = 0;
		}

		~LanguageComboBoxPrivate()
		{
			if (himglFlags) {
				ImageList_Destroy(himglFlags);
			}
		}

		/**
		 * Set the language codes.
		 * @param lcs_array 0-terminated array of language codes.
		 * @return TRUE on success; FALSE on error.
		 */
		LRESULT setLCs(const uint32_t *lcs_array);

		/**
		 * Set the selected language code.
		 *
		 * NOTE: This function will return true if the LC was found,
		 * even if it was already selected.
		 *
		 * @param lc Language code. (0 to unselect)
		 * @return TRUE if set; FALSE if LC was not found.
		 */
		LRESULT setSelectedLC(uint32_t lc);

		/**
		 * Set the selected language code.
		 * @return Selected language code. (0 if none)
		 */
		uint32_t getSelectedLC(void) const;

	public:
		HWND hWnd;		// LanguageComboBox control
		HIMAGELIST himglFlags;	// ImageList for flag icons

		// Is the UI locale right-to-left?
		// If so, this will be set to WS_EX_LAYOUTRTL.
		DWORD dwExStyleRTL;

		SIZE minSize;		// Minimum size required
		bool forcePAL;		// Force PAL region flags?
};

/**
 * Set the language codes.
 * @param lcs_array 0-terminated array of language codes.
 * @return TRUE on success; FALSE on error.
 */
LRESULT LanguageComboBoxPrivate::setLCs(const uint32_t *lcs_array)
{
	assert(lcs_array != nullptr);
	if (!lcs_array) {
		return FALSE;
	}

	// Check the LC of the selected index.
	const uint32_t sel_lc = getSelectedLC();

	// Clear the current ImageList.
	if (himglFlags) {
		SendMessage(hWnd, CBEM_SETIMAGELIST, 0, (LPARAM)nullptr);
		ImageList_Destroy(himglFlags);
		himglFlags = nullptr;
	}
	// Clear the ComboBoxEx.
	for (int i = ComboBox_GetCount(hWnd)-1; i >= 0; i--) {
		SendMessage(hWnd, CBEM_DELETEITEM, i, 0);
	}

	// TODO:
	// - Per-monitor DPI scaling (both v1 and v2)
	// - Handle WM_DPICHANGED.
	// Reference: https://docs.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows
	const UINT dpi = rp_GetDpiForWindow(hWnd);
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

	// Create the ImageList.
	himglFlags = ImageList_Create(iconSize, iconSize, ILC_COLOR32, 13, 16);
	assert(himglFlags != nullptr);

	// Load the flags sprite sheet.
	// TODO: Is premultiplied alpha needed?
	// Reference: https://stackoverflow.com/questions/307348/how-to-draw-32-bit-alpha-channel-bitmaps
	// NOTE: WS_EX_LAYOUTRTL will flip bitmaps in the dropdown box.
	// ILC_MIRROR mirrors the bitmaps if the process is mirrored,
	// but we can't rely on that being the case, and this option
	// was first introduced in Windows XP.
	// We'll flip the image here to counteract it. (flipH == true)
	FlagSpriteSheet flagSpriteSheet(iconSize, (dwExStyleRTL != 0));

	// Initialize the minimum size.
	HFONT hFont = GetWindowFont(hWnd);
	minSize.cx = 0;
	minSize.cy = 0;

	// Add the language entries.
	COMBOBOXEXITEM cbItem;
	cbItem.iItem = 0;
	int iImage = 0;
	if (himglFlags) {
		// We have images.
		cbItem.mask = CBEIF_TEXT | CBEIF_LPARAM | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	} else {
		// No images.
		cbItem.mask = CBEIF_TEXT | CBEIF_LPARAM;
		cbItem.iImage = -1;
		cbItem.iSelectedImage = -1;
	}

	int sel_idx = -1;
	for (; *lcs_array != 0; lcs_array++, cbItem.iItem++) {
		const uint32_t lc = *lcs_array;
		const char *lc_str = SystemRegion::getLocalizedLanguageName(lc);

		tstring s_lc;
		if (lc_str) {
			s_lc = U82T_c(lc_str);
		} else {
			// Invalid language code.
			s_lc = SystemRegion::lcToTString(lc);
		}

		SIZE size;
		if (!LibWin32UI::measureTextSize(hWnd, hFont, s_lc.c_str(), &size)) {
			minSize.cx = std::max(minSize.cx, size.cx);
			minSize.cy = std::max(minSize.cy, size.cy);
		}

		if (himglFlags) {
			// Get the flag icon.
			HBITMAP hbmIcon = flagSpriteSheet.getIcon(lc, forcePAL, dpi);
			assert(hbmIcon != nullptr);
			if (hbmIcon) {
				// Add the icon to the ImageList.
				ImageList_Add(himglFlags, hbmIcon, nullptr);
				DeleteBitmap(hbmIcon);

				// We're setting an image for this entry.
				cbItem.iImage = iImage;
				cbItem.iSelectedImage = iImage;

				// Increment the image index for the next item.
				iImage++;
			} else {
				// No image for this entry.
				// TODO: Verify this!
				cbItem.iImage = -1;
				cbItem.iSelectedImage = -1;
			}
		}

		cbItem.pszText = const_cast<LPTSTR>(s_lc.data());
		cbItem.cchTextMax = static_cast<int>(s_lc.size());
		cbItem.lParam = static_cast<LPARAM>(lc);

		// Insert the item.
		SendMessage(hWnd, CBEM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&cbItem));

		if (sel_lc != 0 && lc == sel_lc) {
			// This was the previously-selected LC.
			sel_idx = static_cast<int>(cbItem.iItem);
		}
	}

	// Add iconSize + iconMargin for the icon.
	minSize.cx += iconSize + iconMargin;

	// Add vertical scrollbar width and CXEDGE.
	// Reference: http://ntcoder.com/2013/10/07/mfc-resize-ccombobox-drop-down-list-based-on-contents/
	minSize.cx += rp_GetSystemMetricsForDpi(SM_CXVSCROLL, dpi);
	minSize.cx += (rp_GetSystemMetricsForDpi(SM_CXEDGE, dpi) * 4);

	if (himglFlags) {
		// Set the new ImageList.
		SendMessage(hWnd, CBEM_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himglFlags));
	}

	// Re-select the previously-selected LC.
	ComboBox_SetCurSel(hWnd, sel_idx);
	return TRUE;
}

/**
 * Set the selected language code.
 *
 * NOTE: This function will return true if the LC was found,
 * even if it was already selected.
 *
 * @param lc Language code. (0 to unselect)
 * @return TRUE if set; FALSE if LC was not found.
 */
LRESULT LanguageComboBoxPrivate::setSelectedLC(uint32_t lc)
{
	// Check if this LC is already selected.
	if (lc == getSelectedLC()) {
		// Already selected.
		return TRUE;
	}

	LRESULT lRet;
	if (lc == 0) {
		// Unselect the selected LC.
		ComboBox_SetCurSel(hWnd, -1);
		lRet = TRUE;
	} else {
		// Find an item with a matching LC.
		lRet = FALSE;
		for (int i = ComboBox_GetCount(hWnd)-1; i >= 0; i--) {
			if (lc == static_cast<uint32_t>(ComboBox_GetItemData(hWnd, i))) {
				// Found it.
				ComboBox_SetCurSel(hWnd, i);
				lRet = TRUE;
				break;
			}
		}
	}

	return lRet;
}

/**
 * Set the selected language code.
 * @return Selected language code. (0 if none)
 */
uint32_t LanguageComboBoxPrivate::getSelectedLC(void) const
{
	const int index = ComboBox_GetCurSel(hWnd);
	return (index >= 0 ? static_cast<uint32_t>(ComboBox_GetItemData(hWnd, index)) : 0);
}

/** LanguageComboBox **/

static LRESULT CALLBACK
LanguageComboBoxWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// FIXME: Don't use GWLP_USERDATA; use extra window bytes?
	switch (uMsg) {
		default:
			break;

		case WM_NCCREATE: {
			LanguageComboBoxPrivate *const d = new LanguageComboBoxPrivate(hWnd);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(d));
			break;
		}

		case WM_NCDESTROY: {
			LanguageComboBoxPrivate *const d = reinterpret_cast<LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			delete d;
			break;
		}

		case WM_LCB_SET_LCS: {
			LanguageComboBoxPrivate *const d = reinterpret_cast<LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->setLCs(reinterpret_cast<const uint32_t*>(lParam));
		}

		case WM_LCB_SET_SELECTED_LC: {
			LanguageComboBoxPrivate *const d = reinterpret_cast<LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->setSelectedLC(static_cast<uint32_t>(wParam));
		}

		case WM_LCB_GET_SELECTED_LC: {
			const LanguageComboBoxPrivate *const d = reinterpret_cast<const LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->getSelectedLC();
		}

		case WM_LCB_GET_MIN_SIZE: {
			const LanguageComboBoxPrivate *const d = reinterpret_cast<const LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));

			// Pack the minimum size into an LPARAM.
			return (short)d->minSize.cx | (((short)d->minSize.cy) << 16);
		}

		case WM_LCB_SET_FORCE_PAL: {
			LanguageComboBoxPrivate *const d = reinterpret_cast<LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));

			// TODO: Update icons. For now, LCs must be set
			// after setting forcePAL.
			d->forcePAL = static_cast<bool>(wParam);
			return TRUE;
		}

		case WM_LCB_GET_FORCE_PAL: {
			const LanguageComboBoxPrivate *const d = reinterpret_cast<const LanguageComboBoxPrivate*>(
				GetWindowLongPtr(hWnd, GWLP_USERDATA));
			return d->forcePAL;
		}
	}

	// Forward the message to the ComboBoxEx class.
	return CallWindowProc(pfnComboBoxExWndProc, hWnd, uMsg, wParam, lParam);
}

void LanguageComboBoxRegister(void)
{
	if (atom_languageComboBox != 0)
		return;

	// LanguageComboBox is superclassing ComboBoxEx.

	// Get ComboBoxEx class information.
	// NOTE: ComboBoxEx was introduced in MSIE 3.0, so we don't
	// need to fall back to regular ComboBox.
	WNDCLASS wndClass;
	BOOL bRet = GetClassInfo(nullptr, WC_COMBOBOXEX, &wndClass);
	assert(bRet != FALSE);
	if (!bRet) {
		// Error getting class info.
		return;
	}

	pfnComboBoxExWndProc = wndClass.lpfnWndProc;
	wndClass.lpfnWndProc = LanguageComboBoxWndProc;
	wndClass.style &= ~CS_GLOBALCLASS;
	wndClass.hInstance = HINST_THISCOMPONENT;
	wndClass.lpszClassName = WC_LANGUAGECOMBOBOX;

	atom_languageComboBox = RegisterClass(&wndClass);
}

void LanguageComboBoxUnregister(void)
{
	if (atom_languageComboBox != 0) {
		UnregisterClass(MAKEINTATOM(atom_languageComboBox), HINST_THISCOMPONENT);
		atom_languageComboBox = 0;
	}
}
