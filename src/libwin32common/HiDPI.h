/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * HiDPI.h: High DPI wrapper functions.                                    *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_HIDPI_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_HIDPI_H__

#include "RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unload modules and reset the DPI configuration.
 * This should be done on DLL exit.
 */
void rp_DpiUnloadModules(void);

/**
 * Get the DPI for the specified window.
 * @param hWnd Window handle.
 * @return DPI, or 0 on error.
 */
UINT rp_GetDpiForWindow(HWND hWnd);

/**
 * GetSystemMetricsForDpi() implementation.
 * This function was first implemented in Windows 10 v1607,
 * but it's basically just GetSystemMetrics() with scaling.
 * @param nIndex GetSystemMetrics() index.
 * @param dpi DPI. (96dpi == 1x)
 * @return GetSystemMetrics() value, scaled to the specified DPI.
 */
static inline int rp_GetSystemMetricsForDpi(int nIndex, UINT dpi)
{
	int sm = GetSystemMetrics(nIndex);
	if (sm != 0 && dpi != 96) {
		// Scale using 96dpi as the base value.
		sm = (sm * dpi) / 96;
	}
	return sm;
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_HIDPI_H__ */
