// https://github.com/ysc3839/win32-darkmode

#include "libwin32common/RpWin32_sdk.h"

#include "DarkMode.hpp"
#include "IatHook.hpp"

// for HTHEME

// 1809 17763
typedef HTHEME (WINAPI *fnOpenNcThemeData)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49

fnSetWindowTheme _SetWindowTheme = nullptr;
fnGetThemeColor _GetThemeColor = nullptr;
fnCloseThemeData _CloseThemeData = nullptr;
fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenThemeData _OpenThemeData = nullptr;
static fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1903 18362
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
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
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
	HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041); // 2004
}

/**
 * Initialize Dark Mode.
 * @return 0 if Dark Mode functionality is available; non-zero if not or an error occurred.
 */
int InitDarkMode(void)
{
	auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
	if (!RtlGetNtVersionNumbers)
		return 1;

	DWORD major, minor;
	RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
	g_buildNumber &= ~0xF0000000;
	if (major != 10 || minor != 0 || !CheckBuildNumber(g_buildNumber)) {
		// Not Windows 10, or not a supported build number.
		return 2;
	}

	HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!hUxtheme)
		return 3;

	_SetWindowTheme = reinterpret_cast<fnSetWindowTheme>(GetProcAddress(hUxtheme, "SetWindowTheme"));
	_GetThemeColor = reinterpret_cast<fnGetThemeColor>(GetProcAddress(hUxtheme, "GetThemeColor"));
	_CloseThemeData = reinterpret_cast<fnCloseThemeData>(GetProcAddress(hUxtheme, "CloseThemeData"));
	_OpenThemeData = reinterpret_cast<fnOpenThemeData>(GetProcAddress(hUxtheme, "OpenThemeData"));
	_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
	_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
	_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
	_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
	_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

	auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
	if (g_buildNumber < 18362) {
		_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
	} else {
		_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);
	}

	//_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
	_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

	_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

	if (_SetWindowTheme &&
		_GetThemeColor &&
		_CloseThemeData &&
		_OpenThemeData &&
		_OpenNcThemeData &&
		_RefreshImmersiveColorPolicyState &&
		_ShouldAppsUseDarkMode &&
		_AllowDarkModeForWindow &&
		(_AllowDarkModeForApp || _SetPreferredAppMode) &&
		//_FlushMenuThemes &&
		_IsDarkModeAllowedForWindow)
	{
		// Dark mode is supported.
		g_darkModeSupported = true;

		AllowDarkModeForApp(true);
		_RefreshImmersiveColorPolicyState();

		g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();

		FixDarkScrollBar();
		return 0;
	}

	// Dark mode is not supported.
	_SetWindowTheme = nullptr;
	_GetThemeColor = nullptr;
	_CloseThemeData = nullptr;
	_OpenNcThemeData = nullptr;
	_RefreshImmersiveColorPolicyState = nullptr;
	_GetIsImmersiveColorUsingHighContrast = nullptr;
	_ShouldAppsUseDarkMode = nullptr;
	_AllowDarkModeForWindow = nullptr;
	_AllowDarkModeForApp = nullptr;
	_SetPreferredAppMode = nullptr;
	_IsDarkModeAllowedForWindow = nullptr;
	_SetWindowCompositionAttribute = nullptr;
	FreeLibrary(hUxtheme);
	return 4;
}
