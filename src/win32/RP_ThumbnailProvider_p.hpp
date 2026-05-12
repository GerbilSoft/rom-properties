/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider_p.hpp: IThumbnailProvider implementation.          *
 * (PRIVATE CLASS)                                                         *
 *                                                                         *
 * Copyright (c) 2016-2026 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "RP_ThumbnailProvider.hpp"
#include "CreateThumbnail.hpp"

// FIXME: MinGW-w64 14.0.0's comdefsp.h has this, but it isn't working...
#ifndef _MSC_VER
#  ifndef RP_IStreamPtr_DEFINED
_COM_SMARTPTR_TYPEDEF(IStream, __uuidof(IStream));
#    define RP_IStreamPtr_DEFINED 1
#  endif /* RP_IStreamPtr_DEFINED */
#endif /* _MSC_VER */

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ThumbnailProviderPrivate RP_ThumbnailProvider_Private

namespace LibRpFile {
	class IRpFile;
}

class RP_ThumbnailProvider_Private
{
public:
	RP_ThumbnailProvider_Private();

private:
	RP_DISABLE_COPY(RP_ThumbnailProvider_Private)

public:
	// Set by IInitializeWithStream::Initialize().
	LibRpFile::IRpFilePtr file;

	// CreateThumbnail instance.
	CreateThumbnail thumbnailer;

	// IStream* used by the IRpFile.
	// NOTE: RpFile_IStream does NOT take ownership of the IStream*.
	IStreamPtr pstream;
	DWORD grfMode;

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
	 * Fallback thumbnail handler function. (internal)
	 * @param hkey_Assoc File association key to check.
	 * @param cx
	 * @param phbmp
	 * @param pdwAlpha
	 * @return HRESULT.
	 */
	HRESULT Fallback_int(LibWin32UI::RegKey &hkey_Assoc,
		UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

public:
	/**
	 * Fallback thumbnail handler function.
	 * @param cx
	 * @param phbmp
	 * @param pdwAlpha
	 * @return HRESULT.
	 */
	HRESULT Fallback(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);
};
