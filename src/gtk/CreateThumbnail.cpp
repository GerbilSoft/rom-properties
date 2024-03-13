/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * CreateThumbnail.cpp: Thumbnail creator for wrapper programs.            *
 *                                                                         *
 * Copyright (c) 2017-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "CreateThumbnail.hpp"

#include "ProxyForUrl.hpp"
#include "check-uid.h"

// Other rom-properties libraries
#include "librpfile/FileSystem.hpp"
using namespace LibRpBase;
using namespace LibRpTexture;

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"
using LibRomData::TCreateThumbnail;

// NetworkManager D-Bus interface to determine if the connection is metered.
#include <glib-object.h>
#include "NetworkManager.h"

// C++ STL classes
using std::string;
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
	CreateThumbnailPrivate() = default;

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
	inline PIMGTYPE rpImageToImgClass(const rp_image_const_ptr &img) const final
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
		PIMGTYPE_unref(imgClass);
	}

	/**
	 * Rescale an ImgClass using the specified scaling method.
	 * @param imgClass ImgClass object.
	 * @param sz New size.
	 * @param method Scaling method.
	 * @return Rescaled ImgClass.
	 */
	inline PIMGTYPE rescaleImgClass(const PIMGTYPE &imgClass, ImgSize sz, ScalingMethod method = ScalingMethod::Nearest) const final
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
	 * @param url URL
	 * @return Proxy, or empty string if no proxy is needed.
	 */
	inline string proxyForUrl(const char *url) const final
	{
		return ::proxyForUrl(url);
	}

	/**
	 * Is the system using a metered connection?
	 *
	 * Note that if the system doesn't support identifying if the
	 * connection is metered, it will be assumed that the network
	 * connection is unmetered.
	 *
	 * @return True if metered; false if not.
	 */
	bool isMetered(void) final
	{
		// Connect to the service using gdbus-codegen's generated code.
		Manager *proxy = nullptr;
		GError *error = nullptr;

		proxy = manager_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_NONE,
			"org.freedesktop.NetworkManager",	// bus name
			"/org/freedesktop/NetworkManager",	// object path
			nullptr,				// GCancellable
			&error);				// GError
		if (!proxy) {
			// Unable to connect.
			// Assume unmetered.
			g_error_free(error);
			return false;
		}

		// https://developer-old.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMMetered
		enum NMMetered : uint32_t {
			NM_METERED_UNKNOWN	= 0,
			NM_METERED_YES		= 1,
			NM_METERED_NO		= 2,
			NM_METERED_GUESS_YES	= 3,
			NM_METERED_GUESS_NO	= 4,
		};
		const NMMetered metered = static_cast<NMMetered>(manager_get_metered(proxy));
		g_object_unref(proxy);
		return (metered == NM_METERED_YES || metered == NM_METERED_GUESS_YES);
	}
};

/** CreateThumbnail **/

/**
 * Create a RomData object from a filename or URI.
 * @param source_file	[in] Source filename or URI
 * @param s_uri		[out] Normalized URI (file:/ for a filename, etc.)
 * @param p_err		[out] RPCT error code
 * @return RomDataPtr on success; nullptr on error.
 */
static RomDataPtr openFromFilenameOrURI(const char *source_file, string &s_uri, int *p_err)
{
	// NOTE: Not checking these in Release builds.
	// TODO: Simplify the "is_directory" code.
	assert(source_file != nullptr);
	assert(p_err != nullptr);
	RomDataPtr romData;

	s_uri.clear();
	const bool enableThumbnailOnNetworkFS = Config::instance()->enableThumbnailOnNetworkFS();

	char *const uri_scheme = g_uri_parse_scheme(source_file);
	if (uri_scheme != nullptr) {
		// This is a URI.
		s_uri = source_file;
		g_free(uri_scheme);
		// Check if it's a local filename.
		gchar *const source_filename = g_filename_from_uri(source_file, nullptr, nullptr);
		if (source_filename) {
			// It's a local filename.
			// Check if it's on a "bad" filesystem.
			if (FileSystem::isOnBadFS(source_filename, enableThumbnailOnNetworkFS)) {
				// It's on a "bad" filesystem.
				g_free(source_filename);
				*p_err = RPCT_ERROR_SOURCE_FILE_BAD_FS;
				return nullptr;
			}

			if (likely(!FileSystem::is_directory(source_filename))) {
				// File: Open the file and call RomDataFactory::create() with the opened file.

				// Attempt to open the ROM file.
				const IRpFilePtr file = std::make_shared<RpFile>(source_filename, RpFile::FM_OPEN_READ_GZ);
				if (!file) {
					// Could not open the file.
					if (p_err) {
						*p_err = RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
					}
					return {};
				}

				// Get the appropriate RomData class for this ROM.
				// RomData class *must* support at least one image type.
				romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
			} else {
				// Directory: Call RomDataFactory::create() with the filename.
				romData = RomDataFactory::create(source_filename);
			}
			g_free(source_filename);
		} else {
			// Not a local filename.
			if (!enableThumbnailOnNetworkFS) {
				// Thumbnailing on network file systems is disabled.
				*p_err = RPCT_ERROR_SOURCE_FILE_BAD_FS;
				return nullptr;
			}

			// Open the file using RpFileGio.
			const IRpFilePtr file = std::make_shared<RpFileGio>(source_file);
			if (!file) {
				// Could not open the file.
				if (p_err) {
					*p_err = RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
				}
				return {};
			}

			// Get the appropriate RomData class for this ROM.
			// RomData class *must* support at least one image type.
			romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
		}
	} else {
		// This is a filename.
		// Note that for everything except the URI, we can use relative paths
		// as well as absolute paths, so the absolute path conversion is only
		// needed to get the URI for the thumbnail.

		// Check if it's on a "bad" filesystem.
		if (FileSystem::isOnBadFS(source_file, enableThumbnailOnNetworkFS)) {
			// It's on a "bad" filesystem.
			*p_err = RPCT_ERROR_SOURCE_FILE_BAD_FS;
			return nullptr;
		}

		// Check if we have an absolute or relative path.
		if (g_path_is_absolute(source_file)) {
			// We have an absolute path.
			gchar *const source_uri = g_filename_to_uri(source_file, nullptr, nullptr);
			if (source_uri) {
				s_uri = source_uri;
				g_free(source_uri);
			}
		} else {
			// We have a relative path.
			// Convert the filename to an absolute path.
			GFile *const curdir = g_file_new_for_path(".");
			if (curdir) {
				GFile *const abspath = g_file_resolve_relative_path(curdir, source_file);
				if (abspath) {
					gchar *const source_uri = g_file_get_uri(abspath);
					if (source_uri) {
						s_uri = source_uri;
						g_free(source_uri);
					}
					g_object_unref(abspath);
				}
				g_object_unref(curdir);
			}
		}

		if (likely(!FileSystem::is_directory(source_file))) {
			// File: Open the file and call RomDataFactory::create() with the opened file.

			// Attempt to open the ROM file.
			const IRpFilePtr file = std::make_shared<RpFile>(source_file, RpFile::FM_OPEN_READ_GZ);
			if (!file) {
				// Could not open the file.
				if (p_err) {
					*p_err = RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
				}
				return {};
			}

			// Get the appropriate RomData class for this ROM.
			// RomData class *must* support at least one image type.
			romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
		} else {
			// Directory: Call RomDataFactory::create() with the filename.
			romData = RomDataFactory::create(source_file);
		}
	}

	if (romData && romData->isValid()) {
		// File has been opened successfully.
		*p_err = 0;
		return romData;
	}

	// File was not opened.
	// TODO: Actual error code?
	*p_err = RPCT_ERROR_CANNOT_OPEN_SOURCE_FILE;
	return {};
}

/**
 * Thumbnail creator function for wrapper programs. (v2)
 * @param source_file Source file or URI (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
extern "C"
G_MODULE_EXPORT int RP_C_API rp_create_thumbnail2(
	const char *source_file, const char *output_file, int maximum_size, unsigned int flags)
{
	// Some of this is based on the GNOME Thumbnailer skeleton project.
	// https://github.com/hadess/gnome-thumbnailer-skeleton/blob/master/gnome-thumbnailer-skeleton.c
	CHECK_UID_RET(RPCT_ERROR_RUNNING_AS_ROOT);

	// Validate flags.
	if ((flags & ~RPCT_FLAG_VALID_MASK) != 0) {
		return RPCT_ERROR_INVALID_FLAGS;
	}

	// Make sure glib is initialized.
	// NOTE: This is a no-op as of glib-2.35.1.
#if !GLIB_CHECK_VERSION(2,35,1)
	g_type_init();
#endif

	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.

	// Attempt to open the ROM file.
	string s_uri;
	int ret = -1;
	const RomDataPtr romData = openFromFilenameOrURI(source_file, s_uri, &ret);
	if (!romData || ret != 0) {
		// Error opening the file.
		return ret;
	}

	// Create the thumbnail.
	unique_ptr<CreateThumbnailPrivate> d(new CreateThumbnailPrivate());
	CreateThumbnailPrivate::GetThumbnailOutParams_t outParams;
	ret = d->getThumbnail(romData, maximum_size, &outParams);
	if (ret != 0) {
		// No image.
		return RPCT_ERROR_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	unique_ptr<const uint8_t*[]> row_pointers;
	guchar *pixels;
#ifdef RP_GTK_USE_GDKTEXTURE
	guchar *texdata = nullptr;
#endif /* RP_GTK_USE_GDKTEXTURE */
	int rowstride;
	int pwRet;

	// tEXt chunks
	RpPngWriter::kv_vector kv;
	const bool doXDG = !(flags & RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA);

	// gdk-pixbuf doesn't support CI8, so we'll assume all
	// images are ARGB32. (Well, ABGR32, but close enough.)
	// TODO: Verify channels, etc.?
	unique_ptr<RpPngWriter> pngWriter(new RpPngWriter(output_file,
		outParams.thumbSize.width, outParams.thumbSize.height,
		rp_image::Format::ARGB32));
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		ret = RPCT_ERROR_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

	/** tEXt chunks. **/
	// NOTE: These are written before IHDR in order to put the
	// tEXt chunks before the IDAT chunk.

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI
	kv.reserve(doXDG ? 5 : 1);

	// Software
	kv.emplace_back("Software", "ROM Properties Page shell extension (GTK" GTK_MAJOR_STR ")");

	if (doXDG) {
		// Modification time and file size
		char mtime_str[32];
		char szFile_str[32];
		mtime_str[0] = 0;
		szFile_str[0] = 0;
		GFile *const f_src = g_file_new_for_uri(s_uri.c_str());
		if (f_src) {
			GError *error = nullptr;
			GFileInfo *const fi_src = g_file_query_info(f_src,
				G_FILE_ATTRIBUTE_TIME_MODIFIED "," G_FILE_ATTRIBUTE_STANDARD_SIZE,
				G_FILE_QUERY_INFO_NONE, nullptr, &error);
			if (!error) {
				// Get the modification time.
				const guint64 mtime = g_file_info_get_attribute_uint64(fi_src, G_FILE_ATTRIBUTE_TIME_MODIFIED);
				if (mtime > 0) {
					snprintf(mtime_str, sizeof(mtime_str), "%" PRId64, (int64_t)mtime);
				}

				// Get the file size.
				const gint64 szFile = g_file_info_get_size(fi_src);
				if (szFile > 0) {
					snprintf(szFile_str, sizeof(szFile_str), "%" PRId64, szFile);
				}

				g_object_unref(fi_src);
			} else {
				g_error_free(error);
			}
			g_object_unref(f_src);
		}

		// Modification time
		if (mtime_str[0] != '\0') {
			kv.emplace_back("Thumb::MTime", mtime_str);
		}

		// MIME type
		const char *const mimeType = romData->mimeType();
		if (mimeType) {
			kv.emplace_back("Thumb::Mimetype", mimeType);
		}

		// File size
		if (szFile_str[0] != '\0') {
			kv.emplace_back("Thumb::Size", szFile_str);
		}

		// Original image dimensions
		if (outParams.fullSize.width > 0 && outParams.fullSize.height > 0) {
			char imgdim_str[16];
			snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.width);
			kv.emplace_back("Thumb::Image::Width", imgdim_str);
			snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.height);
			kv.emplace_back("Thumb::Image::Height", imgdim_str);
		}

		// URI
		// NOTE: The Thumbnail Management Standard specification says spaces
		// must be urlencoded: ' ' -> "%20"
		// KDE4 and KF5 prior to 5.46 did not do this correctly.
		// KF5 5.46 does encode the URI correctly.
		// References:
		// - https://bugs.kde.org/show_bug.cgi?id=393015
		// - https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html
		kv.emplace_back("Thumb::URI", s_uri.c_str());
	}

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	pwRet = pngWriter->write_IHDR(&outParams.sBIT);
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		ret = RPCT_ERROR_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

	/** IDAT chunk **/

	// Get the image data.
#if defined(RP_GTK_USE_GDKTEXTURE)
	// GdkTexture doesn't allow direct access to pixels.
	// We'll need to download it to a local memory buffer.
	// FIXME: Downscaling isn't working for GdkTexture yet, so we have to use the full image size.
	//rowstride = outParams.thumbSize.width * sizeof(uint32_t);
	//pixels = static_cast<guchar*>(malloc(rowstride * outParams.thumbSize.height));
	rowstride = outParams.fullSize.width * sizeof(uint32_t);
	texdata = static_cast<guchar*>(g_malloc(rowstride * outParams.fullSize.height));
	// FIXME: Using GdkTextureDownloader to convert to GDK_MEMORY_B8G8R8A8
	// causes a heap overflow. (R8G8B8A8 works, as does B8G8R8A8_PREMULTIPLIED.)
	// TODO: Un-premultiply the texture.
	gdk_texture_download(outParams.retImg, texdata, rowstride);
	pixels = texdata;
#elif defined(RP_GTK_USE_CAIRO)
	pixels = cairo_image_surface_get_data(outParams.retImg);
	rowstride = cairo_image_surface_get_stride(outParams.retImg);
#else /* GdkPixbuf */
	pixels = gdk_pixbuf_get_pixels(outParams.retImg);
	rowstride = gdk_pixbuf_get_rowstride(outParams.retImg);
#endif

	// Initialize the row pointers.
	row_pointers.reset(new const uint8_t*[outParams.thumbSize.height]);
	for (int y = 0; y < outParams.thumbSize.height; y++, pixels += rowstride) {
		row_pointers[y] = pixels;
	}

	// Write the IDAT section.
#if defined(RP_GTK_USE_GDKTEXTURE) || defined(RP_GTK_USE_CAIRO)
	// GdkTexture and Cairo uses ARGB32.
	// FIXME: Need to un-premultiply alpha on Cairo?
	static const bool is_abgr = false;
#else /* !RP_GTK_USE_CAIRO */
	// GdkPixbuf use ABGR32.
	static const bool is_abgr = true;
#endif
	pwRet = pngWriter->write_IDAT(row_pointers.get(), is_abgr);
#ifdef RP_GTK_USE_GDKTEXTURE
	g_free(texdata);
#endif /* RP_GTK_USE_GDKTEXTURE */
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		ret = RPCT_ERROR_OUTPUT_FILE_FAILED;
		goto cleanup;
	}

cleanup:
	d->freeImgClass(outParams.retImg);
	return ret;
}

/**
 * Thumbnail creator function for wrapper programs. (v1)
 * @param source_file Source file (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
extern "C"
G_MODULE_EXPORT int RP_C_API rp_create_thumbnail(
	const char *source_file, const char *output_file, int maximum_size)
{
	// Wrapper function that calls rp_create_thumbnail2()
	// with flags == 0.
	return rp_create_thumbnail2(source_file, output_file, maximum_size, 0);
}
