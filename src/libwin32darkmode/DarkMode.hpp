// https://github.com/ysc3839/win32-darkmode
#pragma once

// for HTHEME
#include <uxtheme.h>

// for bool in C (FIXME: Functions are probably returning HRESULT...)
#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
} IMMERSIVE_HC_CACHE_MODE;

// 1903 18362
typedef enum {
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
} PreferredAppMode;

typedef enum {
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
} WINDOWCOMPOSITIONATTRIB;

typedef struct _WINDOWCOMPOSITIONATTRIBDATA {
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
} WINDOWCOMPOSITIONATTRIBDATA;

// Standard theming functions (uxtheme)
typedef HRESULT (WINAPI *fnSetWindowTheme)(HWND hWnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
typedef HRESULT (WINAPI *fnGetThemeColor)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF *pColor);
typedef HTHEME (WINAPI *fnOpenThemeData)(HWND hWnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI *fnCloseThemeData)(HTHEME hTheme);
// Theming functions used by TGDarkMode
typedef HPAINTBUFFER (WINAPI *fnBeginBufferedPaint)(HDC hdcTarget, const RECT *prcTarget, BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams, HDC *phdc);
typedef HRESULT (WINAPI *fnBufferedPaintSetAlpha)(HPAINTBUFFER hBufferedPaint, const RECT *prc, BYTE alpha);
typedef HRESULT (WINAPI *fnDrawThemeBackground)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
typedef HRESULT (WINAPI *fnEndBufferedPaint)(HPAINTBUFFER hBufferedPaint, BOOL fUpdateTarget);
typedef HRESULT (WINAPI *fnGetThemeBackgroundContentRect)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect);
typedef HRESULT (WINAPI *fnGetThemeInt)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, int *piVal);
// 1809 17763
typedef bool (WINAPI *fnShouldAppsUseDarkMode)(void); // ordinal 132
typedef bool (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, bool allow); // ordinal 133
typedef bool (WINAPI *fnAllowDarkModeForApp)(bool allow); // ordinal 135, in 1809
typedef void (WINAPI *fnFlushMenuThemes)(void); // ordinal 136
typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)(void); // ordinal 104
typedef bool (WINAPI *fnIsDarkModeAllowedForWindow)(HWND hWnd); // ordinal 137
typedef bool (WINAPI *fnGetIsImmersiveColorUsingHighContrast)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
// 1903 18362
typedef BOOL (WINAPI *fnSetWindowCompositionAttribute)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
typedef bool (WINAPI *fnShouldSystemUseDarkMode)(void); // ordinal 138
typedef PreferredAppMode (WINAPI *fnSetPreferredAppMode)(PreferredAppMode appMode); // ordinal 135, in 1903
typedef bool (WINAPI *fnIsDarkModeAllowedForApp)(void); // ordinal 139

// Standard theming functions (uxtheme)
extern fnSetWindowTheme _SetWindowTheme;
extern fnGetThemeColor _GetThemeColor;
extern fnOpenThemeData _OpenThemeData;
extern fnCloseThemeData _CloseThemeData;
// Theming functions used by TGDarkMode
extern fnBeginBufferedPaint _BeginBufferedPaint;
extern fnBufferedPaintSetAlpha _BufferedPaintSetAlpha;
extern fnDrawThemeBackground _DrawThemeBackground;
extern fnEndBufferedPaint _EndBufferedPaint;
extern fnGetThemeBackgroundContentRect _GetThemeBackgroundContentRect;
extern fnGetThemeInt _GetThemeInt;
// 1809 17763
extern fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode;
extern fnAllowDarkModeForWindow _AllowDarkModeForWindow;
extern fnAllowDarkModeForApp _AllowDarkModeForApp;
extern fnFlushMenuThemes _FlushMenuThemes;
extern fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState;
extern fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow;
extern fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast;
// 1903 18362
extern fnSetWindowCompositionAttribute _SetWindowCompositionAttribute;
extern fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode;
extern fnSetPreferredAppMode _SetPreferredAppMode;

extern bool g_darkModeSupported;
extern bool g_darkModeEnabled;

static inline bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (g_darkModeSupported)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

static inline bool IsHighContrast(void)
{
	HIGHCONTRASTW highContrast = { sizeof(highContrast) };
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
		return highContrast.dwFlags & HCF_HIGHCONTRASTON;
	return false;
}

void RefreshTitleBarThemeColor(HWND hWnd);
bool IsColorSchemeChangeMessage(LPARAM lParam);

void AllowDarkModeForApp(bool allow);
void FixDarkScrollBar(void);

/**
 * Initialize Dark Mode.
 * @return 0 if Dark Mode functionality is available; non-zero if not or an error occurred.
 */
int InitDarkMode(void);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static inline bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}
#endif /* __cplusplus */
