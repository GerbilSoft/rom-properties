/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractIcon_p.hpp: IExtractIcon implementation. (PRIVATE CLASS)      *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTICON_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTICON_P_HPP__

#include "RP_ExtractIcon.hpp"

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
		LibRomData::rp_string filename;

		// RomData object. Loaded in IPersistFile::Load().
		LibRomData::RomData *romData;

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
		static LONG RegisterFileType(RegKey &hkey_Assoc);

		/**
		 * Unregister the file type handler.
		 *
		 * Internal version; this only unregisters for a single Classes key.
		 * Called by the public version multiple times if a ProgID is registered.
		 *
		 * @param hkey_Assoc File association key to unregister under.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(RegKey &hkey_Assoc);

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
		 * Fallback icon handler function. (internal)
		 * @param hkey_Assoc File association key to check.
		 * @param phiconLarge Large icon.
		 * @param phiconSmall Small icon.
		 * @param nIconSize Icon sizes.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		LONG Fallback_int(RegKey &hkey_Assoc,
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
