/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * RomThumbCreator.cpp: Thumbnail creator.                                 *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde.h"

#include "RomThumbCreator.hpp"
#include "RomThumbCreator_p.hpp"

#include "AchQtDBus.hpp"
#include "RpQImageBackend.hpp"
#include "RpQUrl.hpp"

// Other rom-properties libraries
#include "libromdata/RomDataFactory.hpp"
using LibRpBase::Config;
using LibRpBase::RomDataPtr;
using LibRpTexture::rp_image;
using namespace LibRomData;
using namespace LibRpFile;

// C++ STL classes
using std::string;

/**
 * Factory method for ThumbCreator. (KDE4/KF5 only; dropped in KF6)
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

/** RomThumbCreator (KDE4 and KF5 only) **/

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

	RomDataPtr romData;

	// Check if this is a directory.
	const string s_filename = Q2U8_StdString(path);
	if (likely(!FileSystem::is_directory(s_filename.c_str()))) {
		const Config *const config = Config::instance();
		if (!config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages)) {
			// Directory package thumbnailing is disabled.
			return false;
		}

		// Directory: Call RomDataFactory::create() with the filename.
		romData = RomDataFactory::create(s_filename);
	} else {
		// File: Open the file and call RomDataFactory::create() with the opened file.

		// NOTE: QUrl uses the following special characters as delimiters:
		// - '?': query string
		// - '#': anchor
		// so we need to urlencode it first.
		QString path_enc = path;
#ifndef _WIN32
		path_enc.replace(QChar(L'?'), QLatin1String("%3f"));
#endif /* _WIN32 */
		path_enc.replace(QChar(L'#'), QLatin1String("%23"));
		const QUrl path_url(path_enc);

		// Attempt to open the ROM file.
		const IRpFilePtr file(openQUrl(path_url, true));
		if (!file) {
			return false;
		}

		// Get the appropriate RomData class for this ROM.
		// RomData class *must* support at least one image type.
		romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	}

	if (!romData) {
		// Not a supported RomData object.
		return false;
	}

	// Assuming width and height are the same.
	// TODO: What if they aren't?
	Q_D(RomThumbCreator);
	RomThumbCreatorPrivate::GetThumbnailOutParams_t outParams;
	int ret = d->getThumbnail(romData, width, &outParams);

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
			case QImage::Format_ARGB32_Premultiplied:
			default:
				bytespp = 4;
				break;
		}
		if (img.width() * bytespp != img.bytesPerLine()) {
			img = img.copy();
		}
	}

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
