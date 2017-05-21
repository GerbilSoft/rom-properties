/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TCreateThumbnail.cpp: Thumbnail creator template.                       *
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

#ifndef __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_CPP__
#define __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_CPP__

#include "TCreateThumbnail.hpp"

// libcachemgr
#include "libcachemgr/CacheManager.hpp"
using LibCacheMgr::CacheManager;

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpImageLoader.hpp"
#include "librpbase/config/Config.hpp"
using namespace LibRpBase;

// libromdata
#include "../RomDataFactory.hpp"

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRomData {

template<typename ImgClass>
TCreateThumbnail<ImgClass>::TCreateThumbnail()
{ }

template<typename ImgClass>
TCreateThumbnail<ImgClass>::~TCreateThumbnail()
{ }

/**
 * Get an internal image.
 * @param romData	[in] RomData object.
 * @param imageType	[in] Image type.
 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
 * @return Internal image, or null ImgClass on error.
 */
template<typename ImgClass>
ImgClass TCreateThumbnail<ImgClass>::getInternalImage(
	const RomData *romData,
	RomData::ImageType imageType,
	ImgSize *pOutSize)
{
	assert(imageType >= RomData::IMG_INT_MIN && imageType <= RomData::IMG_INT_MAX);
	if (imageType < RomData::IMG_INT_MIN || imageType > RomData::IMG_INT_MAX) {
		// Out of range.
		return getNullImgClass();
	}

	const rp_image *image = romData->image(imageType);
	if (!image) {
		// No image.
		return getNullImgClass();
	}

	// Convert the rp_image to ImgClass.
	ImgClass ret_img = rpImageToImgClass(image);
	if (isImgClassValid(ret_img)) {
		// Image converted successfully.
		if (pOutSize) {
			// Get the image size.
			pOutSize->width = image->width();
			pOutSize->height = image->height();
		}
	}
	return ret_img;
}

/**
 * Get an external image.
 * @param romData	[in] RomData object.
 * @param imageType	[in] Image type.
 * @param req_size	[in] Requested image size.
 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
 * @return External image, or null ImgClass on error.
 */
template<typename ImgClass>
ImgClass TCreateThumbnail<ImgClass>::getExternalImage(
	const RomData *romData, RomData::ImageType imageType,
	int req_size, ImgSize *pOutSize)
{
	assert(imageType >= RomData::IMG_EXT_MIN && imageType <= RomData::IMG_EXT_MAX);
	if (imageType < RomData::IMG_EXT_MIN || imageType > RomData::IMG_EXT_MAX) {
		// Out of range.
		return getNullImgClass();
	}

	// Synchronously download from the source URLs.
	// TODO: Image size selection.
	std::vector<RomData::ExtURL> extURLs;
	int ret = romData->extURLs(imageType, &extURLs, req_size);
	if (ret != 0 || extURLs.empty()) {
		// No URLs.
		return getNullImgClass();
	}

	// NOTE: This will force a configuration timestamp check.
	const Config *const config = Config::instance();
	const bool extImgDownloadEnabled = config->extImgDownloadEnabled();
	const bool downloadHighResScans = config->downloadHighResScans();

	CacheManager cache;
	for (auto iter = extURLs.cbegin(); iter != extURLs.cend(); ++iter) {
		const RomData::ExtURL &extURL = *iter;
		rp_string proxy = proxyForUrl(extURL.url);
		cache.setProxyUrl(!proxy.empty() ? proxy.c_str() : nullptr);

		// Should we attempt to download the image,
		// or just use the local cache?
		// TODO: Verify that this works correctly.
		bool download = extImgDownloadEnabled;
		if (!downloadHighResScans && extURL.high_res) {
			// Don't download high-resolution images, but
			// use them if they've already been downloaded.
			download = false;
		}

		// TODO: Have download() return the actual data and/or load the cached file.
		rp_string cache_filename;
		if (download) {
			// Attempt to download the image if it isn't already
			// present in the rom-properties cache.
			cache_filename = cache.download(extURL.url, extURL.cache_key);
		} else {
			// Don't attempt to download the image.
			// Only check the rom-properties cache.
			cache_filename = cache.findInCache(extURL.cache_key);
		}
		if (cache_filename.empty())
			continue;

		// Attempt to load the image.
		unique_ptr<IRpFile> file(new RpFile(cache_filename, RpFile::FM_OPEN_READ));
		if (file && file->isOpen()) {
			unique_ptr<rp_image> dl_img(RpImageLoader::load(file.get()));
			if (dl_img && dl_img->isValid()) {
				// Image loaded successfully.
				ImgClass ret_img = rpImageToImgClass(dl_img.get());
				if (isImgClassValid(ret_img)) {
					// Image converted successfully.
					if (pOutSize) {
						// Get the image size.
						pOutSize->width = dl_img->width();
						pOutSize->height = dl_img->height();
					}
					// TODO: Transparency processing?
					return ret_img;
				}
			}
		}
	}

	// No image.
	return getNullImgClass();
}

/**
 * Rescale a size while maintaining the aspect ratio.
 * Based on Qt 4.8's QSize::scale().
 * @param rs_size	[in,out] Original size, which will be rescaled.
 * @param tgt_size	[in] Target size.
 */
template<typename ImgClass>
inline void TCreateThumbnail<ImgClass>::rescale_aspect(ImgSize &rs_size, const ImgSize &tgt_size)
{
	// In the original QSize::scale():
	// - rs_*: this
	// - tgt_*: passed-in QSize
	int64_t rw = (((int64_t)tgt_size.height * (int64_t)rs_size.width) / (int64_t)rs_size.height);
	bool useHeight = (rw <= tgt_size.width);

	if (useHeight) {
		rs_size.width = (int)rw;
		rs_size.height = tgt_size.height;
	} else {
		rs_size.height = (int)(((int64_t)tgt_size.width * (int64_t)rs_size.height) / (int64_t)rs_size.width);
		rs_size.width = tgt_size.width;
	}
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param romData	[in] RomData object.
 * @param req_size	[in] Requested image size.
 * @param ret_img	[out] Return image.
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(const RomData *romData, int req_size, ImgClass &ret_img)
{
	uint32_t imgbf = romData->supportedImageTypes();
	uint32_t imgpf = 0;
	ImgSize img_sz;

	// Get the image priority.
	const Config *const config = Config::instance();
	Config::ImgTypePrio_t imgTypePrio;
	Config::ImgTypeResult res = config->getImgTypePrio(romData->className(), &imgTypePrio);
	switch (res) {
		case Config::IMGTR_SUCCESS:
		case Config::IMGTR_SUCCESS_DEFAULTS:
			// Image type priority received successfully.
			// IMGTR_SUCCESS_DEFAULTS indicates the returned
			// data is the default priority, since a custom
			// configuration was not found for this class.
			break;
		case Config::IMGTR_DISABLED:
			// Thumbnails are disabled for this class.
			return RPCT_SOURCE_FILE_CLASS_DISABLED;
		default:
			// Should not happen...
			assert(!"Invalid return value from Config::getImgTypePrio().");
			return RPCT_SOURCE_FILE_ERROR;
	}

	if (config->useIntIconForSmallSizes() && req_size <= 48) {
		// Check for an icon first.
		// TODO: Define "small sizes" somewhere. (DPI independence?)
		if (imgbf & RomData::IMGBF_INT_ICON) {
			ret_img = getInternalImage(romData, RomData::IMG_INT_ICON, &img_sz);
			imgpf = romData->imgpf(RomData::IMG_INT_ICON);
			imgbf &= ~RomData::IMGBF_INT_ICON;

			if (isImgClassValid(ret_img)) {
				// Image retrieved.
				// TODO: Better method than goto?
				goto skip_image_check;
			}
		}
	}

	// Check all available images in image priority order.
	// TODO: Use pointer arithmetic in this loop?
	for (unsigned int i = 0; i < imgTypePrio.length; i++) {
		const RomData::ImageType imgType =
			static_cast<RomData::ImageType>(imgTypePrio.imgTypes[i]);
		assert(imgType <= RomData::IMG_EXT_MAX);
		if (imgType > RomData::IMG_EXT_MAX) {
			// Invalid image type. Ignore it.
			continue;
		}

		const uint32_t bf = (1 << imgType);
		if (!(imgbf & bf)) {
			// Image is not present.
			continue;
		}

		// This image may be present.
		if (imgType <= RomData::IMG_INT_MAX) {
			// Internal image.
			ret_img = getInternalImage(romData, imgType, &img_sz);
			imgpf = romData->imgpf(imgType);
		} else {
			// External image.
			ret_img = getExternalImage(romData, imgType, req_size, &img_sz);
			imgpf = romData->imgpf(imgType);
		}

		if (isImgClassValid(ret_img)) {
			// Image retrieved.
			break;
		}

		// Make sure we don't check this image type again
		// in case there are duplicate entries in the
		// priority list.
		imgbf &= ~bf;
	}

	if (!isImgClassValid(ret_img)) {
		// No image.
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

skip_image_check:
	// TODO: If image is larger than req_size, resize down.
	if (imgpf & RomData::IMGPF_RESCALE_NEAREST) {
		// TODO: User configuration.
		ResizeNearestUpPolicy resize_up = RESIZE_UP_HALF;
		bool needs_resize_up = false;

		switch (resize_up) {
			case RESIZE_UP_NONE:
				// No resize.
				break;

			case RESIZE_UP_HALF:
			default:
				// Only resize images that are less than or equal to
				// half requested thumbnail size.
				needs_resize_up = (img_sz.width  <= (req_size/2)) ||
						  (img_sz.height <= (req_size/2));
				break;

			case RESIZE_UP_ALL:
				// Resize all images that are smaller than the
				// requested thumbnail size.
				needs_resize_up = (img_sz.width  < req_size) ||
						  (img_sz.height < req_size);
				break;
		}

		if (needs_resize_up) {
			// Need to upscale the image.
			ImgSize int_sz = {req_size, req_size};
			// Resize to the next highest integer multiple.
			int_sz.width -= (int_sz.width % img_sz.width);
			int_sz.height -= (int_sz.height % img_sz.height);

			// Calculate the closest size while maintaining the aspect ratio.
			// Based on Qt 4.8's QSize::scale().
			ImgSize rescale_sz = img_sz;
			rescale_aspect(rescale_sz, int_sz);

			ImgClass scaled_img = rescaleImgClass(ret_img, rescale_sz);
			freeImgClass(ret_img);
			ret_img = scaled_img;
		}
	}

	// Image retrieved successfully.
	return RPCT_SUCCESS;
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param file		[in] Open IRpFile object.
 * @param req_size	[in] Requested image size.
 * @param ret_img	[out] Return image.
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(IRpFile *file, int req_size, ImgClass &ret_img)
{
	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::create(file, true);
	if (!romData) {
		// ROM is not supported.
		return RPCT_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Call the actual function.
	int ret = getThumbnail(romData, req_size, ret_img);
	romData->unref();
	return ret;
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param filename	[in] ROM file.
 * @param req_size	[in] Requested image size.
 * @param ret_img	[out] Return image.
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(const rp_char *filename, int req_size, ImgClass &ret_img)
{
	// Attempt to open the ROM file.
	// TODO: OS-specific wrappers, e.g. RpQFile or RpGVfsFile.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<IRpFile> file(new RpFile(filename, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Could not open the file.
		return RPCT_SOURCE_FILE_ERROR;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::create(file.get(), true);
	file.reset(nullptr);	// file is dup()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return RPCT_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Call the actual function.
	int ret = getThumbnail(romData, req_size, ret_img);
	romData->unref();
	return ret;
}

}

#endif /* __ROMPROPERTIES_LIBROMDATA_IMG_TCREATETHUMBNAIL_CPP__ */
