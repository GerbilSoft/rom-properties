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

#include "GdkImageConv.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/RomData.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/rp_image.hpp"
using namespace LibRpBase;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"
using LibRomData::TCreateThumbnail;

// C includes. (C++ namespace)
#include <cinttypes>
#include <cstdio>
#include <cstring>

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

// NOTE: G_MODULE_EXPORT is a no-op on non-Windows platforms.
#if !defined(_WIN32) && defined(__GNUC__) && __GNUC__ >= 4
#undef G_MODULE_EXPORT
#define G_MODULE_EXPORT __attribute__ ((visibility ("default")))
#endif

/**
 * Thumbnail creator function for wrapper programs.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
extern "C"
G_MODULE_EXPORT int rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
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
	unique_ptr<IRpFile> file(new RpFile(utf8_to_rp_string(source_file), RpFile::FM_OPEN_READ));
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

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI
	const char *option_keys[8];
	const char *option_values[8];
	int curopt = 0;

	// TODO: Distinguish between GNOME and XFCE?
	// TODO: Does gdk_pixbuf_savev() support zTXt?
	// TODO: Set keys in the Qt one.
	option_keys[curopt] = "tEXt::Software";
	option_values[curopt] = "ROM Properties Page shell extension (GTK+)";
	curopt++;

	// Modification time and file size.
	char mtime_str[32];
	char szFile_str[32];
	mtime_str[0] = 0;
	szFile_str[0] = 0;
	GFile *const f_src = g_file_new_for_path(source_file);
	if (f_src) {
		GError *error = nullptr;
		GFileInfo *const fi_src = g_file_query_info(f_src,
			G_FILE_ATTRIBUTE_TIME_MODIFIED "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
			G_FILE_QUERY_INFO_NONE, nullptr, &error);
		if (!error) {
			// Get the modification time.
			guint64 mtime = g_file_info_get_attribute_uint64(fi_src, G_FILE_ATTRIBUTE_TIME_MODIFIED);
			if (mtime > 0) {
				snprintf(mtime_str, sizeof(mtime_str), "%" PRId64, (int64_t)mtime);
			}

			// Get the file size.
			gint64 szFile = g_file_info_get_size(fi_src);
			if (szFile > 0) {
				snprintf(szFile_str, sizeof(szFile_str), "%" PRId64, szFile);
			}

			g_object_unref(fi_src);
		} else {
			g_error_free(error);
		}
		g_object_unref(f_src);
	}

	// Modification time.
	if (mtime_str[0] != 0) {
		option_keys[curopt] = "tEXt::Thumb::MTime";
		option_values[curopt] = mtime_str;
		curopt++;
	}

	// MIME type.
	// TODO: Get this directly from the D-Bus call or similar?
	gchar *const content_type = g_content_type_guess(source_file, nullptr, 0, nullptr);
	gchar *mime_type = nullptr;
	if (content_type) {
		gchar *const mime_type = g_content_type_get_mime_type(content_type);
		if (mime_type) {
			option_keys[curopt] = "tEXt::Thumb::Mimetype";
			option_values[curopt] = mime_type;
			curopt++;
		}
		g_free(content_type);
	}

	// File size.
	if (szFile_str[0] != 0) {
		option_keys[curopt] = "tEXt::Thumb::Size";
		option_values[curopt] = szFile_str;
		curopt++;
	}

	// URI.
	gchar *const uri = g_filename_to_uri(source_file, nullptr, nullptr);
	if (uri) {
		option_keys[curopt] = "tEXt::Thumb::URI";
		option_values[curopt] = uri;
		curopt++;
	}

	// End of options.
	option_keys[curopt] = nullptr;
	option_values[curopt] = nullptr;

	// Save the image.
	ret = RPCT_SUCCESS;
	GError *error = nullptr;
	if (!gdk_pixbuf_savev(ret_img, output_file, "png",
	    (char**)option_keys, (char**)option_values, &error))
	{
		// Image save failed.
		g_error_free(error);
		ret = RPCT_OUTPUT_FILE_FAILED;
	}

	g_free(uri);
	g_free(mime_type);
	g_object_unref(ret_img);
	romData->unref();
	return ret;
}
