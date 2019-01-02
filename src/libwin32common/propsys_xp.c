/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * propsys_xp.c: Implementation of PropSys functions not available in XP.  *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
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
