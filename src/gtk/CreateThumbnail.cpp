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
#include "libromdata/common.h"
#include "libromdata/RomData.hpp"
#include "libromdata/RomDataFactory.hpp"
#include "libromdata/TextFuncs.hpp"
#include "libromdata/file/RpFile.hpp"
#include "libromdata/img/rp_image.hpp"
#include "libromdata/img/RpImageLoader.hpp"
using namespace LibRomData;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"

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

// Typedef to fix issues when using references to pointers.
typedef GdkPixbuf *PGDKPIXBUF;

class CreateThumbnailPrivate : public TCreateThumbnail<PGDKPIXBUF>
{
	public:
		CreateThumbnailPrivate();

	private:
		typedef TCreateThumbnail<PGDKPIXBUF> super;
		RP_DISABLE_COPY(CreateThumbnailPrivate)

	private:
		GProxyResolver *proxy_resolver;

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		virtual PGDKPIXBUF rpImageToImgClass(const rp_image *img) const override final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const PGDKPIXBUF &imgClass) const override final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual PGDKPIXBUF getNullImgClass(void) const override final;

		/**
		 * Free an ImgClass object.
		 * This may be no-op for e.g. PGDKPIXBUF.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(PGDKPIXBUF &imgClass) const override final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual PGDKPIXBUF rescaleImgClass(const PGDKPIXBUF &imgClass, const ImgSize &sz) const override final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const override final;
};

CreateThumbnailPrivate::CreateThumbnailPrivate()
	: proxy_resolver(g_proxy_resolver_get_default())
{ }

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
PGDKPIXBUF CreateThumbnailPrivate::rpImageToImgClass(const rp_image *img) const
{
	return GdkImageConv::rp_image_to_GdkPixbuf(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool CreateThumbnailPrivate::isImgClassValid(const PGDKPIXBUF &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
PGDKPIXBUF CreateThumbnailPrivate::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * This may be no-op for e.g. PGDKPIXBUF.
 * @param imgClass ImgClass object.
 */
void CreateThumbnailPrivate::freeImgClass(PGDKPIXBUF &imgClass) const
{
	g_object_unref(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
PGDKPIXBUF CreateThumbnailPrivate::rescaleImgClass(const PGDKPIXBUF &imgClass, const ImgSize &sz) const
{
	// TODO: Interpolation option?
	return gdk_pixbuf_scale_simple(imgClass, sz.width, sz.height, GDK_INTERP_NEAREST);
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string CreateThumbnailPrivate::proxyForUrl(const rp_string &url) const
{
	// TODO: Optimizations.
	// TODO: Support multiple proxies?
	gchar **proxies = g_proxy_resolver_lookup(proxy_resolver,
		rp_string_to_utf8(url).c_str(), nullptr, nullptr);
	gchar *proxy = nullptr;
	if (proxies) {
		// Check if the first proxy is "direct://".
		if (strcmp(proxies[0], "direct://") != 0) {
			// Not direct access. Use this proxy.
			proxy = proxies[0];
		}
	}

	rp_string ret = (proxy ? utf8_to_rp_string(proxy, -1) : rp_string());
	g_strfreev(proxies);
	return ret;
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

	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.

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

	// Create the thumbnail.
	// TODO: If image is larger than maximum_size, resize down.
	CreateThumbnailPrivate *d = new CreateThumbnailPrivate();
	GdkPixbuf *ret_img = nullptr;
	int ret = d->getThumbnail(romData, maximum_size, ret_img);
	delete d;

	if (ret != 0 || !ret_img) {
		// No image.
		if (ret_img) {
			g_object_unref(ret_img);
		}
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image.
	ret = RPCT_SUCCESS;
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
