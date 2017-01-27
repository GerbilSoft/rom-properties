// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "stdafx.h"
#include "RP_ComBase.hpp"

// C includes. (C++ namespace)
#include <cassert>

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
	hShlwapi = LoadLibrary(L"shlwapi.dll");
	assert(hShlwapi != nullptr);
	if (hShlwapi) {
		pQISearch = (PFNQISEARCH)GetProcAddress(hShlwapi, MAKEINTRESOURCEA(219));
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
