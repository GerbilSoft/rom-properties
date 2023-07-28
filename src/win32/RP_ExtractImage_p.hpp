/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ExtractImage_p.hpp: IExtractImage implementation. (PRIVATE CLASS)    *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

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
		~RP_ExtractImage_Private();

	private:
		RP_DISABLE_COPY(RP_ExtractImage_Private)

	public:
		// ROM filename from IPersistFile::Load().
		// NOTE: LPOLESTR, not LPTSTR.
		LPOLESTR olefilename;

		// RomData object. Loaded in IPersistFile::Load().
		LibRpBase::RomData *romData;

		// CreateThumbnail instance.
		CreateThumbnailNoAlpha thumbnailer;

		// Data from IExtractImage::GetLocation().
		SIZE rgSize;
		DWORD dwRecClrDepth;
		DWORD dwFlags;

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
		 * Fallback image handler function. (internal)
		 * @param hkey_Assoc File association key to check.
		 * @param phBmpImage
		 * @return HRESULT.
		 */
		HRESULT Fallback_int(LibWin32UI::RegKey &hkey_Assoc, HBITMAP *phBmpImage);

	public:
		/**
		 * Fallback image handler function.
		 * @param phBmpImage
		 * @return HRESULT.
		 */
		HRESULT Fallback(HBITMAP *phBmpImage);
};
