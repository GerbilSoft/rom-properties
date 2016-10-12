/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016 by David Korth.                                      *
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
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

// C includes.
#include <unistd.h>

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
using std::unique_ptr;

#include <QLabel>
#include <QtCore/QBuffer>
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

class RomThumbCreatorPrivate
{
	private:
		// RomThumbCreatorPrivate is a static class.
		RomThumbCreatorPrivate();
		~RomThumbCreatorPrivate();
		Q_DISABLE_COPY(RomThumbCreatorPrivate)

	public:
		/**
		 * Get an internal image.
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return Internal image, or null QImage on error.
		 */
		static QImage getInternalImage(const RomData *romData, RomData::ImageType imageType);

		/**
		 * Get an external image.
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return External image, or null QImage on error.
		 */
		static QImage getExternalImage(const RomData *romData, RomData::ImageType imageType);
};

/**
 * Get an internal image.
 * @param romData RomData object.
 * @param imageType Image type.
 * @return Internal image, or null QImage on error.
 */
QImage RomThumbCreatorPrivate::getInternalImage(const RomData *romData, RomData::ImageType imageType)
{
	assert(imageType >= RomData::IMG_INT_MIN && imageType <= RomData::IMG_INT_MAX);
	if (imageType < RomData::IMG_INT_MIN || imageType > RomData::IMG_INT_MAX) {
		// Out of range.
		return QImage();
	}

	const rp_image *image = romData->image(imageType);
	if (!image) {
		// No image.
		return QImage();
	}

	// Convert the rp_image to QImage.
	return rpToQImage(image);
}

/**
 * Get an external image.
 * @param romData RomData object.
 * @param imageType Image type.
 * @return External image, or null QImage on error.
 */
QImage RomThumbCreatorPrivate::getExternalImage(const RomData *romData, RomData::ImageType imageType)
{
	assert(imageType >= RomData::IMG_EXT_MIN && imageType <= RomData::IMG_EXT_MAX);
	if (imageType < RomData::IMG_EXT_MIN || imageType > RomData::IMG_EXT_MAX) {
		// Out of range.
		return QImage();
	}

	// Synchronously download from the source URLs.
	const std::vector<RomData::ExtURL> *extURLs = romData->extURLs(imageType);
	if (!extURLs || extURLs->empty()) {
		// No URLs.
		return QImage();
	}

	CacheManager cache;
	for (std::vector<RomData::ExtURL>::const_iterator iter = extURLs->begin();
	     iter != extURLs->end(); ++iter)
	{
		const RomData::ExtURL &extURL = *iter;

		// Check if a proxy is required for this URL.
		// TODO: Optimizations.
		QString proxy = KProtocolManager::proxyForUrl(QUrl(RP2Q(extURL.url)));
		if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
			// No proxy.
			cache.setProxyUrl(nullptr);
		} else {
			// Proxy is specified.
			cache.setProxyUrl(Q2RP(proxy));
		}

		// TODO: Have download() return the actual data and/or load the cached file.
		rp_string cache_filename = cache.download(extURL.url, extURL.cache_key);
		if (cache_filename.empty())
			continue;

		// Attempt to load the image.
		unique_ptr<IRpFile> file(new RpFile(cache_filename, RpFile::FM_OPEN_READ));
		if (file && file->isOpen()) {
			unique_ptr<rp_image> dl_img(RpImageLoader::load(file.get()));
			if (dl_img && dl_img->isValid()) {
				// Image loaded successfully.
				QImage qdl_img = rpToQImage(dl_img.get());
				if (!qdl_img.isNull()) {
					// Image converted successfully.
					// TODO: Width/height and transparency processing?
					return qdl_img;
				}
			}
		}
	}

	// No image.
	return QImage();
}

/** RomThumbCreator **/

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
	Q_UNUSED(width);
	Q_UNUSED(height);

	// Return value.
	// NOTE: Not modifying img unless we have a valid value.
	QImage ret_img;

	// Attempt to open the ROM file.
	// TODO: RpQFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<IRpFile> file(new RpFile(Q2RP(path), RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Could not open the file.
		return false;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	unique_ptr<RomData> romData(RomDataFactory::getInstance(file.get(), true));
	file.reset(nullptr);	// file is dup()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return false;
	}

	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.
	uint32_t imgbf = romData->supportedImageTypes();
	uint32_t imgpf = 0;

	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		ret_img = RomThumbCreatorPrivate::getExternalImage(romData.get(), RomData::IMG_EXT_MEDIA);
		imgpf = romData->imgpf(RomData::IMG_EXT_MEDIA);
	}

	if (ret_img.isNull()) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			ret_img = RomThumbCreatorPrivate::getInternalImage(romData.get(), RomData::IMG_INT_ICON);
			imgpf = romData->imgpf(RomData::IMG_INT_ICON);
		}
	}

	if (ret_img.isNull()) {
		// No image.
		return false;
	}

	if (imgpf & RomData::IMGPF_RESCALE_NEAREST) {
		// TODO: User configuration.
		ResizePolicy resize = RESIZE_HALF;
		bool needs_resize = false;

		switch (resize) {
			case RESIZE_NONE:
				// No resize.
				break;

			case RESIZE_HALF:
			default:
				// Only resize images that are less than or equal to
				// half requested thumbnail size.
				needs_resize = (ret_img.width()  <= (width/2)) ||
					       (ret_img.height() <= (height/2));
				break;

			case RESIZE_ALL:
				// Resize all images that are smaller than the
				// requested thumbnail size.
				needs_resize = (ret_img.width() < width) ||
					       (ret_img.height() < height);
				break;
		}

		if (needs_resize) {
			// Maintain the correct aspect ratio.
			const int img_w = ret_img.width();
			const int img_h = ret_img.height();
			QSize tmp_sz(width, height);
			tmp_sz.scale(ret_img.size(), Qt::KeepAspectRatio);

			// Only resize if the requested size is an integer multiple
			// of the actual image size.
			if ((tmp_sz.width() % img_w == 0) && (tmp_sz.height() % img_h == 0)) {
				// Integer multiple.
				// TODO: Verify how this handles non-square images.
				ret_img = ret_img.scaled(width, height,
					Qt::KeepAspectRatio, Qt::FastTransformation);
			}
		}
	}

	// Return the image.
	img = ret_img;
	return true;
}
