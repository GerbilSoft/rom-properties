#pragma once

/** DarkMode control helpers **/
#include "libwin32common/RpWin32_sdk.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize dark mode for a Dialog control.
 * If top-level, the title bar will be initialized as well.
 * @param hDlg Dialog control handle
 */
void DarkMode_InitDialog(HWND hDlg);

/**
 * Initialize dark mode for a Button control.
 * @param hWnd Button control handle
 */
void DarkMode_InitButton(HWND hWnd);

/**
 * Initialize dark mode for a Button control in a dialog.
 * @param hDlg Dialog handle
 * @param id Button control ID
 */
static inline void DarkMode_InitButton_Dlg(HWND hDlg, WORD id)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	assert(hWnd != nullptr);
	DarkMode_InitButton(hWnd);
}

/**
 * Initialize dark mode for a ComboBox control.
 * @param hWnd ComboBox control handle
 */
void DarkMode_InitComboBox(HWND hWnd);

/**
 * Initialize dark mode for a ComboBox control in a dialog.
 * @param hDlg Dialog handle
 * @param id ComboBox control ID
 */
static inline void DarkMode_InitComboBox_Dlg(HWND hDlg, WORD id)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	assert(hWnd != nullptr);
	DarkMode_InitComboBox(hWnd);
}

/**
 * Initialize dark mode for a ComboBoxEx control.
 * @param hWnd ComboBox control handle
 */
void DarkMode_InitComboBoxEx(HWND hWnd);

/**
 * Initialize dark mode for a ComboBoxEx control in a dialog.
 * @param hDlg Dialog handle
 * @param id ComboBox control ID
 */
static inline void DarkMode_InitComboBoxEx_Dlg(HWND hDlg, WORD id)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	assert(hWnd != nullptr);
	DarkMode_InitComboBoxEx(hWnd);
}

/**
 * Initialize dark mode for an Edit control.
 * @param hWnd Edit control handle
 */
void DarkMode_InitEdit(HWND hWnd);

/**
 * Initialize dark mode for an Edit control in a dialog.
 * @param hDlg Dialog handle
 * @param id Edit control ID
 */
static inline void DarkMode_InitEdit_Dlg(HWND hDlg, WORD id)
{
	HWND hWnd = GetDlgItem(hDlg, id);
	assert(hWnd != nullptr);
	DarkMode_InitEdit(hWnd);
}

#ifdef __cplusplus
}
#endif
