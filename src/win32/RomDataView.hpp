/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RomDataView.hpp: RomData viewer control.                                *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libwin32common/RpWin32_sdk.h"
#include "stdboolx.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WC_ROMDATAVIEW		_T("rp-RomDataView")

void RomDataViewRegister(void);
void RomDataViewUnregister(void);

#define WM_ROMDATAVIEW_SETFILENAMEA	(WM_USER + 1)	// lParam == filenameA
#define WM_ROMDATAVIEW_SETFILENAMEW	(WM_USER + 2)	// lParam == filenameW
#define WM_ROMDATAVIEW_ANIMATION_CTRL	(WM_USER + 3)	// wParam == FALSE to stop, TRUE to start

static inline void RomDataView_SetFileNameA(HWND hWnd, LPCSTR filenameA)
{
	SendMessage(hWnd, WM_ROMDATAVIEW_SETFILENAMEA, 0, reinterpret_cast<LPARAM>(filenameA));
}

static inline void RomDataView_SetFileNameW(HWND hWnd, LPCWSTR filenameW)
{
	SendMessage(hWnd, WM_ROMDATAVIEW_SETFILENAMEW, 0, reinterpret_cast<LPARAM>(filenameW));
}

static inline void RomDataView_AnimationCtrl(HWND hWnd, bool start)
{
	SendMessage(hWnd, WM_ROMDATAVIEW_ANIMATION_CTRL, static_cast<WPARAM>(start), 0);
}

#ifdef UNICODE
#  define RomDataView_SetFileName(hWnd, filename) RomDataView_SetFileNameW((hWnd), (filename))
#else /* !UNICODE */
#  define RomDataView_SetFileName(hWnd, filename) RomDataView_SetFileNameA((hWnd), (filename))
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif
