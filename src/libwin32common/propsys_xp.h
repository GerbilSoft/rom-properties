/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * propsys_xp.c: Implementation of PropSys functions not available in XP.  *
 *                                                                         *
 * Copyright (c) 2018 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__

#include "RpWin32_sdk.h"
#include <propvarutil.h>

// C includes.
#include <string.h>

// Use our versions instead of the standard system versions,
// which aren't present on XP.
#define InitPropVariantFromFileTime InitPropVariantFromFileTime_xp
#define InitPropVariantFromStringVector InitPropVariantFromStringVector_xp

// Functions available on Windows XP, but defined in shlwapi.dll,
// and we're not linking to shlwapi.dll.
#define InitPropVariantFromString InitPropVariantFromString_noShlwapi

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a PROPVARIANT from a FILETIME.
 * @param pftIn		[in] FILEITME
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
static inline HRESULT InitPropVariantFromFileTime_xp(_In_ const FILETIME *pftIn, _Out_ PROPVARIANT *pPropVar)
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

/**
 * Initialize a PROPVARIANT from a string.
 * @param psz		[in] String.
 * @param ppropvar	[out] PROPVARIANT
 */
static inline HRESULT InitPropVariantFromString_noShlwapi(PCWSTR psz, PROPVARIANT *ppropvar)
{
	// The standard InitPropVariantFromString() function, and the
	// wine implementation, uses SHStrDupW(), which requires linking
	// to shlwapi.dll. We'll use MSVCRT functions instead.
	// Reference: https://github.com/wine-mirror/wine/blob/1bb953c6766c9cc4372ca23a7c5b7de101324218/include/propvarutil.h#L107

	const size_t len = wcslen(psz) + 1;
	ppropvar->pwszVal = (PWSTR)CoTaskMemAlloc(len * sizeof(wchar_t));
	if (!ppropvar->pwszVal) {
		PropVariantInit(ppropvar);
		return E_OUTOFMEMORY;
	}

	if (len > 0) {
		wcscpy_s(ppropvar->pwszVal, len, psz);
	}

	ppropvar->vt = VT_LPWSTR;
	return S_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__ */
