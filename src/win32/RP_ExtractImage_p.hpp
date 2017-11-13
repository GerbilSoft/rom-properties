/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage_p.hpp: IExtractImage implementation. (PRIVATE CLASS)    *
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

#ifndef __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__

#include "RP_ExtractImage.hpp"
#include "CreateThumbnail.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ExtractImagePrivate RP_ExtractImage_Private

// CLSID
extern const CLSID CLSID_RP_ExtractImage;

class RP_ExtractImage_Private
{
	public:
		RP_ExtractImage_Private();
		virtual ~RP_ExtractImage_Private();

	private:
		RP_DISABLE_COPY(RP_ExtractImage_Private)

	public:
		// ROM filename from IPersistFile::Load().
		std::string filename;

		// RomData object. Loaded in IPersistFile::Load().
		LibRpBase::RomData *romData;

		// Data from IExtractImage::GetLocation().
		SIZE rgSize;
		DWORD dwRecClrDepth;
		DWORD dwFlags;

		// CreateThumbnail instance.
		CreateThumbnailNoAlpha thumbnailer;

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
		 * Fallback image handler function. (internal)
		 * @param hkey_Assoc File association key to check.
		 * @param phBmpImage
		 * @return HRESULT.
		 */
		HRESULT Fallback_int(LibWin32Common::RegKey &hkey_Assoc, HBITMAP *phBmpImage);

	public:
		/**
		 * Fallback image handler function.
		 * @param phBmpImage
		 * @return HRESULT.
		 */
		HRESULT Fallback(HBITMAP *phBmpImage);
};

#endif /* __ROMPROPERTIES_WIN32_RP_EXTRACTIMAGE_P_HPP__ */
