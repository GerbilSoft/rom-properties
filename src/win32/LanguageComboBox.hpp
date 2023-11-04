/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * LanguageComboBox.hpp: Language ComboBoxEx superclass.                   *
 *                                                                         *
 * Copyright (c) 2017-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WC_LANGUAGECOMBOBOX		_T("rp-LanguageComboBox")

void LanguageComboBoxRegister(void);
void LanguageComboBoxUnregister(void);

// NOTE: ComboBoxEx uses WM_USER+1 through WM_USER+14.
#define WM_LCB_BASE			(WM_USER + 20)

#define WM_LCB_SET_LCS			(WM_LCB_BASE + 1)	// lParam == 0-terminated uint32_t array of LCs
//#define WM_LCB_GET_LCS			(WM_LCB_BASE + 2)	// lParam == ???? [TODO: Figure out how to implement this.]
#define WM_LCB_SET_SELECTED_LC		(WM_LCB_BASE + 3)	// wParam == lc
#define WM_LCB_GET_SELECTED_LC		(WM_LCB_BASE + 4)	// return == selected LC
#define WM_LCB_GET_MIN_SIZE		(WM_LCB_BASE + 5)	// return == packed width/height (use GET_?_LPARAM)
#define WM_LCB_SET_FORCE_PAL		(WM_LCB_BASE + 6)	// wParam == forcePAL (must set LCs afterwards)
#define WM_LCB_GET_FORCE_PAL		(WM_LCB_BASE + 7)	// return == forcePAL

// TODO: Intercept ComboBoxEx's WM_NOTIFY somehow.
//#define LCBN_FIRST			(NM_LAST - 2600U)
//#define LCBN_LC_CHANGED			(LCBN_FIRST - 1)	// FIXME: Include the new LC somewhere?

static inline void LanguageComboBox_SetLCs(HWND hWnd, const uint32_t *lcs_array)
{
	SendMessage(hWnd, WM_LCB_SET_LCS, 0, reinterpret_cast<LPARAM>(lcs_array));
}

static inline bool LanguageComboBox_SetSelectedLC(HWND hWnd, uint32_t lc)
{
	return static_cast<bool>(SendMessage(hWnd, WM_LCB_SET_SELECTED_LC, static_cast<WPARAM>(lc), 0));
}

static inline uint32_t LanguageComboBox_GetSelectedLC(HWND hWnd)
{
	return static_cast<uint32_t>(SendMessage(hWnd, WM_LCB_GET_SELECTED_LC, 0, 0));
}

static inline LPARAM LanguageComboBox_GetMinSize(HWND hWnd)
{
	return SendMessage(hWnd, WM_LCB_GET_MIN_SIZE, 0, 0);
}

static inline bool LanguageComboBox_SetForcePAL(HWND hWnd, bool forcePAL)
{
	return static_cast<bool>(SendMessage(hWnd, WM_LCB_SET_FORCE_PAL, static_cast<WPARAM>(forcePAL), 0));
}

static inline bool LanguageComboBox_GetForcePAL(HWND hWnd)
{
	return static_cast<bool>(SendMessage(hWnd, WM_LCB_GET_FORCE_PAL, 0, 0));
}

#ifdef __cplusplus
}
#endif
