/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider.hpp: IThumbnailProvider implementation.            *
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

#ifndef __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__
#define __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__

#include "libromdata/config.libromdata.h"
#include "libromdata/common.h"

// Reference: http://www.codeproject.com/Articles/338268/COM-in-C
#include "RP_ComBase.hpp"

// IThumbnailProvider
#include "thumbcache.h"

// CLSID
extern "C" {
	extern const CLSID CLSID_RP_ThumbnailProvider;
}

namespace LibRomData {
	class IRpFile;
	class rp_image;
}

// C++ includes.
#include <string>

class RegKey;
class RP_ThumbnailProvider_Private;

class UUID_ATTR("{4723DF58-463E-4590-8F4A-8D9DD4F4355A}")
RP_ThumbnailProvider : public RP_ComBase2<IInitializeWithStream, IThumbnailProvider>
{
	public:
		RP_ThumbnailProvider();
		virtual ~RP_ThumbnailProvider();

	private:
		typedef RP_ComBase2<IInitializeWithStream, IThumbnailProvider> super;
		RP_DISABLE_COPY(RP_ThumbnailProvider)
	private:
		friend class RP_ThumbnailProvider_Private;
		RP_ThumbnailProvider_Private *const d_ptr;

	public:
		// IUnknown
		IFACEMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj) override final;

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
		static LONG RegisterFileType(RegKey &hkcr, LPCWSTR ext);

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
		static LONG UnregisterFileType(RegKey &hkcr, LPCWSTR ext);

	public:
		// IInitializeWithStream
		IFACEMETHODIMP Initialize(IStream *pstream, DWORD grfMode) override final;

		// IThumbnailProvider
		IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha) override final;

	protected:
		// Thumbnail resize policy.
		// TODO: Make this configurable.
		enum ResizePolicy {
			RESIZE_NONE,	// No resizing.

			// Only resize images that are less than or equal to half the
			// requested thumbnail size. This is a compromise to allow
			// small icons like Nintendo DS icons to be enlarged while
			// larger but not-quite 256px images like GameTDB disc scans'
			// (160px) will remain as-is.
			RESIZE_HALF,

			// Resize all images that are smaller than the requested
			// thumbnail size.
			RESIZE_ALL,
		};
};

#ifdef __CRT_UUID_DECL
// Required for MinGW-w64 __uuidof() emulation.
__CRT_UUID_DECL(RP_ThumbnailProvider, 0x4723df58, 0x463e, 0x4590, 0x8f, 0x4a, 0x8d, 0x9d, 0xd4, 0xf4, 0x35, 0x5a)
#endif

#endif /* __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_HPP__ */
