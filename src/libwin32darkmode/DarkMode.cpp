/**
 * Undocumented Win32 APIs for Dark Mode functionality.
 *
 * Based on ysc3839's win32-darkmode example application:
 * https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h
 * Copyright (c) 2019 Richard Yu
 * SPDX-License-Identifier: MIT
 *
 * See LICENSE.ysc3839 for more information.
 */

#include "libwin32common/RpWin32_sdk.h"
#include <tchar.h>

#include "DarkMode.hpp"
#include "IatHook.hpp"

// C includes (C++ namespace)
#include <cassert>

typedef void (WINAPI *fnRtlGetNtVersionNumbers)(LPDWORD major, LPDWORD minor, LPDWORD build);

// Functions added in Windows Vista
typedef int (WINAPI *fnCompareStringOrdinal)(LPCWCH lpString1, int cchCount1, LPCWCH lpString2, int cchCount2, BOOL bIgnoreCase);
// 1809 17763
typedef HTHEME (WINAPI *fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49

// Functions added in Windows Vista
static fnCompareStringOrdinal _CompareStringOrdinal = nullptr;
// Standard theming functions (uxtheme)
static fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1809 17763
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
// 1903 18362
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
static DWORD g_buildNumber = 0;

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow(hWnd) &&
		_ShouldAppsUseDarkMode() &&
		!IsHighContrast())
	{
		dark = TRUE;
	}

	if (g_buildNumber < 18362)
		SetProp(hWnd, _T("UseImmersiveDarkModeColors"), reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && _CompareStringOrdinal != nullptr &&
	    _CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

void AllowDarkModeForApp(bool allow)
{
	if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
	else if (_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? AllowDark : Default);
}

static HTHEME WINAPI MyOpenThemeData(HWND hWnd, LPCWSTR classList)
{
	if (lstrcmpW(classList, L"ScrollBar") == 0) {
		hWnd = nullptr;
		classList = L"Explorer::ScrollBar";
	}
	return _OpenNcThemeData(hWnd, classList);
};

void FixDarkScrollBar(void)
{
	HMODULE hComctl = LoadLibraryEx(_T("comctl32.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hComctl)
		return;

	auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
	if (!addr)
		return;

	DWORD oldProtect;
	if (!VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
		return;

	addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
	VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
}

static constexpr bool CheckBuildNumber(DWORD buildNumber)
{
	/*
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041); // 2004
	*/

	// Assume all versions of Windows 10 1809+,
	// and Windows 11, support these Dark Mode functions.
	return (buildNumber >= 17763);
}

/**
 * Initialize Dark Mode function pointers.
 * @return 0 if Dark Mode functionality is available; non-zero if not or an error occurred.
 */
int InitDarkModePFNs(void)
{
	if (g_darkModeSupported)
		return 0;

	HMODULE hNtDll = GetModuleHandle(_T("ntdll.dll"));
	assert(hNtDll != nullptr);
	if (!hNtDll) {
		// Uh oh, something's broken...
		return 1;
	}

	auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(
		GetProcAddress(hNtDll, "RtlGetNtVersionNumbers"));
	if (!RtlGetNtVersionNumbers) {
		return 2;
	}

	DWORD major, minor;
	RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
	g_buildNumber &= ~0xF0000000;
	if (major != 10 || minor != 0 || !CheckBuildNumber(g_buildNumber)) {
		// Not Windows 10, or not a supported build number.
		return 3;
	}

	HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != nullptr);
	if (!hKernel32) {
		return 4;
	}

	// Functions added in Windows Vista
	_CompareStringOrdinal = reinterpret_cast<fnCompareStringOrdinal>(GetProcAddress(hKernel32, "CompareStringOrdinal"));
	if (!_CompareStringOrdinal) {
		// If we don't even have a function from Vista,
		// we definitely won't have any Dark Mode functions.
		return 5;
	}

	HMODULE hUxtheme = LoadLibraryEx(_T("uxtheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hUxtheme) {
		return 6;
	}

	// Standard theming functions (uxtheme)
	_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));

	// 1809 17763
	_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
	_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
	_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
	_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));

	auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
	if (g_buildNumber < 18362) {
		_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
	} else {
		_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);
	}

	//_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
	_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(
		GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

	_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(
		GetProcAddress(GetModuleHandle(_T("user32.dll")), "SetWindowCompositionAttribute"));

	if (_OpenNcThemeData &&
	    // 1809 17763
	    _ShouldAppsUseDarkMode && _AllowDarkModeForWindow &&
	    (_AllowDarkModeForApp || _SetPreferredAppMode) &&
	    //_FlushMenuThemes &&
	    _RefreshImmersiveColorPolicyState && _IsDarkModeAllowedForWindow)
	{
		// Dark mode is supported.
		g_darkModeSupported = true;
		UpdateDarkModeEnabled();
		return 0;
	}

	// Dark mode is not supported. NULL out all the function pointers.
	_OpenNcThemeData = nullptr;
	// 1809 17763
	_ShouldAppsUseDarkMode = nullptr;
	_AllowDarkModeForWindow = nullptr;
	_AllowDarkModeForApp = nullptr;
	_FlushMenuThemes = nullptr;
	_RefreshImmersiveColorPolicyState = nullptr;
	_IsDarkModeAllowedForWindow = nullptr;
	_GetIsImmersiveColorUsingHighContrast = nullptr;
	// 1903 18362
	_SetWindowCompositionAttribute = nullptr;
	_ShouldSystemUseDarkMode = nullptr;
	_SetPreferredAppMode = nullptr;
	FreeLibrary(hUxtheme);
	return 7;
}

/**
 * Initialize Dark Mode.
 * @return 0 if Dark Mode functionality is available; non-zero if not or an error occurred.
 */
int InitDarkMode(void)
{
	int ret = InitDarkModePFNs();
	if (ret != 0)
		return ret;

	// Dark mode is supported.
	AllowDarkModeForApp(true);
	_RefreshImmersiveColorPolicyState();
	UpdateDarkModeEnabled();
	FixDarkScrollBar();
	return 0;
}

/**
 * Check if a dialog is really supposed to have a dark-colored background for Dark Mode.
 * Needed on Windows in cases where Dark Mode is enabled, but something like
 * StartAllBack isn't installed, resulting in properties dialogs using Light Mode.
 * @param hDlg Dialog handle
 * @return True if Dark Mode; false if not.
 */
bool VerifyDialogDarkMode(HWND hDlg)
{
	if (!g_darkModeEnabled)
		return false;

	// Get the dialog's background brush.
	HDC hDC = GetDC(hDlg);
	if (!hDC)
		return false;
	HBRUSH hBrush = reinterpret_cast<HBRUSH>(SendMessage(hDlg, WM_CTLCOLORDLG, reinterpret_cast<WPARAM>(hDC), reinterpret_cast<LPARAM>(hDlg)));
	ReleaseDC(hDlg, hDC);
	if (!hBrush)
		return false;

	// Get the color from the background brush.
	LOGBRUSH lbr;
	if (GetObject(hBrush, sizeof(lbr), &lbr) != sizeof(lbr) ||
		lbr.lbStyle != BS_SOLID)
	{
		// Failed to get the brush, or it's not a solid color brush.
		return false;
	}

	// Quick and dirty: If (R+G+B)/3 >= 128, assume light mode.
	unsigned int avg = GetRValue(lbr.lbColor) + GetGValue(lbr.lbColor) + GetBValue(lbr.lbColor);
	avg /= 3;
	return (avg < 0x80);
}
