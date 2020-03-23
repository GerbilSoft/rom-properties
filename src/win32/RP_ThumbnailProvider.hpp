/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
 *                                                                         *
 * Copyright (c) 2016-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__
#define __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__

// librpbase
#include "librpbase/config.librpbase.h"
#include "common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "libwin32common/ComBase.hpp"

// IThumbnailProvider
#include "thumbcache-wrapper.hpp"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ThumbnailProvider;
}

namespace LibWin32Common {
	class RegKey;
}

// C++ includes.
#include <string>

class RP_ThumbnailProvider_Private;

class UUID_ATTR("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}")
RP_ThumbnailProvider : public LibWin32Common::ComBase2<IInitializeWithStream, IThumbnailProvider>
{
	public:
		RP_ThumbnailProvider();
	protected:
		virtual ~RP_ThumbnailProvider();

	private:
		typedef LibWin32Common::ComBase2<IInitializeWithStream, IThumbnailProvider> super;
		RP_DISABLE_COPY(RP_ThumbnailProvider)
	private:
		friend class RP_ThumbnailProvider_Private;
		RP_ThumbnailProvider_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) final;

	public:
		/**
		 * Register the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterCLSID(void);

		/**
		 * Register the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG RegisterFileType(LibWin32Common::RegKey &hkcr, LPCTSTR ext);

		/**
		 * Unregister the COM object.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterCLSID(void);

		/**
		 * Unregister the file type handler.
		 * @param hkcr HKEY_CLASSES_ROOT or user-specific classes root.
		 * @param ext File extension, including the leading dot.
		 * @return ERROR_SUCCESS on success; Win32 error code on error.
		 */
		static LONG UnregisterFileType(LibWin32Common::RegKey &hkcr, LPCTSTR ext);

	public:
		// IInitializeWithStream
		IFACEMETHODIMP Initialize(IStream *pstream, DWORD grfMode) final;

		// IThumbnailProvider
		IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) final;
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ThumbnailProvider, __MSABI_LONG(0x4723df58), 0x463e, 0x4590, 0x8f,0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__ */
