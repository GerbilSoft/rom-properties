/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * HiDPI.h: High DPI wrapper functions.                                    *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the DPI for the specified window.
 * @param hWnd Window handle.
 * @return DPI, or 0 on error.
 */
UINT rp_GetDpiForWindow(HWND hWnd);

/**
 * Adjust any size for DPI.
 * @param px Size, in pixels
 * @param dpi DPI (96dpi == 1x)
 * @return Adjusted size
 */
static inline int rp_AdjustSizeForDpi(int px, UINT dpi)
{
	if (dpi <= 96) {
		// 96dpi, or invalid.
		// Return the original size.
		return px;
	}
	return (px * dpi) / 96;
}

/**
 * Adjust any size for the specified window's DPI.
 * @param px Size, in pixels
 * @param hWnd Window
 * @return Adjusted size
 */
static inline int rp_AdjustSizeForWindow(HWND hWnd, int px)
{
	return rp_AdjustSizeForDpi(px, rp_GetDpiForWindow(hWnd));
}

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
	if (sm != 0 && dpi > 96) {
		// Scale using 96dpi as the base value.
		sm = (sm * dpi) / 96;
	}
	return sm;
}

#ifdef __cplusplus
}
#endif
