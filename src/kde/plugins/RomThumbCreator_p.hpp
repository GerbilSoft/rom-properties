/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomThumbCreator_p.hpp: Thumbnail creator. (PRIVATE CLASS)               *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#include "ProxyForUrl.hpp"

// Qt includes
#include <QtGui/QImage>

// C++ STL classes
#include <string>

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"
using LibRomData::TCreateThumbnail;

class RomThumbCreatorPrivate final : public TCreateThumbnail<QImage>
{
public:
	RomThumbCreatorPrivate() = default;

private:
	typedef TCreateThumbnail<QImage> super;
	Q_DISABLE_COPY(RomThumbCreatorPrivate)

public:
	/** TCreateThumbnail functions. **/

	/**
	 * Wrapper function to convert rp_image* to ImgClass.
	 * @param img rp_image
	 * @return ImgClass
	 */
	inline QImage rpImageToImgClass(const rp_image_const_ptr &img) const final
	{
		return rpToQImage(img);
	}

	/**
	 * Wrapper function to check if an ImgClass is valid.
	 * @param imgClass ImgClass
	 * @return True if valid; false if not.
	 */
	inline bool isImgClassValid(const QImage &imgClass) const final
	{
		return !imgClass.isNull();
	}

	/**
	 * Wrapper function to get a "null" ImgClass.
	 * @return "Null" ImgClass.
	 */
	inline QImage getNullImgClass(void) const final
	{
		return {};
	}

	/**
	 * Free an ImgClass object.
	 * @param imgClass ImgClass object.
	 */
	inline void freeImgClass(QImage &imgClass) const final
	{
		// Nothing to do here...
		Q_UNUSED(imgClass)
	}

	/**
	 * Rescale an ImgClass using the specified scaling method.
	 * @param imgClass ImgClass object.
	 * @param sz New size.
	 * @param method Scaling method.
	 * @return Rescaled ImgClass.
	 */
	QImage rescaleImgClass(const QImage &imgClass, ImgSize sz, ScalingMethod method = ScalingMethod::Nearest) const final;

	/**
	 * Get the size of the specified ImgClass.
	 * @param imgClass	[in] ImgClass object.
	 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
	 * @return 0 on success; non-zero on error.
	 */
	int getImgClassSize(const QImage &imgClass, ImgSize *pOutSize) const final
	{
		pOutSize->width = imgClass.width();
		pOutSize->height = imgClass.height();
		return 0;
	}

	/**
	 * Get the proxy for the specified URL.
	 * @param url URL
	 * @return Proxy, or empty string if no proxy is needed.
	 */
	inline std::string proxyForUrl(const char *url) const final
	{
		return ::proxyForUrl(url);
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
