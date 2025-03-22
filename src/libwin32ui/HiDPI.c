/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * HiDPI.c: High DPI wrapper functions.                                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "config.libwin32ui.h"
#include "HiDPI.h"

#include "tcharx.h"

// librpthreads
#include "pthread_once.h"

#ifdef HAVE_SHELLSCALINGAPI_H
//# include <shellscalingapi.h>
#endif

// MONITOR_DPI_TYPE is defined in shellscalingapi.h.
// If that header isn't available, we'll define it here.

#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI = 1,
	MDT_RAW_DPI = 2,
	MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#  define DPI_ENUMS_DECLARED
#endif /* !DPI_ENUMS_DECLARED */

/** DPI functions. **/

// Windows 10 v1607
typedef UINT (WINAPI *pfnGetDpiForWindow_t)(_In_ HWND hWnd);

// Windows 8.1
typedef HRESULT (WINAPI *pfnGetDpiForMonitor_t)(
	_In_ HMONITOR hmonitor,
	_In_ MONITOR_DPI_TYPE dpiType,
	_Out_ UINT *dpiX,
	_Out_ UINT *dpiY);

// pthread_once() control variable.
static pthread_once_t hidpi_once_control = PTHREAD_ONCE_INIT;

// Function pointers.
static union {
	pfnGetDpiForWindow_t pfnGetDpiForWindow;
	pfnGetDpiForMonitor_t pfnGetDpiForMonitor;
} pfns;

// shcore.dll (Windows 8.1)
// NOTE: We have to ensure that this is unloaded properly!
static HMODULE hShcore_dll = NULL;

// DPI query type.
typedef enum {
	DPIQT_GetDeviceCaps = 0,
	DPIQT_GetDpiForMonitor,
	DPIQT_GetDpiForWindow,
} DPIQueryType;
static DPIQueryType dpiQueryType = DPIQT_GetDeviceCaps;

/**
 * Initialize the DPI function pointers.
 * Called by pthread_once();
 */
static void rp_init_DPIQueryType(void)
{
	// Try GetDpiForWindow(). (Windows 10 v1607)
	HMODULE hUser32_dll = GetModuleHandle(_T("user32.dll"));
	if (hUser32_dll) {
		pfns.pfnGetDpiForWindow = (pfnGetDpiForWindow_t)GetProcAddress(hUser32_dll, "GetDpiForWindow");
		if (pfns.pfnGetDpiForWindow) {
			// Found GetDpiForWindow().
			dpiQueryType = DPIQT_GetDpiForWindow;
			return;
		}
	}

	// Try GetDpiForMonitor(). (Windows 8.1)
	hShcore_dll = LoadLibraryEx(_T("shcore.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hShcore_dll) {
		pfns.pfnGetDpiForMonitor = (pfnGetDpiForMonitor_t)GetProcAddress(hShcore_dll, "GetDpiForMonitor");
		if (pfns.pfnGetDpiForMonitor) {
			// Found GetDpiForMonitor().
			dpiQueryType = DPIQT_GetDpiForMonitor;
			return;
		}
		// Not found. Unload the DLL.
		FreeLibrary(hShcore_dll);
		hShcore_dll = NULL;
	}

	// None of those functions are available.
	// Fall back to system-wide DPI.
	dpiQueryType = DPIQT_GetDeviceCaps;
}

/**
 * Unload modules and reset the DPI configuration.
 * This should be done on DLL exit.
 */
void rp_DpiUnloadModules(void)
{
	if (hShcore_dll) {
		FreeLibrary(hShcore_dll);
		hShcore_dll = NULL;
	}
	hidpi_once_control = PTHREAD_ONCE_INIT;
}

/**
 * Get the DPI for the specified window.
 * @param hWnd Window handle.
 * @return DPI, or 0 on error.
 */
UINT rp_GetDpiForWindow(HWND hWnd)
{
	UINT dpi = 0;
	pthread_once(&hidpi_once_control, rp_init_DPIQueryType);

	switch (dpiQueryType) {
		default:
		case DPIQT_GetDeviceCaps: {
			// Windows 7 and earlier: System-wide DPI.
			// NOTE: Assuming dpiX is the same as dpiY.
			HDC hDC = GetDC(NULL);
			dpi = (UINT)GetDeviceCaps(hDC, LOGPIXELSX);
			ReleaseDC(NULL, hDC);
			break;
		}

		case DPIQT_GetDpiForMonitor: {
			// Windows 8.1: Per-monitor DPI.
			UINT dpiX, dpiY;

			// Get the monitor closest to the specified window.
			HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

			// Get the monitor DPI.
			// NOTE: dpiX is the same as dpiY according to MSDN.
			// https://docs.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor
			HRESULT hr = pfns.pfnGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
			if (SUCCEEDED(hr)) {
				dpi = dpiX;
			}
			break;
		}

		case DPIQT_GetDpiForWindow: {
			// Windows 10 v1607: Per-montior DPI v2.
			dpi = pfns.pfnGetDpiForWindow(hWnd);
			break;
		}
	}

	return dpi;
}
