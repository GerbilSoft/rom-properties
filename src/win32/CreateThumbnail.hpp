/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * CreateThumbnail.hpp: TCreateThumbnail<HBITMAP> implementation.          *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "libromdata/img/TCreateThumbnail.hpp"
#include "libwin32common/RpWin32_sdk.h"

/**
 * CreateThumbnail implementation for Windows.
 * This version uses alpha transparency.
 */
class CreateThumbnail : public LibRomData::TCreateThumbnail<HBITMAP>
{
	public:
		explicit CreateThumbnail() = default;

	private:
		typedef TCreateThumbnail<HBITMAP> super;
		RP_DISABLE_COPY(CreateThumbnail)

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		HBITMAP rpImageToImgClass(const LibRpTexture::rp_image *img) const override;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		inline bool isImgClassValid(const HBITMAP &imgClass) const final
		{
			return (imgClass != nullptr);
		}

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		inline HBITMAP getNullImgClass(void) const final
		{
			return nullptr;
		}

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		inline void freeImgClass(HBITMAP &imgClass) const final
		{
			DeleteBitmap(imgClass);
		}

		/**
		 * Rescale an ImgClass using the specified scaling method.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @param method Scaling method.
		 * @return Rescaled ImgClass.
		 */
		HBITMAP rescaleImgClass(const HBITMAP &imgClass, ImgSize sz, ScalingMethod method = ScalingMethod::Nearest) const override;

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		int getImgClassSize(const HBITMAP &imgClass, ImgSize *pOutSize) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @param url URL
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		inline std::string proxyForUrl(const char *url) const final
		{
			// rp-download uses WinInet on Windows, which
			// always uses the system proxy.
			((void)url);
			return std::string();
		}

		/**
		 * Is the system using a metered connection?
		 *
		 * Note that if the system doesn't support identifying if the
		 * connection is metered, it will be assumed that the network
		 * connection is unmetered.
		 *
		 * @return True if metered; false if not.
		 */
		bool isMetered(void) final;
};

/**
 * CreateThumbnail implementation for Windows.
 * This version does NOT use alpha transparency.
 * COLOR_WINDOW is used for the background.
 */
class CreateThumbnailNoAlpha final : public CreateThumbnail
{
	public:
		explicit CreateThumbnailNoAlpha() = default;

	private:
		typedef CreateThumbnail super;
		RP_DISABLE_COPY(CreateThumbnailNoAlpha)

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		HBITMAP rpImageToImgClass(const LibRpTexture::rp_image *img) const final;

		/**
		 * Rescale an ImgClass using the specified scaling method.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @param method Scaling method.
		 * @return Rescaled ImgClass.
		 */
		HBITMAP rescaleImgClass(const HBITMAP &imgClass, ImgSize sz, ScalingMethod method = ScalingMethod::Nearest) const final;
};
