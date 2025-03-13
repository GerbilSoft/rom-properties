/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_p.hpp: IExtractIcon implementation. (PRIVATE CLASS)      *
 *                                                                         *
 * Copyright (c) 2016-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_ExtractIcon.hpp"
#include "CreateThumbnail.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ExtractIconPrivate RP_ExtractIcon_Private

// CLSID
extern const CLSID CLSID_RP_ExtractIcon;

class RP_ExtractIcon_Private
{
public:
	RP_ExtractIcon_Private();

private:
	RP_DISABLE_COPY(RP_ExtractIcon_Private)

public:
	// ROM filename from IPersistFile::Load()
	// NOTE: IPersistFile::Load() gets LPOLESTR, which is LPWSTR.
	std::wstring olefilename;

	// RomData object. Loaded in IPersistFile::Load().
	LibRpBase::RomDataPtr romData;

	// CreateThumbnail instance.
	CreateThumbnail thumbnailer;

public:
	/**
	 * Register the file type handler.
	 *
	 * Internal version; this only registers for a single Classes key.
	 * Called by the public version multiple times if a ProgID is registered.
	 *
	 * @param hkey_Assoc File association key to register under.
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	static LONG RegisterFileType(LibWin32UI::RegKey &hkey_Assoc);

	/**
	 * Unregister the file type handler.
	 *
	 * Internal version; this only unregisters for a single Classes key.
	 * Called by the public version multiple times if a ProgID is registered.
	 *
	 * @param hkey_Assoc File association key to unregister under.
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	static LONG UnregisterFileType(LibWin32UI::RegKey &hkey_Assoc);

private:
	/**
	 * Use IExtractIconW from a fallback icon handler.
	 * @param pExtractIconW	[in] Pointer to IExtractIconW interface
	 * @param phiconLarge	[out,opt] Large icon
	 * @param phiconSmall	[out,opt] Small icon
	 * @param nIconSize	[in] Icon size
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	LONG DoExtractIconW(_In_ IExtractIconW *pExtractIconW,
		_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall, UINT nIconSize);

	/**
	 * Use IExtractIconA from an old fallback icon handler.
	 * @param pExtractIconA	[in] Pointer to IExtractIconW interface
	 * @param phiconLarge	[out,opt] Large icon
	 * @param phiconSmall	[out,opt] Small icon
	 * @param nIconSize	[in] Icon size
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	LONG DoExtractIconA(_In_ IExtractIconA *pExtractIconA,
		_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall, UINT nIconSize);

	/**
	 * Fallback icon handler function. (internal)
	 * This function reads the RP_Fallback key for fallback data.
	 * @param hkey_Assoc	[in] File association key to check
	 * @param phiconLarge	[out,opt] Large icon
	 * @param phiconSmall	[out,opt] Small icon
	 * @param nIconSize	[in] Icon sizes (LOWORD == large icon size; HIWORD == small icon size)
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	LONG Fallback_int(LibWin32UI::RegKey &hkey_Assoc,
		_Outptr_opt_ HICON *phiconLarge, _Outptr_opt_ HICON *phiconSmall, UINT nIconSize);

public:
	/**
	 * Fallback icon handler function.
	 * @param phiconLarge Large icon
	 * @param phiconSmall Small icon
	 * @param nIconSize Icon sizes (LOWORD == large icon size; HIWORD == small icon size)
	 * @return ERROR_SUCCESS on success; Win32 error code on error.
	 */
	LONG Fallback(HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
};
