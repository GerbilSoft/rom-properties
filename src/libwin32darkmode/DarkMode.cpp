// https://github.com/ysc3839/win32-darkmode

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

	auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) -> HTHEME {
		if (wcscmp(classList, L"ScrollBar") == 0) {
			hWnd = nullptr;
			classList = L"Explorer::ScrollBar";
		}
		return _OpenNcThemeData(hWnd, classList);
	};

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
 * Initialize Dark Mode.
 * @return 0 if Dark Mode functionality is available; non-zero if not or an error occurred.
 */
int InitDarkMode(void)
{
	auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(
		GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetNtVersionNumbers"));
	if (!RtlGetNtVersionNumbers)
		return 1;

	DWORD major, minor;
	RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
	g_buildNumber &= ~0xF0000000;
	if (major != 10 || minor != 0 || !CheckBuildNumber(g_buildNumber)) {
		// Not Windows 10, or not a supported build number.
		return 2;
	}

	HMODULE hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	assert(hKernel32 != nullptr);
	if (!hKernel32)
		return 3;

	// Functions added in Windows Vista
	_CompareStringOrdinal = reinterpret_cast<fnCompareStringOrdinal>(GetProcAddress(hKernel32, "CompareStringOrdinal"));
	if (!_CompareStringOrdinal) {
		// If we don't even have a function from Vista,
		// we definitely won't have any Dark Mode functions.
		return 4;
	}

	HMODULE hUxtheme = LoadLibraryEx(_T("uxtheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hUxtheme)
		return 5;

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

		AllowDarkModeForApp(true);
		_RefreshImmersiveColorPolicyState();

		UpdateDarkModeEnabled();

		FixDarkScrollBar();
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
	return 6;
}
