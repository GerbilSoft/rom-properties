/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.cpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017-2018 by David Korth.                                 *
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
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#include "GdkImageConv.hpp"

// librpbase
#include "librpbase/common.h"
#include "librpbase/RomData.hpp"
#include "librpbase/TextFuncs.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/rp_image.hpp"
#include "librpbase/img/RpPngWriter.hpp"
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
#include <string>
using std::string;
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
		PGDKPIXBUF rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		bool isImgClassValid(const PGDKPIXBUF &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		PGDKPIXBUF getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		void freeImgClass(PGDKPIXBUF &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		PGDKPIXBUF rescaleImgClass(const PGDKPIXBUF &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		int getImgClassSize(const PGDKPIXBUF &imgClass, ImgSize *pOutSize) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		string proxyForUrl(const string &url) const final;
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
 * Get the size of the specified ImgClass.
 * @param imgClass	[in] ImgClass object.
 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
 * @return 0 on success; non-zero on error.
 */
int CreateThumbnailPrivate::getImgClassSize(const PGDKPIXBUF &imgClass, ImgSize *pOutSize) const
{
	pOutSize->width = gdk_pixbuf_get_width(imgClass);
	pOutSize->height = gdk_pixbuf_get_height(imgClass);
	return 0;
}

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
string CreateThumbnailPrivate::proxyForUrl(const string &url) const
{
	// TODO: Optimizations.
	// TODO: Support multiple proxies?
	gchar **proxies = g_proxy_resolver_lookup(proxy_resolver,
		url.c_str(), nullptr, nullptr);
	gchar *proxy = nullptr;
	if (proxies) {
		// Check if the first proxy is "direct://".
		if (strcmp(proxies[0], "direct://") != 0) {
			// Not direct access. Use this proxy.
			proxy = proxies[0];
		}
	}

	string ret = (proxy ? string(proxy, -1) : string());
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
	unique_ptr<IRpFile> file(new RpFile(source_file, RpFile::FM_OPEN_READ));
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
	rp_image::sBIT_t sBIT;
	int ret = d->getThumbnail(romData, maximum_size, ret_img, &sBIT);
	delete d;

	if (ret != 0 || !ret_img) {
		// No image.
		if (ret_img) {
			g_object_unref(ret_img);
		}
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	const int height = gdk_pixbuf_get_height(ret_img);
	unique_ptr<const uint8_t*[]> row_pointers;
	guchar *pixels;
	int rowstride;
	int pwRet;

	// tEXt chunks.
	RpPngWriter::kv_vector kv;
	char mtime_str[32];
	char szFile_str[32];
	GFile *f_src;
	gchar *content_type;
	gchar *uri = nullptr;

	// gdk-pixbuf doesn't support CI8, so we'll assume all
	// images are ARGB32. (Well, ABGR32, but close enough.)
	// TODO: Verify channels, etc.?
	RpPngWriter *pngWriter = new RpPngWriter(output_file,
		gdk_pixbuf_get_width(ret_img), height,
		rp_image::FORMAT_ARGB32);
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		ret = RPCT_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

	/** tEXt chunks. **/
	// NOTE: These are written before IHDR in order to put the
	// tEXt chunks before the IDAT chunk.

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI
	kv.reserve(5);

	// Software.
	// TODO: Distinguish between GNOME and XFCE?
	// TODO: Does gdk_pixbuf_savev() support zTXt?
	// TODO: Set keys in the Qt one.
	kv.push_back(std::make_pair("Software", "ROM Properties Page shell extension (GTK+)"));

	// Modification time and file size.
	mtime_str[0] = 0;
	szFile_str[0] = 0;
	f_src = g_file_new_for_path(source_file);
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
		kv.push_back(std::make_pair("Thumb::MTime", mtime_str));
	}

	// MIME type.
	// TODO: Get this directly from the D-Bus call or similar?
	content_type = g_content_type_guess(source_file, nullptr, 0, nullptr);
	if (content_type) {
		gchar *const mime_type = g_content_type_get_mime_type(content_type);
		if (mime_type) {
			kv.push_back(std::make_pair("Thumb::Mimetype", mime_type));
			g_free(mime_type);
		}
		g_free(content_type);
	}

	// File size.
	if (szFile_str[0] != 0) {
		kv.push_back(std::make_pair("Thumb::Size", szFile_str));
	}

	// URI.
	// NOTE: KDE desktops don't urlencode spaces or non-ASCII characters.
	// GTK+ desktops do urlencode spaces and non-ASCII characters.
	if (g_path_is_absolute(source_file)) {
		// We have an absolute path.
		uri = g_filename_to_uri(source_file, nullptr, nullptr);
	} else {
		// We have a relative path.
		// Convert the filename to an absolute path.
		GFile *curdir = g_file_new_for_path(".");
		if (curdir) {
			GFile *abspath = g_file_resolve_relative_path(curdir, source_file);
			if (abspath) {
				uri = g_file_get_uri(abspath);
				g_object_unref(abspath);
			}
			g_object_unref(curdir);
		}
	}

	if (uri) {
		kv.push_back(std::make_pair("Thumb::URI", uri));
		g_free(uri);
	}

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	pwRet = pngWriter->write_IHDR(&sBIT);
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		ret = RPCT_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

	/** IDAT chunk. **/

	// Initialize the row pointers.
	row_pointers.reset(new const uint8_t*[height]);
	pixels = gdk_pixbuf_get_pixels(ret_img);
	rowstride = gdk_pixbuf_get_rowstride(ret_img);
	for (int y = 0; y < height; y++, pixels += rowstride) {
		row_pointers[y] = pixels;
	}

	// Write the IDAT section.
	pwRet = pngWriter->write_IDAT(row_pointers.get(), true);
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		ret = RPCT_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

cleanup:
	delete pngWriter;
	g_object_unref(ret_img);
	romData->unref();
	return ret;
}
