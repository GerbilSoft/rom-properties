/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_p.hpp: IExtractIcon implementation. (PRIVATE CLASS)      *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_P_HPP__

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
		~RP_ExtractIcon_Private();

	private:
		RP_DISABLE_COPY(RP_ExtractIcon_Private)

	public:
		// ROM filename from IPersistFile::Load().
		std::u8string filename;

		// RomData object. Loaded in IPersistFile::Load().
		LibRpBase::RomData *romData;

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
		static LONG RegisterFileType(LibWin32Common::RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32Common::RegKey &hkey_Assoc);

	private:
		/**
		 * Use IExtractIconW from a fallback icon handler.
		 * @param pExtractIconW Pointer to IExtractIconW interface.
		 * @param phiconLarge Large icon.
		 * @param phiconSmall Small icon.
		 * @param nIconSize Icon sizes.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG DoExtractIconW(IExtractIconW *pExtractIconW,
			HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

		/**
		 * Use IExtractIconA from an old fallback icon handler.
		 * @param pExtractIconA Pointer to IExtractIconW interface.
		 * @param phiconLarge Large icon.
		 * @param phiconSmall Small icon.
		 * @param nIconSize Icon sizes.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG DoExtractIconA(IExtractIconA *pExtractIconA,
			HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

		/**
		 * Get the icon index from an icon resource specification,
		 * e.g. "C:\\Windows\\Some.DLL,1" .
		 * @param szIconSpec Icon resource specification
		 * @return Icon index, or 0 (default) if unknown.
		 */
		static int getIconIndexFromSpec(LPCTSTR szIconSpec);

		/**
		 * Fallback icon handler function. (internal)
		 * This function reads the RP_Fallback key for fallback data.
		 * @param hkey_Assoc File association key to check.
		 * @param phiconLarge Large icon.
		 * @param phiconSmall Small icon.
		 * @param nIconSize Icon sizes.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG Fallback_int(LibWin32Common::RegKey &hkey_Assoc,
			HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

	public:
		/**
		 * Fallback icon handler function.
		 * @param phiconLarge Large icon.
		 * @param phiconSmall Small icon.
		 * @param nIconSize Icon sizes.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG Fallback(HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
};

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTICON_P_HPP__ */
