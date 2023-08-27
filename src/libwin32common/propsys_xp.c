/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * propsys_xp.c: Implementation of PropSys functions not available in XP.  *
 *                                                                         *
 * Copyright (c) 2018-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "propsys_xp.h"

// C includes
#include <assert.h>

// propsys_xp.c isn't used by libromdata directly,
// so use some linker hax to force linkage.
extern unsigned char RP_LibWin32Common_propsys_xp_ForceLinkage;
unsigned char RP_LibWin32Common_propsys_xp_ForceLinkage;

/**
 * Initialize a PROPVARIANT from a string vector.
 * @param prgsz		[in] String vector.
 * @param cElems	[in] Number of strings in the string vector.
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
HRESULT WINAPI InitPropVariantFromStringVector_xp(_In_ PCWSTR *prgsz, ULONG cElems, PROPVARIANT *pPropVar)
{
	ULONG i;

	pPropVar->vt = VT_VECTOR | VT_BSTR;
	if (cElems == 0) {
		// No elements.
		pPropVar->cabstr.cElems = cElems;
		pPropVar->cabstr.pElems = NULL;
		return S_OK;
	}

	// Allocate memory for the vector.
	pPropVar->cabstr.pElems = CoTaskMemAlloc(cElems * sizeof(PWCHAR));
	if (!pPropVar->cabstr.pElems) {
		// Unable to allocate memory.
		pPropVar->cabstr.cElems = 0;
		return E_OUTOFMEMORY;
	}

	// Copy the strings.
	for (i = 0; i < cElems; i++) {
		BSTR bstr = SysAllocString(prgsz[i]);
		if (!bstr && prgsz[i]) {
			// Error copying the string.
			// Free all the other strings and cancel.
			ULONG j;
			for (j = 0; j < i; j++) {
				SysFreeString(pPropVar->cabstr.pElems[j]);
			}
			CoTaskMemFree(pPropVar->cabstr.pElems);
			pPropVar->cabstr.pElems = NULL;
			pPropVar->cabstr.cElems = 0;
			return E_OUTOFMEMORY;
		}

		pPropVar->cabstr.pElems[i] = bstr;
	}

	// Strings copied.
	pPropVar->cabstr.cElems = cElems;
	return S_OK;
}

/**
 * Initialize a PROPVARIANT from a string.
 * @param psz		[in] String.
 * @param ppropvar	[out] PROPVARIANT
 */
HRESULT WINAPI InitPropVariantFromString_noShlwapi(_In_ PCWSTR psz, _Out_ PROPVARIANT *ppropvar)
{
	// The standard InitPropVariantFromString() function, and the
	// wine implementation, uses SHStrDupW(), which requires linking
	// to shlwapi.dll. We'll use MSVCRT functions instead.
	// Reference: https://github.com/wine-mirror/wine/blob/1bb953c6766c9cc4372ca23a7c5b7de101324218/include/propvarutil.h#L107
	size_t byteCount;

	assert(psz != NULL);
	assert(ppropvar != NULL);
	if (!psz) {
		return E_INVALIDARG;
	} else if (!ppropvar) {
		return E_POINTER;
	}

	byteCount = (wcslen(psz) + 1) * sizeof(wchar_t);
	ppropvar->pwszVal = (PWSTR)CoTaskMemAlloc(byteCount);
	if (!ppropvar->pwszVal) {
		PropVariantInit(ppropvar);
		return E_OUTOFMEMORY;
	}

	memcpy(ppropvar->pwszVal, psz, byteCount);
	ppropvar->vt = VT_LPWSTR;
	return S_OK;
}
