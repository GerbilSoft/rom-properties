/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KDE5)                        *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2017 by David Korth.                                 *
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
		virtual QImage rpImageToImgClass(const rp_image *img) const override final;

		/**
		 * Wrapper function to check if an ImgClass is valid.
		 * @param imgClass ImgClass
		 * @return True if valid; false if not.
		 */
		virtual bool isImgClassValid(const QImage &imgClass) const override final;

		/**
		 * Wrapper function to get a "null" ImgClass.
		 * @return "Null" ImgClass.
		 */
		virtual QImage getNullImgClass(void) const override final;

		/**
		 * Free an ImgClass object.
		 * @param imgClass ImgClass object.
		 */
		virtual void freeImgClass(QImage &imgClass) const override final;

		/**
		 * Rescale an ImgClass using nearest-neighbor scaling.
		 * @param imgClass ImgClass object.
		 * @param sz New size.
		 * @return Rescaled ImgClass.
		 */
		virtual QImage rescaleImgClass(const QImage &imgClass, const ImgSize &sz) const override final;

		/**
		 * Get the proxy for the specified URL.
		 * @return Proxy, or empty string if no proxy is needed.
		 */
		virtual rp_string proxyForUrl(const rp_string &url) const override final;
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
 * Get the proxy for the specified URL.
 * @return Proxy, or empty string if no proxy is needed.
 */
rp_string RomThumbCreatorPrivate::proxyForUrl(const rp_string &url) const
{
	QString proxy = KProtocolManager::proxyForUrl(QUrl(RP2Q(url)));
	if (proxy.isEmpty() || proxy == QLatin1String("DIRECT")) {
		// No proxy.
		return rp_string();
	}

	// Proxy is required..
	return Q2RP(proxy);
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
	int ret = d->getThumbnail(Q2RP(path), width, img);
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
	RomThumbCreatorPrivate *d = new RomThumbCreatorPrivate();
	QImage ret_img;
	int ret = d->getThumbnail(romData, maximum_size, ret_img);
	delete d;

	if (ret != 0 || ret_img.isNull()) {
		// No image.
		romData->unref();
		return RPCT_SOURCE_FILE_NO_IMAGE;
	}

	// Get values for the XDG thumbnail cache text chunks.
	// KDE uses this order: Software, MTime, Mimetype, Size, URI

	// Software.
	// TODO: KDE uses zTXt here.
	// Qt uses zTXt if the text data is 40 characters or more.
	ret_img.setText(QLatin1String("Software"),
		QString::fromLatin1("ROM Properties Page shell extension (KDE%1)").arg(QT_VERSION >> 16));

	// Modification time.
	const QString qs_source_file = QString::fromUtf8(source_file);
	QFileInfo fi_src(qs_source_file);
	int64_t mtime = fi_src.lastModified().toMSecsSinceEpoch() / 1000;
	if (mtime > 0) {
		ret_img.setText(QLatin1String("Thumb::MTime"),
			QString::number(mtime));
	}

	// MIME type.
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	// Use QMimeDatabase for Qt5.
	QMimeDatabase mimeDatabase;
	QMimeType mimeType = mimeDatabase.mimeTypeForFile(fi_src);
	ret_img.setText(QLatin1String("Thumb::Mimetype"), mimeType.name());
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	// Use KMimeType for Qt4.
	KMimeType::Ptr mimeType = KMimeType::findByPath(qs_source_file, 0, true);
	if (mimeType) {
		ret_img.setText(QLatin1String("Thumb::Mimetype"), mimeType->name());
	}
#endif

	// File size.
	int64_t szFile = fi_src.size();
	if (szFile > 0) {
		ret_img.setText(QLatin1String("Thumb::Size"),
			QString::number(szFile));
	}

	// URI.
	QUrl url = QUrl::fromLocalFile(qs_source_file);
	if (url.isValid() && !url.isEmpty()) {
		ret_img.setText(QLatin1String("Thumb::URI"), url.toString());
	}

	// Save the image.
	ret = RPCT_SUCCESS;
	if (!ret_img.save(QString::fromUtf8(output_file), "png")) {
		// Image save failed.
		ret = RPCT_OUTPUT_FILE_FAILED;
	}

	romData->unref();
	return ret;
}
