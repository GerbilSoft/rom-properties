/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * RP_ThumbnailProvider_p.hpp: IThumbnailProvider implementation.          *
 * (PRIVATE CLASS)                                                         *
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

#ifndef __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_P_HPP__
#define __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_P_HPP__

#include "RP_ThumbnailProvider.hpp"
#include "libromdata/img/TCreateThumbnail.hpp"

// Workaround for RP_D() expecting the no-underscore naming convention.
#define RP_ThumbnailProviderPrivate RP_ThumbnailProvider_Private

// CLSID
extern const CLSID CLSID_RP_ThumbnailProvider;

namespace LibRomData {
	class IRpFile;
}

class RP_ThumbnailProvider_Private : public LibRomData::TCreateThumbnail<HBITMAP>
{
	public:
		RP_ThumbnailProvider_Private();
		virtual ~RP_ThumbnailProvider_Private();

	private:
		typedef TCreateThumbnail<HBITMAP> super;
		RP_DISABLE_COPY(RP_ThumbnailProvider_Private)

	public:
		// Set by IInitializeWithStream::Initialize().
		LibRomData::IRpFile *file;
		// IStream* used by the IRpFile.
		// NOTE: Do NOT Release() this; RpFile_IStream handles it.
		IStream *pstream;
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
		 * Fallback thumbnail handler function. (internal)
		 * @param hkey_Assoc File association key to check.
		 * @param cx
		 * @param phbmp
		 * @param pdwAlpha
		 * @return HRESULT.
		 */
		HRESULT Fallback_int(RegKey &hkey_Assoc,
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

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual HBITMAP rpImageToImgClass(const LibRomData::rp_image *img) const override final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const HBITMAP &imgClass) const override final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual HBITMAP getNullImgClass(void) const override final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. HBITMAP.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(HBITMAP &imgClass) const override final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual HBITMAP rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const override final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual LibRomData::rp_string proxyForUrl(const LibRomData::rp_string &url) const override final;
};

#endif /* __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_P_HPP__ */
