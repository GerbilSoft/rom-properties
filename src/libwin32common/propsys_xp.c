/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * propsys_xp.c: Implementation of PropSys functions not available in XP.  *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "propsys_xp.h"

/**
 * Initialize a PROPVARIANT from a string vector.
 * @param prgsz		[in] String vector.
 * @param cElems	[in] Number of strings in the string vector.
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
HRESULT InitPropVariantFromStringVector_xp(_In_ PCWSTR *prgsz, ULONG cElems, PROPVARIANT *pPropVar)
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
