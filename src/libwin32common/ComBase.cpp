/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.cpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "ComBase.hpp"
#include "HiDPI.h"

// C includes. (C++ namespace)
#include <cassert>

// librpthreads
#include "librpthreads/pthread_once.h"

namespace LibWin32Common {

// References of all objects.
volatile ULONG RP_ulTotalRefCount = 0;

/** Dynamically loaded common functions **/
static volatile pthread_once_t once_control = PTHREAD_ONCE_INIT;

// QISearch()
static HMODULE hShlwapi_dll = nullptr;
PFNQISEARCH pfnQISearch = nullptr;

// IsThemeActive()
static HMODULE hUxTheme_dll = nullptr;
PFNISTHEMEACTIVE pfnIsThemeActive = nullptr;

static void initFnPtrs(void)
{
	// SHLWAPI.DLL!QISearch
	hShlwapi_dll = LoadLibrary(_T("shlwapi.dll"));
	assert(hShlwapi_dll != nullptr);
	if (hShlwapi_dll) {
		pfnQISearch = reinterpret_cast<PFNQISEARCH>(GetProcAddress(hShlwapi_dll, MAKEINTRESOURCEA(219)));
		assert(pfnQISearch != nullptr);
		if (!pfnQISearch) {
			FreeLibrary(hShlwapi_dll);
			hShlwapi_dll = nullptr;
		}
	}

	// UXTHEME.DLL!IsThemeActive
	hUxTheme_dll = LoadLibrary(_T("uxtheme.dll"));
	assert(hUxTheme_dll != nullptr);
	if (hUxTheme_dll) {
		pfnIsThemeActive = reinterpret_cast<PFNISTHEMEACTIVE>(GetProcAddress(hUxTheme_dll, "IsThemeActive"));
		if (!pfnIsThemeActive) {
			FreeLibrary(hUxTheme_dll);
			hUxTheme_dll = nullptr;
		}
	}
}

void incRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedIncrement(&RP_ulTotalRefCount);

	// Make sure the function pointers are initialized.
	pthread_once((pthread_once_t*)&once_control, initFnPtrs);
}

void decRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedDecrement(&RP_ulTotalRefCount);
	if (ulRefCount != 0)
		return;

	// Last Release(). Unload the function pointers.
	// NOTE: once_control should not be PTHREAD_ONCE_INIT here.
	assert(once_control != PTHREAD_ONCE_INIT);
	// This is always correct for our pthread_once() implementation.
	while (once_control != 1) {
		SwitchToThread();
	}

	if (hShlwapi_dll) {
		pfnQISearch = nullptr;
		FreeLibrary(hShlwapi_dll);
		hShlwapi_dll = nullptr;
	}
	if (hUxTheme_dll) {
		pfnIsThemeActive = nullptr;
		FreeLibrary(hUxTheme_dll);
		hUxTheme_dll = nullptr;
	}

	// Unload modules needed for High-DPI, if necessary.
	rp_DpiUnloadModules();

	// Finished unloading function pointers.
	once_control = PTHREAD_ONCE_INIT;
}

}
