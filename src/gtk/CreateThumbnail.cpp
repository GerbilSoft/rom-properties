/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.cpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.h"

// librpbase, librptexture
using namespace LibRpBase;
using LibRpTexture::rp_image;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"
using LibRomData::TCreateThumbnail;

// C++ STL classes.
using std::string;
using std::u8string;
using std::unique_ptr;

// GTK+ major version.
// We can't simply use GTK_MAJOR_VERSION because
// that has parentheses.
#if GTK_CHECK_VERSION(5,0,0)
#  error Needs updating for GTK5.
#elif GTK_CHECK_VERSION(4,0,0)
#  define GTK_MAJOR_STR "4"
#elif GTK_CHECK_VERSION(3,0,0)
#  define GTK_MAJOR_STR "3"
#elif GTK_CHECK_VERSION(2,0,0)
#  define GTK_MAJOR_STR "2"
#else
#  error GTK+ is too old.
#endif

/** CreateThumbnailPrivate **/

class CreateThumbnailPrivate : public TCreateThumbnail<PIMGTYPE>
{
	public:
		CreateThumbnailPrivate() { }

	private:
		typedef TCreateThumbnail<PIMGTYPE> super;
		RP_DISABLE_COPY(CreateThumbnailPrivate)

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		inline PIMGTYPE rpImageToImgClass(const rp_image *img) const final
		{
			// NOTE: Don't premultiply the image when using Cairo,
			// since the image data is going directly to PNG.
			return rp_image_to_PIMGTYPE(img, false);
		}

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		bool isImgClassValid(const PIMGTYPE &imgClass) const final
		{
			return (imgClass != nullptr);
		}

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		inline PIMGTYPE getNullImgClass(void) const final
		{
			return nullptr;
		}

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		inline void freeImgClass(PIMGTYPE &imgClass) const final
		{
			PIMGTYPE_destroy(imgClass);
		}

		/**
		 * Rescale an ImgClass using the specified scaling method.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @param method Scaling method.
		 * @return Rescaled ImgClass.
		 */
		inline PIMGTYPE rescaleImgClass(const PIMGTYPE &imgClass, const ImgSize &sz, ScalingMethod method = ScalingMethod::Nearest) const final
		{
			return PIMGTYPE_scale(imgClass, sz.width, sz.height, (method == ScalingMethod::Bilinear));
		}

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		inline int getImgClassSize(const PIMGTYPE &imgClass, ImgSize *pOutSize) const final
		{
			return PIMGTYPE_get_size(imgClass, &pOutSize->width, &pOutSize->height);
		}

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		string proxyForUrl(const string &url) const final;
};

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
string CreateThumbnailPrivate::proxyForUrl(const string &url) const
{
	// TODO: Support multiple proxies?
	string ret;

	GProxyResolver *const proxy_resolver = g_proxy_resolver_get_default();
	assert(proxy_resolver != nullptr);
	if (!proxy_resolver) {
		// Default proxy resolver is not available...
		return ret;
	}

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

	if (proxy) {
		ret.assign(proxy);
	}
	g_strfreev(proxies);
	return ret;
}

/** CreateThumbnail **/

/**
 * Open a file from a filename or URI.
 * @param source_file	[in] Source filename or URI.
 * @param pp_file	[out] Opened file.
 * @param s_uri		[out] Normalized URI. (file:/ for a filename, etc.)
 * @return 0 on success; RPCT error code on error.
 */
static int openFromFilenameOrURI(const char8_t *source_file, IRpFile **pp_file, u8string &s_uri)
{
	// NOTE: Not checking these in Release builds.
	assert(source_file != nullptr);
	assert(pp_file != nullptr);

	*pp_file = nullptr;
	s_uri.clear();
	const bool enableThumbnailOnNetworkFS = Config::instance()->enableThumbnailOnNetworkFS();

	IRpFile *file = nullptr;
	char *const uri_scheme = g_uri_parse_scheme(reinterpret_cast<const char*>(source_file));
	if (uri_scheme != nullptr) {
		// This is a URI.
		s_uri = source_file;
		g_free(uri_scheme);
		// Check if it's a local filename.
		gchar *const source_filename = g_filename_from_uri(reinterpret_cast<const char*>(source_file), nullptr, nullptr);
		if (source_filename) {
			// It's a local filename.
			// Check if it's on a "bad" filesystem.
			if (FileSystem::isOnBadFS(source_filename, enableThumbnailOnNetworkFS)) {
				// It's on a "bad" filesystem.
				g_free(source_filename);
				return RPCT_SOURCE_FILE_BAD_FS;
			}

			// Open the file using RpFile.
			file = new RpFile(reinterpret_cast<const char8_t*>(source_filename), RpFile::FM_OPEN_READ_GZ);
			g_free(source_filename);
		} else {
			// Not a local filename.
			if (!enableThumbnailOnNetworkFS) {
				// Thumbnailing on network file systems is disabled.
				return RPCT_SOURCE_FILE_BAD_FS;
			}

			// Open the file using RpFileGio.
			file = new RpFileGio(source_file);
		}
	} else {
		// This is a filename.
		// Note that for everything except the URI, we can use relative paths
		// as well as absolute paths, so the absolute path conversion is only
		// needed to get the URI for the thumbnail.

		// Check if it's on a "bad" filesystem.
		// FIXME: U8STRFIX
		if (FileSystem::isOnBadFS(reinterpret_cast<const char*>(source_file), enableThumbnailOnNetworkFS)) {
			// It's on a "bad" filesystem.
			return RPCT_SOURCE_FILE_BAD_FS;
		}

		// Check fi we have an absolute or relative path.
		if (g_path_is_absolute(reinterpret_cast<const char*>(source_file))) {
			// We have an absolute path.
			gchar *const source_uri = g_filename_to_uri(reinterpret_cast<const char*>(source_file), nullptr, nullptr);
			if (source_uri) {
				s_uri = reinterpret_cast<const char8_t*>(source_uri);
				g_free(source_uri);
			}
		} else {
			// We have a relative path.
			// Convert the filename to an absolute path.
			GFile *curdir = g_file_new_for_path(".");
			if (curdir) {
				GFile *abspath = g_file_resolve_relative_path(curdir, reinterpret_cast<const char*>(source_file));
				if (abspath) {
					gchar *const source_uri = g_file_get_uri(abspath);
					if (source_uri) {
						s_uri = reinterpret_cast<const char8_t*>(source_uri);
						g_free(source_uri);
					}
					g_object_unref(abspath);
				}
				g_object_unref(curdir);
			}
		}

		// Open the file using RpFile.
		file = new RpFile(source_file, RpFile::FM_OPEN_READ_GZ);
	}

	if (file && file->isOpen()) {
		// File has been opened successfully.
		*pp_file = file;
		return 0;
	}

	// File was not opened.
	// TODO: Actual error code?
	UNREF(file);
	return RPCT_SOURCE_FILE_ERROR;
}

/**
 * Thumbnail creator function for wrapper programs.
 *
 * NOTE: This is exported as a C function. Keep it as `const char`.
 * Do not change this to `const char8_t`.
 *
 * @param source_file Source file [UTF-8]
 * @param output_file Output file [UTF-8]
 * @param maximum_size Maximum size
 * @return 0 on success; non-zero on error.
 */
extern "C"
G_MODULE_EXPORT int RP_C_API rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// Some of this is based on the GNOME Thumbnailer skeleton project.
	// https://github.com/hadess/gnome-thumbnailer-skeleton/blob/master/gnome-thumbnailer-skeleton.c
	CHECK_UID_RET(RPCT_RUNNING_AS_ROOT);

	// Make sure glib is initialized.
	// NOTE: This is a no-op as of glib-2.35.1.
#if !GLIB_CHECK_VERSION(2,35,1)
	g_type_init();
#endif

	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.

	// Attempt to open the ROM file.
	IRpFile *file = nullptr;
	u8string s_uri;
	int ret = openFromFilenameOrURI(reinterpret_cast<const char8_t*>(source_file), &file, s_uri);
	if (ret != 0) {
		// Error opening the file.
		return ret;
	}
	assert(file != nullptr);

	// Get the appropriate RomData class for this ROM.
	// RomData class *must* support at least one image type.
	RomData *const romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	file->unref();	// file is ref()'d by RomData.
	if (!romData) {
		// ROM is not supported.
		return RPCT_SOURCE_FILE_NOT_SUPPORTED;
	}

	// Create the thumbnail.
	// TODO: If image is larger than maximum_size, resize down.
	unique_ptr<CreateThumbnailPrivate> d(new CreateThumbnailPrivate());
	CreateThumbnailPrivate::GetThumbnailOutParams_t outParams;
	ret = d->getThumbnail(romData, maximum_size, &outParams);
	if (ret != 0) {
		// No image.
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	unique_ptr<const uint8_t*[]> row_pointers;
	guchar *pixels;
	int rowstride;
	int pwRet;

	// tEXt chunks.
	RpPngWriter::kv_vector kv;
	char mtime_str[32];
	char szFile_str[32];
	GFile *f_src = nullptr;
	const char *mimeType;

	// gdk-pixbuf doesn't support CI8, so we'll assume all
	// images are ARGB32. (Well, ABGR32, but close enough.)
	// TODO: Verify channels, etc.?
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(
		reinterpret_cast<const char8_t*>(output_file),
		outParams.thumbSize.width, outParams.thumbSize.height,
		rp_image::Format::ARGB32));
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

	// Software
	static const char8_t sw[] = U8("ROM Properties Page shell extension (GTK") U8(GTK_MAJOR_STR) U8(")");
	kv.emplace_back("Software", sw);

	// Modification time and file size
	mtime_str[0] = 0;
	szFile_str[0] = 0;
	f_src = g_file_new_for_uri(reinterpret_cast<const char*>(s_uri.c_str()));
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

	// FIXME: U8STRFIX

	// Modification time
	if (mtime_str[0] != 0) {
		kv.emplace_back("Thumb::MTime", reinterpret_cast<const char8_t*>(mtime_str));
	}

	// MIME type
	mimeType = romData->mimeType();
	if (mimeType) {
		kv.emplace_back("Thumb::Mimetype", reinterpret_cast<const char8_t*>(mimeType));
	}

	// File size
	if (szFile_str[0] != 0) {
		kv.emplace_back("Thumb::Size", reinterpret_cast<const char8_t*>(szFile_str));
	}

	// Original image dimensions
	if (outParams.fullSize.width > 0 && outParams.fullSize.height > 0) {
		char imgdim_str[16];
		snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.width);
		kv.emplace_back("Thumb::Image::Width", reinterpret_cast<const char8_t*>(imgdim_str));
		snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.height);
		kv.emplace_back("Thumb::Image::Height", reinterpret_cast<const char8_t*>(imgdim_str));
	}

	// URI
	// NOTE: The Thumbnail Management Standard specification says spaces
	// must be urlencoded: ' ' -> "%20"
	// KDE4 and KF5 prior to 5.46 did not do this correctly.
	// KF5 5.46 does encode the URI correctly.
	// References:
	// - https://bugs.kde.org/show_bug.cgi?id=393015
	// - https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
	kv.emplace_back("Thumb::URI", reinterpret_cast<const char8_t*>(s_uri.c_str()));

	// Write the tEXt chunks
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	pwRet = pngWriter->write_IHDR(&outParams.sBIT);
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		ret = RPCT_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

	/** IDAT chunk. **/

	// Initialize the row pointers
	row_pointers.reset(new const uint8_t*[outParams.thumbSize.height]);
	pixels = PIMGTYPE_get_image_data(outParams.retImg);
	rowstride = PIMGTYPE_get_rowstride(outParams.retImg);
	for (int y = 0; y < outParams.thumbSize.height; y++, pixels += rowstride) {
		row_pointers[y] = pixels;
	}

	// Write the IDAT section
#ifdef RP_GTK_USE_CAIRO
	// Cairo uses ARGB32
	static const bool is_abgr = false;
#else /* !RP_GTK_USE_CAIRO */
	// GdkPixbuf uses ABGR32
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
	d->freeImgClass(outParams.retImg);
	romData->unref();
	return ret;
}
