/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.cpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017-2019 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

// librpbase
#include "librpbase/common.h"
#include "librpbase/RomData.hpp"
#include "librpbase/file/FileSystem.hpp"
#include "librpbase/file/RpFile.hpp"
#include "librpbase/img/RpPngWriter.hpp"
using namespace LibRpBase;

// RpFileGio
#include "RpFile_gio.hpp"

// librptexture
#include "librptexture/img/rp_image.hpp"
using LibRpTexture::rp_image;

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

// PIMGTYPE
#include "PIMGTYPE.hpp"

// GTK+ major version.
// We can't simply use GTK_MAJOR_VERSION because
// that has parentheses.
#if GTK_CHECK_VERSION(4,0,0)
# error Needs updating for GTK4.
#elif GTK_CHECK_VERSION(3,0,0)
# define GTK_MAJOR_STR "3"
#elif GTK_CHECK_VERSION(2,0,0)
# define GTK_MAJOR_STR "2"
#else
# error GTK+ is too old.
#endif

/** CreateThumbnailPrivate **/

class CreateThumbnailPrivate : public TCreateThumbnail<PIMGTYPE>
{
	public:
		CreateThumbnailPrivate();

	private:
		typedef TCreateThumbnail<PIMGTYPE> super;
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
		PIMGTYPE rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		bool isImgClassValid(const PIMGTYPE &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		PIMGTYPE getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		void freeImgClass(PIMGTYPE &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		PIMGTYPE rescaleImgClass(const PIMGTYPE &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		int getImgClassSize(const PIMGTYPE &imgClass, ImgSize *pOutSize) const final;

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
PIMGTYPE CreateThumbnailPrivate::rpImageToImgClass(const rp_image *img) const
{
	return rp_image_to_PIMGTYPE(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool CreateThumbnailPrivate::isImgClassValid(const PIMGTYPE &imgClass) const
{
	return (imgClass != nullptr);
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
PIMGTYPE CreateThumbnailPrivate::getNullImgClass(void) const
{
	return nullptr;
}

/**
 * Free an ImgClass object.
 * @param imgClass ImgClass object.
 */
void CreateThumbnailPrivate::freeImgClass(PIMGTYPE &imgClass) const
{
	PIMGTYPE_destroy(imgClass);
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
PIMGTYPE CreateThumbnailPrivate::rescaleImgClass(const PIMGTYPE &imgClass, const ImgSize &sz) const
{
	// TODO: Interpolation option?
	return PIMGTYPE_scale(imgClass, sz.width, sz.height, false);
}

/**
 * Get the size of the specified ImgClass.
 * @param imgClass	[in] ImgClass object.
 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
 * @return 0 on success; non-zero on error.
 */
int CreateThumbnailPrivate::getImgClassSize(const PIMGTYPE &imgClass, ImgSize *pOutSize) const
{
#ifdef RP_GTK_USE_CAIRO
	// TODO: Verify that this is an image surface.
	pOutSize->width = cairo_image_surface_get_width(imgClass);
	pOutSize->height = cairo_image_surface_get_height(imgClass);
#else /* !RP_GTK_USE_CAIRO */
	pOutSize->width = gdk_pixbuf_get_width(imgClass);
	pOutSize->height = gdk_pixbuf_get_height(imgClass);
#endif
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
 * @param source_file Source file or URI. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
extern "C"
G_MODULE_EXPORT int rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// Some of this is based on the GNOME Thumbnailer skeleton project.
	// https://github.com/hadess/gnome-thumbnailer-skeleton/blob/master/gnome-thumbnailer-skeleton.c

	if (getuid() == 0 || geteuid() == 0) {
		g_critical("*** " G_LOG_DOMAIN " does not support running as root.");
		return RPCT_RUNNING_AS_ROOT;
	}

	// Make sure glib is initialized.
	// NOTE: This is a no-op as of glib-2.36.
#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif

	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.

	// Is this a URI or a filename?
	char *source_uri = nullptr;
	char *source_filename = nullptr;
	bool free_source_uri = false;
	bool free_source_filename = false;

	char *const uri_scheme = g_uri_parse_scheme(source_file);
	if (uri_scheme != nullptr) {
		// This is a URI.
		source_uri = const_cast<char*>(source_file);
		g_free(uri_scheme);
		// Check if it's a local filename.
		source_filename = g_filename_from_uri(source_file, nullptr, nullptr);
		if (source_filename) {
			// It's a local filename.
			free_source_filename = true;
		}
	} else {
		// This is a filename.
		source_filename = const_cast<char*>(source_file);

		// We might have a relative path.
		// If so, it needs to be converted to an absolute path.
		if (g_path_is_absolute(source_file)) {
			// We have an absolute path.
			source_uri = g_filename_to_uri(source_file, nullptr, nullptr);
		} else {
			// We have a relative path.
			// Convert the filename to an absolute path.
			GFile *curdir = g_file_new_for_path(".");
			if (curdir) {
				GFile *abspath = g_file_resolve_relative_path(curdir, source_file);
				if (abspath) {
					source_uri = g_file_get_uri(abspath);
					g_object_unref(abspath);
				}
				g_object_unref(curdir);
			}
		}
		free_source_uri = true;
	}

	// Check for "bad" file systems.
	if (source_filename) {
		const Config *const config = Config::instance();
		if (FileSystem::isOnBadFS(source_filename, config->enableThumbnailOnNetworkFS())) {
			// This file is on a "bad" file system.
			if (free_source_uri) {
				g_free(source_uri);
			}
			if (free_source_filename) {
				g_free(source_filename);
			}
			return RPCT_SOURCE_FILE_BAD_FS;
		}
	}

	// Attempt to open the ROM file.
	IRpFile *file;
	if (source_filename) {
		// Local file. Use RpFile.
		file = new RpFile(source_filename, RpFile::FM_OPEN_READ_GZ);
	} else {
		// Not a local file. Use RpFileGio.
		file = new RpFileGio(source_uri);
	}

	if (!file->isOpen()) {
		// Could not open the file.
		file->unref();
		if (free_source_uri) {
			g_free(source_uri);
		}
		if (free_source_filename) {
			g_free(source_filename);
		}
		return RPCT_SOURCE_FILE_ERROR;
	}

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		if (free_source_uri) {
			g_free(source_uri);
		}
		if (free_source_filename) {
			g_free(source_filename);
		}
		return RPCT_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Create the thumbnail.
	// TODO: If image is larger than maximum_size, resize down.
	unique_ptr<CreateThumbnailPrivate> d(new CreateThumbnailPrivate());
	PIMGTYPE ret_img = nullptr;
	rp_image::sBIT_t sBIT;
	int ret = d->getThumbnail(romData, maximum_size, ret_img, &sBIT);

	if (ret != 0 || !d->isImgClassValid(ret_img)) {
		// No image.
		if (ret_img) {
			d->freeImgClass(ret_img);
		}
		romData->unref();
		if (free_source_uri) {
			g_free(source_uri);
		}
		if (free_source_filename) {
			g_free(source_filename);
		}
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	TCreateThumbnail<PIMGTYPE>::ImgSize imgSz;
	d->getImgClassSize(ret_img, &imgSz);
	unique_ptr<const uint8_t*[]> row_pointers;
	guchar *pixels;
	int rowstride;
	int pwRet;

	// tEXt chunks.
	RpPngWriter::kv_vector kv;
	char mtime_str[32];
	char szFile_str[32];
	GFile *f_src = nullptr;
	gchar *content_type = nullptr;

	// gdk-pixbuf doesn't support CI8, so we'll assume all
	// images are ARGB32. (Well, ABGR32, but close enough.)
	// TODO: Verify channels, etc.?
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(output_file,
		imgSz.width, imgSz.height, rp_image::FORMAT_ARGB32));
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
	kv.emplace_back("Software", "ROM Properties Page shell extension (GTK" GTK_MAJOR_STR ")");

	// Modification time and file size.
	mtime_str[0] = 0;
	szFile_str[0] = 0;
	f_src = g_file_new_for_uri(source_uri);
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
		kv.emplace_back("Thumb::MTime", mtime_str);
	}

	// MIME type.
	// source_uri is only used for the file extension.
	// TODO: Get this directly from the D-Bus call or similar?
	// TODO: Get the ~4K header data read from RomDataFactory and use it here?
	content_type = g_content_type_guess(source_uri, nullptr, 0, nullptr);
	if (content_type) {
		gchar *const mime_type = g_content_type_get_mime_type(content_type);
		if (mime_type) {
			kv.emplace_back("Thumb::Mimetype", mime_type);
			g_free(mime_type);
		}
		g_free(content_type);
	}

	// File size.
	if (szFile_str[0] != 0) {
		kv.emplace_back("Thumb::Size", szFile_str);
	}

	// URI.
	// NOTE: The Thumbnail Management Standard specification says spaces
	// must be urlencoded: ' ' -> "%20"
	// KDE4 and KF5 prior to 5.46 did not do this correctly.
	// KF5 5.46 does encode the URI correctly.
	// References:
	// - https://bugs.kde.org/show_bug.cgi?id=393015
	// - https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
	kv.emplace_back("Thumb::URI", source_uri);

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
	row_pointers.reset(new const uint8_t*[imgSz.height]);
#ifdef RP_GTK_USE_CAIRO
	pixels = cairo_image_surface_get_data(ret_img);
	rowstride = cairo_image_surface_get_stride(ret_img);
#else /* !RP_GTK_USE_CAIRO */
	pixels = gdk_pixbuf_get_pixels(ret_img);
	rowstride = gdk_pixbuf_get_rowstride(ret_img);
#endif
	for (int y = 0; y < imgSz.height; y++, pixels += rowstride) {
		row_pointers[y] = pixels;
	}

	// Write the IDAT section.
#ifdef RP_GTK_USE_CAIRO
	// Cairo uses ARGB32.
	static const bool is_abgr = false;
#else /* !RP_GTK_USE_CAIRO */
	// GdkPixbuf uses ABGR32.
	static const bool is_abgr = true;
#endif
	pwRet = pngWriter->write_IDAT(row_pointers.get(), is_abgr);
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		ret = RPCT_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

cleanup:
	d->freeImgClass(ret_img);
	romData->unref();
	if (free_source_uri) {
		g_free(source_uri);
	}
	if (free_source_filename) {
		g_free(source_filename);
	}
	return ret;
}
