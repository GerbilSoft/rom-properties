/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * propsys_xp.c: Implementation of PropSys functions not available in XP.  *
 *                                                                         *
 * Copyright (c) 2018-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__
#define __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__

#include "RpWin32_sdk.h"
#include <propvarutil.h>
#include "dll-macros.h"	// for RP_LIBROMDATA_PUBLIC

// C includes
#include <string.h>

// Use our versions instead of the standard system versions,
// which aren't present on XP.
#define InitPropVariantFromFileTime InitPropVariantFromFileTime_xp
#define InitPropVariantFromStringVector InitPropVariantFromStringVector_xp

// Functions available on Windows XP, but defined in shlwapi.dll,
// and we're not linking to shlwapi.dll.
#define InitPropVariantFromString InitPropVariantFromString_noShlwapi

// Functions not available on Windows 7.
// This function is part of sensorsutils.h.
#define InitPropVariantFromFloat InitPropVariantFromFloat_noSensorsUtils

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
	pPropVar->vt = VT_FILETIME;
	pPropVar->filetime = *pftIn;
	return S_OK;
}

/**
 * Initialize a PROPVARIANT from a string vector.
 * @param prgsz		[in] String vector.
 * @param cElems	[in] Number of strings in the string vector.
 * @param pPropVar	[out] PROPVARIANT
 * @return HRESULT
 */
RP_LIBROMDATA_PUBLIC
HRESULT WINAPI InitPropVariantFromStringVector_xp(_In_ PCWSTR *prgsz, ULONG cElems, PROPVARIANT *pPropVar);

/**
 * Initialize a PROPVARIANT from a string.
 * @param psz		[in] String.
 * @param ppropvar	[out] PROPVARIANT
 */
RP_LIBROMDATA_PUBLIC
HRESULT WINAPI InitPropVariantFromString_noShlwapi(_In_ PCWSTR psz, _Out_ PROPVARIANT *ppropvar);

/**
 * Initialize a PROPVARIANT from a float.
 * @param fvalueIn     [in] float
 * @param pPropVar     [out] PROPVARIANT
 * @return HRESULT
 */
static inline HRESULT InitPropVariantFromFloat_noSensorsUtils(_In_ float fvalueIn, _Out_ PROPVARIANT *pPropVar)
{
       pPropVar->vt = VT_R4;
       pPropVar->fltVal = fvalueIn;
       return S_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBWIN32COMMON_PROPSYS_XP_H__ */
