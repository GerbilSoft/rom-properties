/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * TCreateThumbnail.cpp: Thumbnail creator template.                       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

// NOTE: This is #included in other files,
// so don't use any 'using' statements!

#include "TCreateThumbnail.hpp"

// Cache Manager
#include "CacheManager.hpp"

// librpbase, librpfile
#include "librpbase/RomData.hpp"
#include "librpbase/RomFields.hpp"
#include "librpbase/config/Config.hpp"
#include "librpbase/img/RpImageLoader.hpp"
#include "librpfile/RpFile.hpp"
using namespace LibRpBase;
using namespace LibRpFile;

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

// libromdata
#include "../RomDataFactory.hpp"

// C includes. (C++ namespace)
#include <cassert>
#include <cstring>

// C++ includes.
#include <memory>
using std::unique_ptr;

namespace LibRomData {

/**
 * Get an internal image.
 * @param romData	[in] RomData object.
 * @param imageType	[in] Image type.
 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
 * @param sBIT		[out,opt] sBIT metadata.
 * @return Internal image, or null ImgClass on error.
 */
template<typename ImgClass>
ImgClass TCreateThumbnail<ImgClass>::getInternalImage(
	const RomData *romData,
	RomData::ImageType imageType,
	ImgSize *pOutSize,
	rp_image::sBIT_t *sBIT)
{
	assert(imageType >= RomData::IMG_INT_MIN && imageType <= RomData::IMG_INT_MAX);
	if (imageType < RomData::IMG_INT_MIN || imageType > RomData::IMG_INT_MAX) {
		// Out of range.
		if (sBIT) {
			memset(sBIT, 0, sizeof(*sBIT));
		}
		return getNullImgClass();
	}

	const rp_image *image = romData->image(imageType);
	if (!image) {
		// No image.
		if (sBIT) {
			memset(sBIT, 0, sizeof(*sBIT));
		}
		return getNullImgClass();
	}

	// TODO: Multiple internal image sizes. [add reqSize]

	// Convert the rp_image to ImgClass.
	ImgClass ret_img = rpImageToImgClass(image);
	if (isImgClassValid(ret_img)) {
		// Image converted successfully.
		if (pOutSize) {
			// Get the image size.
			// NOTE: The image may have been resized on Windows,
			// since Windows has issues with non-square images.
			// Hence, we have to get the size from ret_img.
			// TODO: Check for errors?
			getImgClassSize(ret_img, pOutSize);
		}
		if (sBIT) {
			// Get the sBIT metadata.
			if (image->get_sBIT(sBIT) != 0) {
				// No sBIT metadata.
				// Clear the struct.
				memset(sBIT, 0, sizeof(*sBIT));
			}
		}
	}
	return ret_img;
}

/**
 * Get an external image.
 * @param romData	[in] RomData object.
 * @param imageType	[in] Image type.
 * @param reqSize	[in] Requested image size.
 * @param pOutSize	[out,opt] Pointer to ImgSize to store the image's size.
 * @param sBIT		[out,opt] sBIT metadata.
 * @return External image, or null ImgClass on error.
 */
template<typename ImgClass>
ImgClass TCreateThumbnail<ImgClass>::getExternalImage(
	const RomData *romData, RomData::ImageType imageType,
	int reqSize, ImgSize *pOutSize,
	rp_image::sBIT_t *sBIT)
{
	assert(imageType >= RomData::IMG_EXT_MIN && imageType <= RomData::IMG_EXT_MAX);
	if (imageType < RomData::IMG_EXT_MIN || imageType > RomData::IMG_EXT_MAX) {
		// Out of range.
		if (sBIT) {
			memset(sBIT, 0, sizeof(*sBIT));
		}
		return getNullImgClass();
	}

	// Synchronously download from the source URLs.
	// TODO: Image size selection.
	std::vector<RomData::ExtURL> extURLs;
	int ret = romData->extURLs(imageType, &extURLs, reqSize);
	if (ret != 0 || extURLs.empty()) {
		// No URLs.
		if (sBIT) {
			memset(sBIT, 0, sizeof(*sBIT));
		}
		return getNullImgClass();
	}

	// NOTE: This will force a configuration timestamp check.
	const Config *const config = Config::instance();
	const bool extImgDownloadEnabled = config->extImgDownloadEnabled();
	const Config::ImgBandwidth imgBandwidthMode = unlikely(this->isMetered())
		? config->imgBandwidthMetered()
		: config->imgBandwidthUnmetered();

	CacheManager cache;
	const auto extURLs_cend = extURLs.cend();
	for (auto iter = extURLs.cbegin(); iter != extURLs_cend; ++iter) {
		const RomData::ExtURL &extURL = *iter;
		std::string proxy = proxyForUrl(extURL.url.c_str());
		cache.setProxyUrl(!proxy.empty() ? proxy.c_str() : nullptr);

		// Should we attempt to download the image,
		// or just use the local cache?
		// TODO: Verify that this works correctly.
		bool download = extImgDownloadEnabled || (imgBandwidthMode == Config::ImgBandwidth::None);
		if ((imgBandwidthMode != Config::ImgBandwidth::HighRes) && extURL.high_res) {
			// Don't download high-resolution images, but
			// use them if they've already been downloaded.
			download = false;
		}

		// TODO: Have download() return the actual data and/or load the cached file.
		std::string cache_filename;
		if (download) {
			// Attempt to download the image if it isn't already
			// present in the rom-properties cache.
			cache_filename = cache.download(extURL.cache_key);
		} else {
			// Don't attempt to download the image.
			// Only check the rom-properties cache.
			cache_filename = cache.findInCache(extURL.cache_key);
		}
		if (cache_filename.empty())
			continue;

		// Attempt to load the image.
		unique_RefBase<RpFile> file(new RpFile(cache_filename, RpFile::FM_OPEN_READ));
		if (file->isOpen()) {
			rp_image *const dl_img = RpImageLoader::load(file.get());
			if (dl_img && dl_img->isValid()) {
				// Image loaded successfully.
				file->close();
				ImgClass ret_img = rpImageToImgClass(dl_img);
				if (isImgClassValid(ret_img)) {
					// Image converted successfully.
					if (pOutSize) {
						// Get the image size.
						pOutSize->width = dl_img->width();
						pOutSize->height = dl_img->height();
					}
					// Get the sBIT metadata.
					if (sBIT) {
						if (dl_img->get_sBIT(sBIT) != 0) {
							// No sBIT metadata.
							// Clear the struct.
							memset(sBIT, 0, sizeof(*sBIT));
						}
					}
					// TODO: Transparency processing?
					return ret_img;
				}
			}
			UNREF(dl_img);
		}
	}

	// No image.
	if (sBIT) {
		memset(sBIT, 0, sizeof(*sBIT));
	}
	return getNullImgClass();
}

/**
 * Rescale a size while maintaining the aspect ratio.
 * Based on Qt 4.8's QSize::scale().
 * @param rs_size	[in,out] Original size, which will be rescaled.
 * @param tgt_size	[in] Target size.
 */
template<typename ImgClass>
inline void TCreateThumbnail<ImgClass>::rescale_aspect(ImgSize &rs_size, ImgSize tgt_size)
{
	// In the original QSize::scale():
	// - rs_*: this
	// - tgt_*: passed-in QSize
	int64_t rw = ((static_cast<int64_t>(tgt_size.height) *
		       static_cast<int64_t>(rs_size.width)) /
		      static_cast<int64_t>(rs_size.height));
	bool useHeight = (rw <= tgt_size.width);

	if (useHeight) {
		rs_size.width = static_cast<int>(rw);
		rs_size.height = tgt_size.height;
	} else {
		rs_size.height = static_cast<int>((static_cast<int64_t>(tgt_size.width) *
						   static_cast<int64_t>(rs_size.height)) /
						  static_cast<int64_t>(rs_size.width));
		rs_size.width = tgt_size.width;
	}
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param romData	[in] RomData object
 * @param reqSize	[in] Requested image size (single dimension; assuming square image) [0 for full size or largest]
 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(const RomData *romData, int reqSize, GetThumbnailOutParams_t *pOutParams)
{
	assert(romData != nullptr);
	assert(reqSize >= 0);
	assert(pOutParams != nullptr);
	if (reqSize < 0) {
		// Invalid parameter...
		return RPCT_ERROR_INVALID_IMAGE_SIZE;
	}

	// Zero out the output parameters initially.
	pOutParams->thumbSize.width = 0;
	pOutParams->thumbSize.height = 0;
	pOutParams->fullSize.width = 0;
	pOutParams->fullSize.height = 0;
	memset(&pOutParams->sBIT, 0, sizeof(pOutParams->sBIT));
	pOutParams->retImg = getNullImgClass();

	uint32_t imgbf = romData->supportedImageTypes();
	uint32_t imgpf = 0;

	// Get the image priority.
	const Config *const config = Config::instance();
	Config::ImgTypePrio_t imgTypePrio;
	Config::ImgTypeResult res = config->getImgTypePrio(romData->className(), &imgTypePrio);
	switch (res) {
		case Config::ImgTypeResult::Success:
		case Config::ImgTypeResult::SuccessDefaults:
			// Image type priority received successfully.
			// ImgTypeResult::SuccessDefaults indicates the returned
			// data is the default priority, since a custom configuration
			// was not found for this class.
			break;
		case Config::ImgTypeResult::Disabled:
			// Thumbnails are disabled for this class.
			return RPCT_ERROR_SOURCE_FILE_CLASS_DISABLED;
		default:
			// Should not happen...
			assert(!"Invalid return value from Config::getImgTypePrio().");
			return RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
	}

	if (config->useIntIconForSmallSizes() && reqSize <= 48) {
		// Check for an icon first.
		// TODO: Define "small sizes" somewhere. (DPI independence?)
		if (imgbf & RomData::IMGBF_INT_ICON) {
			pOutParams->retImg = getInternalImage(romData, RomData::IMG_INT_ICON, &pOutParams->fullSize, &pOutParams->sBIT);
			imgpf = romData->imgpf(RomData::IMG_INT_ICON);
			imgbf &= ~RomData::IMGBF_INT_ICON;

			if (isImgClassValid(pOutParams->retImg)) {
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

		const uint32_t bf = (1U << imgType);
		if (!(imgbf & bf)) {
			// Image is not present.
			continue;
		}

		// This image may be present.
		if (imgType <= RomData::IMG_INT_MAX) {
			// Internal image.
			pOutParams->retImg = getInternalImage(romData, imgType, &pOutParams->fullSize, &pOutParams->sBIT);
			imgpf = romData->imgpf(imgType);
		} else {
			// External image.
			pOutParams->retImg = getExternalImage(romData, imgType, reqSize, &pOutParams->fullSize, &pOutParams->sBIT);
			imgpf = romData->imgpf(imgType);
		}

		if (isImgClassValid(pOutParams->retImg)) {
			// Image retrieved.
			break;
		}

		// Make sure we don't check this image type again
		// in case there are duplicate entries in the
		// priority list.
		imgbf &= ~bf;
	}

	if (!isImgClassValid(pOutParams->retImg)) {
		// No image.
		return RPCT_ERROR_SOURCE_FILE_NO_IMAGE;
	}

skip_image_check:
	if (pOutParams->fullSize.width <= 0 || pOutParams->fullSize.height <= 0) {
		// Image size is invalid.
		freeImgClass(pOutParams->retImg);
		pOutParams->retImg = getNullImgClass();
		return RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
	}

	if (imgpf & RomData::IMGPF_RESCALE_RFT_DIMENSIONS_2) {
		// Find the second RFT_DIMENSIONS field.
		const RomFields *const fields = romData->fields();
		const RomFields::Field *field[2] = {nullptr, nullptr};
		const auto iter_end = fields->cend();
		for (auto iter = fields->cbegin(); iter != iter_end; ++iter) {
			if (iter->type != RomFields::RFT_DIMENSIONS)
				continue;
			// Found an RFT_DIMENSIONS.
			if (!field[0]) {
				field[0] = &(*iter);
			} else {
				field[1] = &(*iter);
				break;
			}
		}

		if (field[1]) {
			// Found dimensions.
			ImgSize rescaleSize = {
				field[1]->data.dimensions[0],
				field[1]->data.dimensions[1],
			};
			ImgClass scaled_img = rescaleImgClass(pOutParams->retImg, rescaleSize);
			if (isImgClassValid(scaled_img)) {
				freeImgClass(pOutParams->retImg);
				pOutParams->retImg = scaled_img;
				pOutParams->fullSize = rescaleSize;

				// Disable nearest-neighbor scaling, since we already lost
				// pixel-perfect sharpness with the rescale.
				imgpf &= ~RomData::IMGPF_RESCALE_NEAREST;
			}
		}
	}

	if (imgpf & RomData::IMGPF_RESCALE_ASPECT_8to7) {
		// If the image width is 256 or 512, rescale to an 8:7 pixel aspect ratio.
		int scaleW = 0;
		switch (pOutParams->fullSize.width) {
			case 256:
				scaleW = 292;
				break;
			case 512:
				scaleW = 584;
				break;
			default:
				break;
		}
		if (scaleW != 0) {
			pOutParams->fullSize.width = scaleW;
			ImgClass scaled_img = rescaleImgClass(pOutParams->retImg, pOutParams->fullSize, ScalingMethod::Bilinear);
			if (isImgClassValid(scaled_img)) {
				freeImgClass(pOutParams->retImg);
				pOutParams->retImg = scaled_img;

				// Disable nearest-neighbor scaling, since we already lost
				// pixel-perfect sharpness with the 8:7 rescale.
				imgpf &= ~RomData::IMGPF_RESCALE_NEAREST;
			}
		}
	}

	// Thumbnail size, in case it has to be adjusted.
	ImgSize thumbSize = pOutParams->fullSize;

	if (reqSize > 0 && (imgpf & RomData::IMGPF_RESCALE_NEAREST)) {
		// Nearest-neighbor upscale may be needed.
		// TODO: User configuration.
		ResizeNearestUpPolicy resize_up = RESIZE_UP_HALF;
		bool needs_resize_up = false;

		// FIXME: Only if both dimensions are less, or if the second dimension
		// isn't much bigger? (e.g. skip 64x1024)
		switch (resize_up) {
			case RESIZE_UP_NONE:
				// No resize.
				break;

			case RESIZE_UP_HALF:
			default:
				// Only resize images that are less than or equal to
				// half requested thumbnail size.
				needs_resize_up = (thumbSize.width  <= (reqSize/2)) ||
						  (thumbSize.height <= (reqSize/2));
				break;

			case RESIZE_UP_ALL:
				// Resize all images that are smaller than the
				// requested thumbnail size.
				needs_resize_up = (thumbSize.width  < reqSize) ||
						  (thumbSize.height < reqSize);
				break;
		}

		if (needs_resize_up) {
			// Need to upscale the image.
			ImgSize int_sz = {reqSize, reqSize};
			// Resize to the next highest integer multiple.
			int_sz.width -= (int_sz.width % thumbSize.width);
			int_sz.height -= (int_sz.height % thumbSize.height);

			// Calculate the closest size while maintaining the aspect ratio.
			// Based on Qt 4.8's QSize::scale().
			ImgSize rescale_sz = thumbSize;
			rescale_aspect(rescale_sz, int_sz);

			// FIXME: If the original image is 64x1024, the rescale
			// may result in 0x0, which is no good. If this happens,
			// skip the rescaling entirely.
			if (rescale_sz.width > 0 && rescale_sz.height > 0) {
				ImgClass scaled_img = rescaleImgClass(pOutParams->retImg, rescale_sz);
				if (isImgClassValid(scaled_img)) {
					freeImgClass(pOutParams->retImg);
					pOutParams->retImg = scaled_img;
					thumbSize = rescale_sz;
				}
			}
		}
	}

	// Check if a downscale is needed.
	if (reqSize > 0 && (thumbSize.width > reqSize || thumbSize.height > reqSize)) {
		// Downscale is needed.
		ImgSize rescale_sz = thumbSize;
		const ImgSize target_sz = {reqSize, reqSize};
		rescale_aspect(rescale_sz, target_sz);

		// FIXME: If the original image is 64x1024, the rescale
		// may result in 0x0, which is no good. If this happens,
		// skip the rescaling entirely.
		if (rescale_sz.width > 0 && rescale_sz.height > 0) {
			ImgClass scaled_img = rescaleImgClass(pOutParams->retImg, rescale_sz, ScalingMethod::Bilinear);
			if (isImgClassValid(scaled_img)) {
				freeImgClass(pOutParams->retImg);
				pOutParams->retImg = scaled_img;
				thumbSize = rescale_sz;
			}
		}
	}
	// Image retrieved successfully.
	pOutParams->thumbSize = thumbSize;
	return RPCT_SUCCESS;
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param file		[in] Open IRpFile object
 * @param reqSize	[in] Requested image size (single dimension; assuming square image)
 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(IRpFile *file, int reqSize, GetThumbnailOutParams_t *pOutParams)
{
	assert(file != nullptr);
	assert(reqSize > 0);
	assert(pOutParams != nullptr);
	if (reqSize <= 0) {
		// Invalid parameter...
		return RPCT_ERROR_INVALID_IMAGE_SIZE;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	if (!romData) {
		// ROM is not supported.
		return RPCT_ERROR_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Call the actual function.
	int ret = getThumbnail(romData, reqSize, pOutParams);
	romData->unref();
	return ret;
}

/**
 * Create a thumbnail for the specified ROM file.
 * @param filename	[in] ROM file
 * @param reqSize	[in] Requested image size (single dimension; assuming square image)
 * @param pOutParams	[out] Output parameters (If an error occurs, pOutParams->retImg will be null)
 * @return 0 on success; non-zero on error.
 */
template<typename ImgClass>
int TCreateThumbnail<ImgClass>::getThumbnail(const char *filename, int reqSize, GetThumbnailOutParams_t *pOutParams)
{
	assert(filename != nullptr);
	assert(reqSize > 0);
	assert(pOutParams != nullptr);
	if (reqSize <= 0) {
		// Invalid parameter...
		return RPCT_ERROR_INVALID_IMAGE_SIZE;
	}

	// Attempt to open the ROM file.
	// TODO: OS-specific wrappers, e.g. RpQFile or RpGVfsFile.
	// For now, using RpFile, which is an stdio wrapper.
	RpFile *const file = new RpFile(filename, RpFile::FM_OPEN_READ_GZ);
	if (!file->isOpen()) {
		// Could not open the file.
		file->unref();
		return RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *const romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return RPCT_ERROR_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Call the actual function.
	int ret = getThumbnail(romData, reqSize, pOutParams);
	romData->unref();
	return ret;
}

}
