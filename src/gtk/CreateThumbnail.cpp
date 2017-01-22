/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.cpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017 by David Korth.                                      *
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

#include "CreateThumbnail.hpp"
#include "GdkImageConv.hpp"

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

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
using std::unique_ptr;

// glib
#include <glib.h>
#include <glib-object.h>

// GdkPixbuf
#include <gdk-pixbuf/gdk-pixbuf.h>

/** CreateThumbnailPrivate **/

class CreateThumbnailPrivate
{
	private:
		// CreateThumbnailPrivate is a static class.
		CreateThumbnailPrivate();
		~CreateThumbnailPrivate();
		CreateThumbnailPrivate(const CreateThumbnailPrivate &other);
		CreateThumbnailPrivate &operator=(const CreateThumbnailPrivate &other);

	public:
		/**
		 * Get an internal image.
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return Internal image, or nullptr on error.
		 */
		static GdkPixbuf *getInternalImage(const RomData *romData, RomData::ImageType imageType);

		/**
		 * Get an external image.
		 * @param romData RomData object.
		 * @param imageType Image type.
		 * @return External image, or null QImage on error.
		 */
		static GdkPixbuf *getExternalImage(const RomData *romData, RomData::ImageType imageType);

		/**
		 * Rescale a size while maintaining the aspect ratio.
		 * Based on Qt 4.8's QSize::scale().
		 * @param rs_width	[in,out] Original width, which will be rescaled.
		 * @param rs_height	[in,out] Original height, which will be rescaled.
		 * @param tgt_width	[in] Target width.
		 * @param tgt_height	[in] Target height.
		 */
		static inline void rescale_aspect(int *rs_width, int *rs_height, int tgt_width, int tgt_height);
};

/**
 * Get an internal image.
 * @param romData RomData object.
 * @param imageType Image type.
 * @return Internal image, or null QImage on error.
 */
GdkPixbuf *CreateThumbnailPrivate::getInternalImage(const RomData *romData, RomData::ImageType imageType)
{
	assert(imageType >= RomData::IMG_INT_MIN && imageType <= RomData::IMG_INT_MAX);
	if (imageType < RomData::IMG_INT_MIN || imageType > RomData::IMG_INT_MAX) {
		// Out of range.
		return nullptr;
	}

	const rp_image *image = romData->image(imageType);
	if (!image) {
		// No image.
		return nullptr;
	}

	// Convert the rp_image to GdkPixbuf.
	return GdkImageConv::rp_image_to_GdkPixbuf(image);
}

/**
 * Get an external image.
 * @param romData RomData object.
 * @param imageType Image type.
 * @return External image, or null QImage on error.
 */
GdkPixbuf *CreateThumbnailPrivate::getExternalImage(const RomData *romData, RomData::ImageType imageType)
{
	assert(imageType >= RomData::IMG_EXT_MIN && imageType <= RomData::IMG_EXT_MAX);
	if (imageType < RomData::IMG_EXT_MIN || imageType > RomData::IMG_EXT_MAX) {
		// Out of range.
		return nullptr;
	}

	// Synchronously download from the source URLs.
	const std::vector<RomData::ExtURL> *extURLs = romData->extURLs(imageType);
	if (!extURLs || extURLs->empty()) {
		// No URLs.
		return nullptr;
	}

	// Use the GIO proxy resolver to look up proxies.
	GProxyResolver *proxy_resolver = g_proxy_resolver_get_default();

	CacheManager cache;
	for (auto iter = extURLs->cbegin(); iter != extURLs->cend(); ++iter) {
		const RomData::ExtURL &extURL = *iter;

		// TODO: Optimizations.
		// TODO: Support multiple proxies?
		gchar **proxies = g_proxy_resolver_lookup(proxy_resolver, extURL.url.c_str(), nullptr, nullptr);
		gchar *proxy = nullptr;
		if (proxies) {
			// Check if the first proxy is "direct://".
			if (strcmp(proxies[0], "direct://") != 0) {
				// Not direct access. Use this proxy.
				proxy = proxies[0];
			}
		}
		cache.setProxyUrl(proxy);
		g_strfreev(proxies);

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
				GdkPixbuf *pixbuf = GdkImageConv::rp_image_to_GdkPixbuf(dl_img.get());
				if (pixbuf) {
					// Image converted successfully.
					// TODO: Width/height and transparency processing?
					return pixbuf;
				}
			}
		}
	}

	// No image.
	return nullptr;
}

/**
 * Rescale a size while maintaining the aspect ratio.
 * Based on Qt 4.8's QSize::scale().
 * @param rs_width	[in,out] Original width, which will be rescaled.
 * @param rs_height	[in,out] Original height, which will be rescaled.
 * @param tgt_width	[in] Target width.
 * @param tgt_height	[in] Target height.
 */
inline void CreateThumbnailPrivate::rescale_aspect(int *rs_width, int *rs_height, int tgt_width, int tgt_height)
{
	// In the original QSize::scale():
	// - rs_*: this
	// - tgt_*: passed-in QSize
	int64_t rw = (((int64_t)tgt_height * (int64_t)*rs_width) / (int64_t)*rs_height);
	bool useHeight = (rw <= tgt_width);

	if (useHeight) {
		*rs_width = rw;
		*rs_height = tgt_height;
	} else {
		*rs_height = (int)(((int64_t)tgt_width * (int64_t)*rs_height) / (int64_t)*rs_width);
		*rs_width = tgt_width;
	}
}

/** CreateThumbnail **/

/**
 * Thumbnail creator function for wrapper programs.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
int rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// Some of this is based on the GNOME Thumbnailer skeleton project.
	// https://github.com/hadess/gnome-thumbnailer-skeleton/blob/master/gnome-thumbnailer-skeleton.c

	// Make sure glib is initialized.
	// NOTE: This is a no-op as of glib-2.36.
#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif

	// Attempt to open the ROM file.
	// TODO: RpGVfsFile wrapper.
	// For now, using RpFile, which is an stdio wrapper.
	unique_ptr<IRpFile> file(new RpFile(source_file, RpFile::FM_OPEN_READ));
	if (!file || !file->isOpen()) {
		// Could not open the file.
		return RPCT_SOURCE_FILE_ERROR;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::getInstance(file.get(), true);
	file.reset(nullptr);	// file is dup()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return RPCT_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Image to save.
	GdkPixbuf *ret_img = nullptr;

	// TODO: Customize which ones are used per-system.
	// For now, check EXT MEDIA, then INT ICON.
	uint32_t imgbf = romData->supportedImageTypes();
	uint32_t imgpf = 0;

	if (imgbf & RomData::IMGBF_EXT_MEDIA) {
		// External media scan.
		ret_img = CreateThumbnailPrivate::getExternalImage(romData, RomData::IMG_EXT_MEDIA);
		imgpf = romData->imgpf(RomData::IMG_EXT_MEDIA);
	}

	if (!ret_img) {
		// No external media scan.
		// Try an internal image.
		if (imgbf & RomData::IMGBF_INT_ICON) {
			// Internal icon.
			ret_img = CreateThumbnailPrivate::getInternalImage(romData, RomData::IMG_INT_ICON);
			imgpf = romData->imgpf(RomData::IMG_INT_ICON);
		}
	}

	if (!ret_img) {
		// No image.
		romData->unref();
		return false;
	}

	// TODO: If image is larger than maximum_size, resize down.
	if (imgpf & RomData::IMGPF_RESCALE_NEAREST) {
		// TODO: User configuration.
		ResizeNearestUpPolicy resize_up = RESIZE_UP_HALF;
		bool needs_resize_up = false;
		const int img_w = gdk_pixbuf_get_width(ret_img);
		const int img_h = gdk_pixbuf_get_height(ret_img);

		switch (resize_up) {
			case RESIZE_UP_NONE:
				// No resize.
				break;

			case RESIZE_UP_HALF:
			default:
				// Only resize images that are less than or equal to
				// half requested thumbnail size.
				needs_resize_up = (img_w <= (maximum_size/2)) ||
						  (img_h <= (maximum_size/2));
				break;

			case RESIZE_UP_ALL:
				// Resize all images that are smaller than the
				// requested thumbnail size.
				needs_resize_up = (img_w < maximum_size) ||
						  (img_h < maximum_size);
				break;
		}

		if (needs_resize_up) {
			// Need to upscale the image.
			int width = maximum_size;
			int height = maximum_size;
			// Resize to the next highest integer multiple.
			width -= (width % img_w);
			height -= (height % img_h);

			// Calculate the closest size while maintaining the aspect ratio.
			// Based on Qt 4.8's QSize::scale().
			int rs_width = img_w;
			int rs_height = img_h;
			CreateThumbnailPrivate::rescale_aspect(&rs_width, &rs_height, width, height);
			GdkPixbuf *scaled_img = gdk_pixbuf_scale_simple(
				ret_img, rs_width, rs_height, GDK_INTERP_NEAREST);
			g_object_unref(ret_img);
			ret_img = scaled_img;
		}
	}

	// Save the image.
	int ret = RPCT_SUCCESS;
	GError *error = nullptr;
	if (!gdk_pixbuf_save(ret_img, output_file, "png", &error, nullptr)) {
		// Image save failed.
		g_error_free(error);
		ret = RPCT_OUTPUT_FILE_FAILED;
	}

	g_object_unref(ret_img);
	romData->unref();
	return ret;
}
