/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
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

#include "RomThumbCreator.hpp"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// libcachemgr
#include "libcachemgr/CacheManager.hpp"
using LibCacheMgr::CacheManager;

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/img/rp_image.hpp"
using namespace LibRomData;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

// C includes.
#include <unistd.h>

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
using std::unique_ptr;

#include <QtCore/QUrl>
#include <QtGui/QImage>

// KDE protocol manager.
// Used to find the KDE proxy settings.
#include <kprotocolmanager.h>

/**
 * Factory method.
 * References:
 * - https://api.kde.org/4.x-api/kdelibs-apidocs/kio/html/classThumbCreator.html
 * - https://api.kde.org/frameworks/kio/html/classThumbCreator.html
 */
extern "C" {
	Q_DECL_EXPORT ThumbCreator *new_creator()
	{
		// Register RpQImageBackend.
		// TODO: Static initializer somewhere?
		rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

		return new RomThumbCreator();
	}
}

class RomThumbCreatorPrivate : public TCreateThumbnail<QImage>
{
	public:
		RomThumbCreatorPrivate() { }

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
		virtual QImage rpImageToImgClass(const rp_image *img) const override final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const QImage &imgClass) const override final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual QImage getNullImgClass(void) const override final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. QImage.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(QImage &imgClass) const override final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual QImage rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const override final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const override final;
};

/** RomThumbCreatorPrivate **/

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
QImage RomThumbCreatorPrivate::rpImageToImgClass(const rp_image *img) const
{
	return rpToQImage(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool RomThumbCreatorPrivate::isImgClassValid(const QImage &imgClass) const
{
	return !imgClass.isNull();
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
QImage RomThumbCreatorPrivate::getNullImgClass(void) const
{
	return QImage();
}

/**
 * Free an ImgClass object.
 * This may be no-op for e.g. QImage.
 * @param imgClass ImgClass object.
 */
void RomThumbCreatorPrivate::freeImgClass(QImage &imgClass) const
{
	// Nothing to do here...
	Q_UNUSED(imgClass)
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
QImage RomThumbCreatorPrivate::rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const
{
	return imgClass.scaled(sz.width, sz.height,
		Qt::KeepAspectRatio, Qt::FastTransformation);
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string RomThumbCreatorPrivate::proxyForUrl(const rp_string &url) const
{
	QString proxy = KProtocolManager::proxyForUrl(QUrl(RP2Q(url)));
	if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
		// No proxy.
		return rp_string();
	}

	// Proxy is required..
	return Q2RP(proxy);
}

/** RomThumbCreator **/

RomThumbCreator::RomThumbCreator()
	: d_ptr(new RomThumbCreatorPrivate())
{ }

RomThumbCreator::~RomThumbCreator()
{
	delete d_ptr;
}

/**
 * Create a thumbnail for a ROM image.
 * @param path Local pathname of the ROM image.
 * @param width Requested width.
 * @param height Requested height.
 * @param img Target image.
 * @return True if a thumbnail was created; false if not.
 */
bool RomThumbCreator::create(const QString &path, int width, int height, QImage &img)
{
	Q_UNUSED(height);

	// Assuming width and height are the same.
	// TODO: What if they aren't?
	Q_D(RomThumbCreator);
	int ret = d->getThumbnail(Q2RP(path), width, img);
	return (ret == 0);
}
