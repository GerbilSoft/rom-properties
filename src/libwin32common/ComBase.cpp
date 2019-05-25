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

// QISearch() function.
static HMODULE hShlwapi = nullptr;
PFNQISEARCH pQISearch = nullptr;

void incRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedIncrement(&RP_ulTotalRefCount);
	if (ulRefCount != 1)
		return;

	// First initialization. Load QISearch().
	hShlwapi = LoadLibrary(_T("shlwapi.dll"));
	assert(hShlwapi != nullptr);
	if (hShlwapi) {
		pQISearch = reinterpret_cast<PFNQISEARCH>(GetProcAddress(hShlwapi, MAKEINTRESOURCEA(219)));
		assert(pQISearch != nullptr);
		if (!pQISearch) {
			FreeLibrary(hShlwapi);
			hShlwapi = nullptr;
		}
	}
}

void decRpGlobalRefCount(void)
{
	ULONG ulRefCount = InterlockedDecrement(&RP_ulTotalRefCount);
	if (ulRefCount != 0)
		return;

	// Last Release(). Unload QISearch().
	pQISearch = nullptr;
	FreeLibrary(hShlwapi);
	hShlwapi = nullptr;
}

}
