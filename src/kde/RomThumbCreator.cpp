/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde.h"
#include "check-uid.hpp"

#include "RomThumbCreator.hpp"
#include "RpQImageBackend.hpp"
#include "AchQtDBus.hpp"

// librpbase, librptexture
using namespace LibRpBase;
using LibRpTexture::rp_image;

// RpFileKio
#include "RpFile_kio.hpp"

// libromdata
#include "libromdata/RomDataFactory.hpp"
using LibRomData::RomDataFactory;

// TCreateThumbnail is a templated class,
// so we have to #include the .cpp file here.
#include "libromdata/img/TCreateThumbnail.cpp"
using LibRomData::TCreateThumbnail;

// C++ STL classes.
using std::string;
using std::unique_ptr;

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
		// Register RpQImageBackend and AchQtDBus.
		rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);
#if defined(ENABLE_ACHIEVEMENTS) && defined(HAVE_QtDBus_NOTIFY)
		AchQtDBus::instance();
#endif /* ENABLE_ACHIEVEMENTS && HAVE_QtDBus_NOTIFY */

		return new RomThumbCreator();
	}
}

class RomThumbCreatorPrivate : public TCreateThumbnail<QImage>
{
	public:
		RomThumbCreatorPrivate() { }

	private:
		typedef TCreateThumbnail<QImage> super;
		Q_DISABLE_COPY(RomThumbCreatorPrivate)

	public:
		/** TCreateThumbnail functions. **/

		/**
		 * Wrapper function to convert rp_image* to ImgClass.
		 * @param img rp_image
		 * @return ImgClass
		 */
		inline QImage rpImageToImgClass(const rp_image *img) const final
		{
			return rpToQImage(img);
		}

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		inline bool isImgClassValid(const QImage &imgClass) const final
		{
			return !imgClass.isNull();
		}

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		inline QImage getNullImgClass(void) const final
		{
			return QImage();
		}

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		inline void freeImgClass(QImage &imgClass) const final
		{
			// Nothing to do here...
			Q_UNUSED(imgClass)
		}

		/**
		 * Rescale an ImgClass using the specified scaling method.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @param method Scaling method.
		 * @return Rescaled ImgClass.
		 */
		inline QImage rescaleImgClass(const QImage &imgClass, const ImgSize &sz, ScalingMethod method = ScalingMethod::Nearest) const final
		{
			Qt::TransformationMode mode;
			switch (method) {
				case ScalingMethod::Nearest:
					mode = Qt::FastTransformation;
					break;
				case ScalingMethod::Bilinear:
					mode = Qt::SmoothTransformation;
					break;
				default:
					assert(!"Invalid scaling method.");
					mode = Qt::FastTransformation;
					break;
			}

			return imgClass.scaled(sz.width, sz.height, Qt::IgnoreAspectRatio, mode);
		}

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		int getImgClassSize(const QImage &imgClass, ImgSize *pOutSize) const final
		{
			pOutSize->width = imgClass.width();
			pOutSize->height = imgClass.height();
			return 0;
		}

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		string proxyForUrl(const string &url) const final;
};

/** RomThumbCreatorPrivate **/

/**
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
string RomThumbCreatorPrivate::proxyForUrl(const string &url) const
{
	QString proxy = KProtocolManager::proxyForUrl(QUrl(U82Q(url)));
	if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
		// No proxy.
		return string();
	}

	// Proxy is required..
	return Q2U8(proxy);
}

/** RomThumbCreator **/

RomThumbCreator::RomThumbCreator()
	: d_ptr(new RomThumbCreatorPrivate())
{ }

RomThumbCreator::~RomThumbCreator()
{
	delete d_ptr;
}

/**
 * Creates a thumbnail.
 *
 * Note that this method should not do any scaling.  The @p width and @p
 * height parameters are provided as hints for images that are generated
 * from non-image data (like text).
 *
 * @param path    The path of the file to create a preview for.  This is
 *                always a local path.
 * @param width   The requested preview width (see the note on scaling
 *                above).
 * @param height  The requested preview height (see the note on scaling
 *                above).
 * @param img     The QImage to store the preview in.
 *
 * @return @c true if a preview was successfully generated and store in @p
 *         img, @c false otherwise.
 */
bool RomThumbCreator::create(const QString &path, int width, int height, QImage &img)
{
	Q_UNUSED(height);
	if (path.isEmpty()) {
		return false;
	}

	// Attempt to open the ROM file.
	// NOTE: QUrl uses the following special characters as delimiters:
	// - '?': query string
	// - '#': anchor
	// so we need to urlencode it first.
	QString path_enc = path;
#ifndef _WIN32
	path_enc.replace(QChar(L'?'), QLatin1String("%3f"));
#endif /* _WIN32 */
	path_enc.replace(QChar(L'#'), QLatin1String("%23"));
	QUrl path_url(path_enc);

	IRpFile *const file = openQUrl(path_url, true);
	if (!file) {
		return false;
	}

	// Assuming width and height are the same.
	// TODO: What if they aren't?
	Q_D(RomThumbCreator);
	RomThumbCreatorPrivate::GetThumbnailOutParams_t outParams;
	int ret = d->getThumbnail(file, width, &outParams);
	if (ret == 0) {
		img = outParams.retImg;
		// FIXME: KF5 5.91, Dolphin 21.12.1
		// If img.width() * bytespp != img.bytesPerLine(), the image
		// pitch is incorrect. Test image: hi_mark_sq.ktx (145x130)
		// The underlying QImage works perfectly fine, though...
		int bytespp;
		switch (img.format()) {
			// TODO: Other formats?
			case QImage::Format_Indexed8:
				bytespp = 1;
				break;
			case QImage::Format_ARGB32:
			default:
				bytespp = 4;
				break;
		}
		if (img.width() * bytespp != img.bytesPerLine()) {
			img = img.copy();
		}
	}
	file->unref();
	return (ret == 0);
}

/**
 * Returns the flags for this plugin.
 *
 * @return XOR'd flags values.
 * @see Flags
 */
ThumbCreator::Flags RomThumbCreator::flags(void) const
{
	return ThumbCreator::None;
}

/** CreateThumbnail **/

/**
 * Thumbnail creator function for wrapper programs.
 * @param source_file Source file. (UTF-8)
 * @param output_file Output file. (UTF-8)
 * @param maximum_size Maximum size.
 * @return 0 on success; non-zero on error.
 */
extern "C"
Q_DECL_EXPORT int RP_C_API rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.
	CHECK_UID_RET(RPCT_RUNNING_AS_ROOT);

	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

	// Attempt to open the ROM file.
	QUrl localUrl = localizeQUrl(QUrl(QString::fromUtf8(source_file)));
	IRpFile *const file = openQUrl(localUrl, true);
	if (!file) {
		// Could not open the file.
		return RPCT_SOURCE_FILE_ERROR;
	}

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
	RomThumbCreatorPrivate *const d = new RomThumbCreatorPrivate();
	RomThumbCreatorPrivate::GetThumbnailOutParams_t outParams;
	int ret = d->getThumbnail(romData, maximum_size, &outParams);
	delete d;

	if (ret != 0 || outParams.retImg.isNull()) {
		// No image.
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	const int height = outParams.retImg.height();

	/** tEXt chunks. **/
	// NOTE: These are written before IHDR in order to put the
	// tEXt chunks before the IDAT chunk.

	// NOTE: QString::toStdString() uses toAscii() in Qt4.
	// Hence, we'll use toUtf8() manually.

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI
	RpPngWriter::kv_vector kv;

	// Determine the image format.
	rp_image::Format format;
	switch (outParams.retImg.format()) {
		case QImage::Format_Indexed8:
			format = rp_image::Format::CI8;
			break;
		case QImage::Format_ARGB32:
			format = rp_image::Format::ARGB32;
			break;
		default:
			// Unsupported...
			assert(!"Unsupported QImage image format.");
			romData->unref();
			return RPCT_OUTPUT_FILE_FAILED;
	}

	RpPngWriter *pngWriter = new RpPngWriter(output_file,
		outParams.retImg.width(), height, format);
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		delete pngWriter;
		romData->unref();
		return RPCT_OUTPUT_FILE_FAILED;
	}

	// Software
	static const char8_t sw[] = U8("ROM Properties Page shell extension (") U8(RP_KDE_UPPER) U8(")");
	kv.emplace_back("Software", sw);

	// Local filename
	QString qs_source_filename;
	if (localUrl.scheme().isEmpty() || localUrl.isLocalFile()) {
		qs_source_filename = localUrl.toLocalFile();
	}

	// FIXME: Local files only. Figure out how to handle this for remote.
	if (!qs_source_filename.isEmpty()) {
		QFileInfo fi_src(qs_source_filename);

		// Modification time.
		int64_t mtime = fi_src.lastModified().toMSecsSinceEpoch() / 1000;
		if (mtime > 0) {
			// FIXME: U8STRFIX for rp_sprintf()
			char buf[32];
			snprintf(buf, sizeof(buf), "%" PRId64, mtime);
			kv.emplace_back("Thumb::MTime", reinterpret_cast<const char8_t*>(buf));
		}

		// File size.
		off64_t szFile = fi_src.size();
		if (szFile > 0) {
			// FIXME: U8STRFIX for rp_sprintf()
			char buf[32];
			snprintf(buf, sizeof(buf), "%" PRId64, szFile);
			kv.emplace_back("Thumb::Size", reinterpret_cast<const char8_t*>(buf));
		}
	}

	// MIME type
	// FIXME: U8STRFIX
	const char *const mimeType = romData->mimeType();
	if (mimeType) {
		kv.emplace_back("Thumb::Mimetype", reinterpret_cast<const char8_t*>(mimeType));
	}

	// Original image dimensions
	if (outParams.fullSize.width > 0 && outParams.fullSize.height > 0) {
		// FIXME: U8STRFIX
		char imgdim_str[16];
		snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.width);
		kv.emplace_back("Thumb::Image::Width", reinterpret_cast<const char8_t*>(imgdim_str));
		snprintf(imgdim_str, sizeof(imgdim_str), "%d", outParams.fullSize.height);
		kv.emplace_back("Thumb::Image::Height", reinterpret_cast<const char8_t*>(imgdim_str));
	}

	// URI
	// NOTE: KDE desktops don't urlencode spaces or non-ASCII characters.
	// GTK+ desktops *do* urlencode spaces and non-ASCII characters.
	// FIXME: Do we want to store the local URI or the original URI?
	// FIXME: U8STRFIX
	kv.emplace_back("Thumb::URI", reinterpret_cast<const char8_t*>(localUrl.toEncoded().constData()));

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// CI8 palette
	// This will be an empty vector if the image isn't CI8.
	// RpPngWriter will ignore the palette arguments in that case.
	QVector<QRgb> colorTable = outParams.retImg.colorTable();

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	int pwRet = pngWriter->write_IHDR(&outParams.sBIT,
		colorTable.constData(), colorTable.size());
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		delete pngWriter;
		romData->unref();
		return RPCT_OUTPUT_FILE_FAILED;
	}

	/** IDAT chunk. **/

	// Initialize the row pointers
	unique_ptr<const uint8_t*[]> row_pointers(new const uint8_t*[height]);
	const uint8_t *bits = outParams.retImg.bits();
	const int bytesPerLine = outParams.retImg.bytesPerLine();
	for (int y = 0; y < height; y++, bits += bytesPerLine) {
		row_pointers[y] = bits;
	}

	// Write the IDAT section
	pwRet = pngWriter->write_IDAT(row_pointers.get());
	if (pwRet != 0) {
		// Error writing IDAT.
		// TODO: Unlink the PNG image.
		ret = RPCT_OUTPUT_FILE_FAILED;
	}

	delete pngWriter;
	romData->unref();
	return ret;
}
