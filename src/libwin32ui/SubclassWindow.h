/***************************************************************************
 * ROM Properties Page shell extension. (libwin32ui)                       *
 * SubclassWindow.h: Wrapper functions for COMCTL32 subclassing.           *
 *                                                                         *
 * Copyright (c) 2019-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "config.libwin32common.h"

// On Windows XP and later, the COMCTL32 subclassing functions
// can be accessed directly, so nothing needs to be done here.
// Reference: http://vb.mvps.org/samples/HookXP/

// NOTE: COMCTL32 must be loaded before using these functions!

#ifdef ENABLE_OLDWINCOMPAT

#include "./RpWin32_sdk.h"
#include <commctrl.h>

// Undefine isolation-aware subclass macros.
// This breaks isolation awareness, but Windows 2000 doesn't have that anyway.
#ifdef SetWindowSubclass
#  undef SetWindowSubclass
#endif
#ifdef GetWindowSubclass
#  undef GetWindowSubclass
#endif
#ifdef RemoveWindowSubclass
#  undef RemoveWindowSubclass
#endif
#ifdef DefSubclassProc
#  undef DefSubclassProc
#endif

#define SetWindowSubclass(hWnd, pfnSubclass, uIdsubclass, dwRefData) SetWindowSubclass_compat((hWnd), (pfnSubclass), (uIdsubclass), (dwRefData))
#define GetWindowSubclass(hWnd, pfnSubclass, uIdSubclass, pdwRefData) GetWindowSubclass_compat((hWnd), (pfnSubclass), (uIdSubclass), (pdwRefData))
#define RemoveWindowSubclass(hWnd, pfnSubclass, uIdsubclass) RemoveWindowSubclass_compat((hWnd), (pfnSubclass), (uIdsubclass))
#define DefSubclassProc(hWnd, uMsg, lParam, wParam) DefSubclassProc_compat((hWnd), (uMsg), (lParam), (wParam))

// TODO: Initialize function pointers once instead of on every call?

typedef BOOL (WINAPI *PFNSETWINDOWSUBCLASS)(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass,
	_In_ DWORD_PTR dwRefData);

static inline BOOL SetWindowSubclass_compat(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass,
	_In_ DWORD_PTR dwRefData)
{
	PFNSETWINDOWSUBCLASS pfnSetWindowSubclass;

	HMODULE hComCtl32_dll = GetModuleHandle(_T("comctl32.dll"));
	if (!hComCtl32_dll) {
		return FALSE;
	}

	pfnSetWindowSubclass = (PFNSETWINDOWSUBCLASS)GetProcAddress(hComCtl32_dll, MAKEINTRESOURCEA(410));
	if (!pfnSetWindowSubclass) {
		return FALSE;
	}

	return pfnSetWindowSubclass(hWnd, pfnSubclass, uIdSubclass, dwRefData);
}

typedef BOOL (WINAPI *PFNGETWINDOWSUBCLASS)(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass,
	_In_ DWORD_PTR *pdwRefData);

static inline BOOL GetWindowSubclass_compat(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass,
	_Out_opt_ DWORD_PTR *pdwRefData)
{
	PFNGETWINDOWSUBCLASS pfnGetWindowSubclass;

	HMODULE hComCtl32_dll = GetModuleHandle(_T("comctl32.dll"));
	if (!hComCtl32_dll) {
		return FALSE;
	}

	pfnGetWindowSubclass = (PFNGETWINDOWSUBCLASS)GetProcAddress(hComCtl32_dll, MAKEINTRESOURCEA(411));
	if (!pfnGetWindowSubclass) {
		return FALSE;
	}

	return pfnGetWindowSubclass(hWnd, pfnSubclass, uIdSubclass, pdwRefData);
}

typedef BOOL (WINAPI *PFNREMOVEWINDOWSUBCLASS)(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass);

static inline BOOL RemoveWindowSubclass_compat(
	_In_ HWND hWnd,
	_In_ SUBCLASSPROC pfnSubclass,
	_In_ UINT_PTR uIdSubclass)
{
	PFNREMOVEWINDOWSUBCLASS pfnRemoveWindowSubclass;

	HMODULE hComCtl32_dll = GetModuleHandle(_T("comctl32.dll"));
	if (!hComCtl32_dll) {
		return FALSE;
	}

	pfnRemoveWindowSubclass = (PFNREMOVEWINDOWSUBCLASS)GetProcAddress(hComCtl32_dll, MAKEINTRESOURCEA(412));
	if (!pfnRemoveWindowSubclass) {
		return FALSE;
	}

	return pfnRemoveWindowSubclass(hWnd, pfnSubclass, uIdSubclass);
}

typedef LRESULT (WINAPI *PFNDEFSUBCLASSPROC)(
	_In_ HWND hWnd,
	_In_ UINT uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam);

static inline LRESULT DefSubclassProc_compat(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PFNDEFSUBCLASSPROC pfnDefSubclassProc;

	HMODULE hComCtl32_dll = GetModuleHandle(_T("comctl32.dll"));
	if (!hComCtl32_dll) {
		return FALSE;
	}

	pfnDefSubclassProc = (PFNDEFSUBCLASSPROC)GetProcAddress(hComCtl32_dll, MAKEINTRESOURCEA(413));
	if (!pfnDefSubclassProc) {
		return FALSE;
	}

	return pfnDefSubclassProc(hWnd, uMsg, wParam, lParam);
}

#endif /* ENABLE_OLDWINCOMPAT */
