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

/** Initialize dark mode for specific types of controls **/

void DarkMode_InitButton(HWND hWnd);
void DarkMode_InitComboBox(HWND hWnd);
void DarkMode_InitComboBoxEx(HWND hWnd);
void DarkMode_InitEdit(HWND hWnd);
void DarkMode_InitRichEdit(HWND hWnd);

/** Same as above, but with GetDlgItem wrappers **/

#define DARKMODE_GETDLGITEM_WRAPPER(ControlName) \
static inline void DarkMode_Init##ControlName##_Dlg(HWND hDlg, WORD id) \
{ \
	HWND hWnd = GetDlgItem(hDlg, id); \
	assert(hWnd); \
	DarkMode_Init##ControlName(hWnd); \
}

DARKMODE_GETDLGITEM_WRAPPER(Button)
DARKMODE_GETDLGITEM_WRAPPER(ComboBox)
DARKMODE_GETDLGITEM_WRAPPER(ComboBoxEx)
DARKMODE_GETDLGITEM_WRAPPER(Edit)
DARKMODE_GETDLGITEM_WRAPPER(RichEdit)

#ifdef __cplusplus
}
#endif
