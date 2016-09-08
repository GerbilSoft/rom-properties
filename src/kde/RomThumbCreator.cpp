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

// libcachemgr
#include "libcachemgr/CacheManager.hpp"
using LibCacheMgr::CacheManager;

// C includes.
#include <unistd.h>

// libromdata
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

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
		return new RomThumbCreator();
	}
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

	// Set to true if we have an image.
	bool haveImage = false;

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
	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		// Synchronously download from the source URLs.
		const std::vector<RomData::ExtURL> *extURLs = romData->extURLs(RomData::IMG_EXT_MEDIA);
		if (extURLs && !extURLs->empty()) {
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
					rp_image *dl_img = RpImageLoader::load(file.get());
					if (dl_img && dl_img->isValid()) {
						// Image loaded successfully.
						QImage qdl_img = rpToQImage(dl_img);
						if (!qdl_img.isNull()) {
							// Image converted successfully.
							// TODO: Width/height and transparency processing?
							img = qdl_img;
							haveImage = true;
							break;
						}
					}
				}
			}
		}
	}

	if (!haveImage) {
		// No external media scan.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			const rp_image *image = romData->image(RomData::IMG_INT_ICON);
			if (image) {
				// Convert the icon to QImage.
				img = rpToQImage(image);
				if (!img.isNull()) {
					haveImage = true;
				}
			}
		}
	}

	return haveImage;
}
