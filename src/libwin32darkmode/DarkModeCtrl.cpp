/** DarkMode control helpers **/
#include "DarkModeCtrl.hpp"
#include "DarkMode.hpp"
#include "TGDarkMode.hpp"	// TortoiseGit dark mode subclasses

// gcc branch prediction hints.
// Should be used in combination with profile-guided optimization.
#ifdef __GNUC__
#  define likely(x)	__builtin_expect(!!(x), 1)
#  define unlikely(x)	__builtin_expect(!!(x), 0)
#else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
#endif

/**
 * Initialize dark mode for a Dialog control.
 * If top-level, the title bar will be initialized as well.
 * @param hDlg Dialog control handle
 */
void DarkMode_InitDialog(HWND hDlg)
{
	if (unlikely(!g_darkModeSupported))
		return;

	_SetWindowTheme(hDlg, L"CFD", NULL);
	_AllowDarkModeForWindow(hDlg, true);

	const LONG_PTR lpStyle = GetWindowLongPtr(hDlg, GWL_STYLE);
	if (!(lpStyle & WS_CHILD)) {
		// Top-level window
		RefreshTitleBarThemeColor(hDlg);
	}

	SendMessageW(hDlg, WM_THEMECHANGED, 0, 0);
}

/**
 * Initialize dark mode for a Button control.
 * @param hWnd Control handle
 */
void DarkMode_InitButton(HWND hWnd)
{
	if (unlikely(!g_darkModeSupported))
		return;

	// FIXME: Not working for BS_GROUPBOX or BS_AUTOCHECKBOX.
	_SetWindowTheme(hWnd, L"Explorer", NULL);
	_AllowDarkModeForWindow((hWnd), true);

	const LONG_PTR lpStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
	switch (lpStyle & BS_TYPEMASK) {
		default:
			break;
		case BS_GROUPBOX:
		case BS_CHECKBOX:
		case BS_AUTOCHECKBOX:
		case BS_3STATE:
		case BS_AUTO3STATE:
		case BS_RADIOBUTTON:
		case BS_AUTORADIOBUTTON:
			// Groupbox, checkbox, or radio button.
			// Need to subclass it for proper text colors.
			SetWindowSubclass(hWnd, TGDarkMode_ButtonSubclassProc, TGDarkMode_SubclassID, 0);
			break;
	}

	SendMessage((hWnd), WM_THEMECHANGED, 0, 0);
}

/**
 * Initialize dark mode for a ComboBox control.
 * @param hWnd ComboBox control handle
 */
void DarkMode_InitComboBox(HWND hWnd)
{
	if (unlikely(!g_darkModeSupported))
		return;

	// FIXME: Not working for ComboBoxEx.
	_SetWindowTheme(hWnd, L"CFD", NULL);
	_AllowDarkModeForWindow((hWnd), true);
	SendMessage((hWnd), WM_THEMECHANGED, 0, 0);
}

/**
 * Initialize dark mode for an Edit control.
 * @param hWnd Edit control handle
 */
void DarkMode_InitEdit(HWND hWnd)
{
	if (unlikely(!g_darkModeSupported))
		return;

	_SetWindowTheme(hWnd, L"CFD", NULL);
	_AllowDarkModeForWindow((hWnd), true);
	SendMessage((hWnd), WM_THEMECHANGED, 0, 0);
}
