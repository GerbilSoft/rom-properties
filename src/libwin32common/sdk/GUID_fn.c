/***************************************************************************
 * ROM Properties Page shell extension. (libwin32common)                   *
 * GUID.h: GUID function reimplementations for ANSI builds.                *
 *                                                                         *
 * Copyright (c) 2016-2019 by David Korth.                                 *
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

#ifndef UNICODE

// StringFromGUID2()
#include <objbase.h>

// Our functions
#include "GUID_fn.h"

/**
 * StringFromGUID2() wrapper function for ANSI builds.
 * @param rclsid	[in] CLSID
 * @param lpszClsidA	[out] Buffer for the CLSID string.
 * @param cchMax	[in] Length of lpszClsidA, in characters.
 */
#undef StringFromGUID2
int StringFromGUID2A(_In_ REFGUID rclsid, _Out_writes_(cchMax) LPSTR lpszClsidA, _In_ int cchMax)
{
	wchar_t szClsidW[48];	// maybe only 40 is needed?
	const wchar_t *pW;
	LONG lResult = StringFromGUID2(rclsid, szClsidW, sizeof(szClsidW)/sizeof(szClsidW[0]));
	if (lResult <= 0)
		return ERROR_INVALID_PARAMETER;

	// CLSID strings only have ASCII characters,
	// so we can convert it the easy way.
	for (pW = szClsidW; cchMax > 0; lpszClsidA++, pW++, cchMax--) {
		*lpszClsidA = (*pW & 0xFF);
		if (*pW == L'\0')
			break;
	}

	// Make sure the string is NULL-terminated.
	if (cchMax == 0) {
		*(lpszClsidA-1) = '\0';
	}

	return lResult;
}

#endif /* !UNICODE */
