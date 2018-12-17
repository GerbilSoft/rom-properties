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

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__

#include "RpWin32_sdk.h"
#include <propvarutil.h>

// Use our versions instead of the standard system versions,
// which aren't present on XP.
#define InitPropVariantFromFileTime InitPropVariantFromFileTime_xp
#define InitPropVariantFromStringVector InitPropVariantFromStringVector_xp

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a PROPVARIANT from a FILETIME.
 * @param pftIn		[in] FILEITME
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
inline HRESULT InitPropVariantFromFileTime_xp(_In_ const FILETIME *pftIn, _Out_ PROPVARIANT *pPropVar)
{
	// TODO: FILETIME doesn't have a 64-bit accessor.
	pPropVar->vt = VT_FILETIME;
	pPropVar->uhVal.LowPart = pftIn->dwLowDateTime;
	pPropVar->uhVal.HighPart = pftIn->dwHighDateTime;
	return S_OK;
}

/**
 * Initialize a PROPVARIANT from a string vector.
 * @param prgsz		[in] String vector.
 * @param cElems	[in] Number of strings in the string vector.
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
HRESULT InitPropVariantFromStringVector_xp(_In_ PCWSTR *prgsz, ULONG cElems, PROPVARIANT *pPropVar);

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__ */
