/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.cpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "ComBase.hpp"

// C includes. (C++ namespace)
#include <cassert>

namespace LibWin32Common {

// References of all objects.
volatile ULONG RP_ulTotalRefCount = 0;

/** Dynamically loaded common functions **/
volatile int pfn_init_status = 0;	// sort-of pthread_once() implementation

// QISearch()
static HMODULE hShlwapi_dll = nullptr;
PFNQISEARCH pfnQISearch = nullptr;

// IsThemeActive()
static HMODULE hUxTheme_dll = nullptr;
PFNISTHEMEACTIVE pfnIsThemeActive = nullptr;

void incRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedIncrement(&RP_ulTotalRefCount);
	if (ulRefCount != 1) {
		// Not the first initialization.
		// Check if the functions have been loaded yet.
		while (pfn_init_status != 1) {
			SwitchToThread();
		}
		return;
	}

	// First initialization.
	// Make sure pfn_init_status is 0, just in case
	// something tries initializing a component while
	// the last component was being uninitialized.
	while (pfn_init_status != 0) {
		SwitchToThread();
	}
	pfn_init_status = 2;	// NOTE: Not atomic, but that shouldn't be needed.

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

	// Initialized.
	pfn_init_status = 1;
}

void decRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedDecrement(&RP_ulTotalRefCount);
	if (ulRefCount != 0)
		return;

	// Last Release(). Unload the function pointers.
	// NOTE: pfn_init_status should not be 0 here.
	assert(pfn_init_status != 0);
	while (pfn_init_status != 1) {
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

	// Finished unloading function pointers.
	pfn_init_status = 0;
}

}
