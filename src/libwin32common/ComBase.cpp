/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * ComBase.cpp: Base class for COM objects.                                *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
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

}
