/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
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

#include "RomThumbCreator.hpp"
#include "RpQt.hpp"
#include "RpQImageBackend.hpp"

// libcachemgr
#include "libcachemgr/CacheManager.hpp"
using LibCacheMgr::CacheManager;

// librpbase
#include "librpbase/RomData.hpp"
#include "librpbase/TextFuncs.hpp"
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

// C includes.
#include <unistd.h>

// C includes. (C++ namespace)
#include <cassert>

// C++ includes.
#include <memory>
#include <string>
using std::string;
using std::unique_ptr;

// Qt includes.
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QtCore/QMimeDatabase>
#else
#include <kmimetype.h>
#endif

// KDE protocol manager.
// Used to find the KDE proxy settings.
#include <kprotocolmanager.h>

// Qt major version.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
# error Needs updating for Qt6.
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
# define QT_MAJOR_STR "5"
#elif QT_VERSION >= QT_VERSION_CHECK(4,0,0)
# define QT_MAJOR_STR "4"
#else
# error Qt is too old.
#endif

/**
 * Factory method.
 * References:
 * - https://api.kde.org/4.x-api/kdelibs-apidocs/kio/html/classThumbCreator.html
 * - https://api.kde.org/frameworks/kio/html/classThumbCreator.html
 */
extern "C" {
	Q_DECL_EXPORT ThumbCreator *new_creator()
	{
		// Register RpQImageBackend.
		// TODO: Static initializer somewhere?
		rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

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
		QImage rpImageToImgClass(const rp_image *img) const final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		bool isImgClassValid(const QImage &imgClass) const final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		QImage getNullImgClass(void) const final;

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		void freeImgClass(QImage &imgClass) const final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		QImage rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const final;

		/**
		 * Get the size of the specified ImgClass.
		 * @param imgClass	[in] ImgClass object.
		 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
		 * @return 0 on success; non-zero on error.
		 */
		int getImgClassSize(const QImage &imgClass, ImgSize *pOutSize) const final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		string proxyForUrl(const string &url) const final;
};

/** RomThumbCreatorPrivate **/

/**
 * Wrapper function to convert rp_image* to ImgClass.
 * @param img rp_image
 * @return ImgClass.
 */
QImage RomThumbCreatorPrivate::rpImageToImgClass(const rp_image *img) const
{
	return rpToQImage(img);
}

/**
 * Wrapper function to check if an ImgClass is valid.
 * @param imgClass ImgClass
 * @return True if valid; false if not.
 */
bool RomThumbCreatorPrivate::isImgClassValid(const QImage &imgClass) const
{
	return !imgClass.isNull();
}

/**
 * Wrapper function to get a "null" ImgClass.
 * @return "Null" ImgClass.
 */
QImage RomThumbCreatorPrivate::getNullImgClass(void) const
{
	return QImage();
}

/**
 * Free an ImgClass object.
 * @param imgClass ImgClass object.
 */
void RomThumbCreatorPrivate::freeImgClass(QImage &imgClass) const
{
	// Nothing to do here...
	Q_UNUSED(imgClass)
}

/**
 * Rescale an ImgClass using nearest-neighbor scaling.
 * @param imgClass ImgClass object.
 * @param sz New size.
 * @return Rescaled ImgClass.
 */
QImage RomThumbCreatorPrivate::rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const
{
	return imgClass.scaled(sz.width, sz.height,
		Qt::KeepAspectRatio, Qt::FastTransformation);
}

/**
 * Get the size of the specified ImgClass.
 * @param imgClass	[in] ImgClass object.
 * @param pOutSize	[out] Pointer to ImgSize to store the image size.
 * @return 0 on success; non-zero on error.
 */
int RomThumbCreatorPrivate::getImgClassSize(const QImage &imgClass, ImgSize *pOutSize) const
{
	pOutSize->width = imgClass.width();
	pOutSize->height = imgClass.height();
	return 0;
}

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

	// Assuming width and height are the same.
	// TODO: What if they aren't?
	Q_D(RomThumbCreator);
	int ret = d->getThumbnail(Q2U8(path), width, img);
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
Q_DECL_EXPORT int rp_create_thumbnail(const char *source_file, const char *output_file, int maximum_size)
{
	// NOTE: TCreateThumbnail() has wrappers for opening the
	// ROM file and getting RomData*, but we're doing it here
	// in order to return better error codes.

	// Register RpQImageBackend.
	// TODO: Static initializer somewhere?
	rp_image::setBackendCreatorFn(RpQImageBackend::creator_fn);

	// Attempt to open the ROM file.
	// TODO: RpQFile wrapper.
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
	RomThumbCreatorPrivate *d = new RomThumbCreatorPrivate();
	QImage ret_img;
	rp_image::sBIT_t sBIT;
	int ret = d->getThumbnail(romData, maximum_size, ret_img, &sBIT);
	delete d;

	if (ret != 0 || ret_img.isNull()) {
		// No image.
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Save the image using RpPngWriter.
	const int height = ret_img.height();

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
	switch (ret_img.format()) {
		case QImage::Format_Indexed8:
			format = rp_image::FORMAT_CI8;
			break;
		case QImage::Format_ARGB32:
			format = rp_image::FORMAT_ARGB32;
			break;
		default:
			// Unsupported...
			assert(!"Unsupported QImage image format.");
			romData->unref();
			return RPCT_OUTPUT_FILE_FAILED;
	}

	RpPngWriter *pngWriter = new RpPngWriter(output_file,
		ret_img.width(), height, format);
	if (!pngWriter->isOpen()) {
		// Could not open the PNG writer.
		delete pngWriter;
		romData->unref();
		return RPCT_OUTPUT_FILE_FAILED;
	}

	// Software.
	static const char sw[] = "ROM Properties Page shell extension (KDE" QT_MAJOR_STR ")";
	kv.push_back(std::make_pair("Software", sw));

	// Modification time.
	const QString qs_source_file = U82Q(source_file);
	QFileInfo fi_src(qs_source_file);
	int64_t mtime = fi_src.lastModified().toMSecsSinceEpoch() / 1000;
	if (mtime > 0) {
		kv.push_back(std::make_pair("Thumb::MTime",
			string(QString::number(mtime).toUtf8().constData())));
	}

	// MIME type.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// Use QMimeDatabase for Qt5.
	QMimeDatabase mimeDatabase;
	QMimeType mimeType = mimeDatabase.mimeTypeForFile(fi_src);
	kv.push_back(std::make_pair("Thumb::Mimetype",
		string(mimeType.name().toUtf8().constData())));
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	// Use KMimeType for Qt4.
	KMimeType::Ptr mimeType = KMimeType::findByPath(qs_source_file, 0, true);
	if (mimeType) {
		kv.push_back(std::make_pair("Thumb::Mimetype",
			string(mimeType->name().toUtf8().constData())));
	}
#endif

	// File size.
	int64_t szFile = fi_src.size();
	if (szFile > 0) {
		kv.push_back(std::make_pair("Thumb::Size",
			string(QString::number(szFile).toUtf8().constData())));
	}

	// URI.
	// NOTE: KDE desktops don't urlencode spaces or non-ASCII characters.
	// GTK+ desktops do urlencode spaces and non-ASCII characters.
	QUrl url = QUrl::fromLocalFile(fi_src.absoluteFilePath());
	if (url.isValid() && !url.isEmpty()) {
		kv.push_back(std::make_pair("Thumb::URI",
			string(url.toString().toUtf8().constData())));
	}

	// Write the tEXt chunks.
	pngWriter->write_tEXt(kv);

	/** IHDR **/

	// CI8 palette.
	// This will be an empty vector if the image isn't CI8.
	// RpPngWriter will ignore the palette arguments in that case.
	QVector<QRgb> colorTable = ret_img.colorTable();

	// If sBIT wasn't found, all fields will be 0.
	// RpPngWriter will ignore sBIT in this case.
	int pwRet = pngWriter->write_IHDR(&sBIT,
		colorTable.constData(), colorTable.size());
	if (pwRet != 0) {
		// Error writing IHDR.
		// TODO: Unlink the PNG image.
		delete pngWriter;
		romData->unref();
		return RPCT_OUTPUT_FILE_FAILED;
	}

	/** IDAT chunk. **/

	// Initialize the row pointers.
	unique_ptr<const uint8_t*[]> row_pointers(new const uint8_t*[height]);
	const uint8_t *bits = ret_img.bits();
	const int bytesPerLine = ret_img.bytesPerLine();
	for (int y = 0; y < height; y++, bits += bytesPerLine) {
		row_pointers[y] = bits;
	}

	// Write the IDAT section.
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
