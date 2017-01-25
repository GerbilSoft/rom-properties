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

class RP_ThumbnailProvider_Private : public TCreateThumbnail<HBITMAP>
{
	public:
		RP_ThumbnailProvider_Private();
		virtual ~RP_ThumbnailProvider_Private();

	private:
		typedef TCreateThumbnail<HBITMAP> super;
		RP_DISABLE_COPY(RP_ThumbnailProvider_Private)

	public:
		// IRpFile IInitializeWithStream::Initialize().
		IRpFile *file;

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual HBITMAP rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const HBITMAP &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual HBITMAP getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. HBITMAP.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(HBITMAP &imgClass) const final;

		/**
		 * Get an ImgClass's size.
		 * @param imgClass ImgClass object.
		 * @retrun Size.
		 */
		virtual ImgSize getImgSize(const HBITMAP &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual HBITMAP rescaleImgClass(const HBITMAP &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const final;
};

#endif /* __ROMPROPERTIES_WIN32_RP_THUMBNAILPROVIDER_P_HPP__ */
