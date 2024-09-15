/***************************************************************************
 * ROM Properties Page shell extension. (KF6)                              *
 * RomThumbnailCreator.cpp: Thumbnail creator.                             *
 *                                                                         *
 * Copyright (c) 2016-2024 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "config.kde.h"

#include "RomThumbnailCreator.hpp"
#include "RomThumbCreator_p.hpp"

#include "AchQtDBus.hpp"
#include "RpQImageBackend.hpp"
#include "RpQUrl.hpp"

// Other rom-properties libraries
#include "libromdata/RomDataFactory.hpp"
using LibRpBase::Config;
using LibRpBase::RomDataPtr;
using namespace LibRomData;
using namespace LibRpFile;

// C++ STL classes
using std::string;
using std::unique_ptr;

/** RomThumbnailCreator (KF5 5.100 and later) **/

RomThumbnailCreator::RomThumbnailCreator(QObject *parent, const QVariantList &args)
	: super(parent, args)
	, d_ptr(new RomThumbnailCreatorPrivate())
{ }

RomThumbnailCreator::~RomThumbnailCreator()
{
	delete d_ptr;
}

/**
 * Create a thumbnail. (new interface added in KF5 5.100)
 *
 * @param request ThumbnailRequest
 * @return ThumbnailResult
 */
KIO::ThumbnailResult RomThumbnailCreator::create(const KIO::ThumbnailRequest &request)
{
	const QUrl url = request.url();
	if (url.isEmpty()) {
		return KIO::ThumbnailResult::fail();
	}

	RomDataPtr romData;

	// Check if this is a directory.
	const QUrl localUrl = localizeQUrl(url);
	const string s_local_filename = localUrl.toLocalFile().toUtf8().constData();
	if (unlikely(!s_local_filename.empty() && FileSystem::is_directory(s_local_filename.c_str()))) {
		const Config *const config = Config::instance();
		if (!config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages)) {
			// Directory package thumbnailing is disabled.
			return KIO::ThumbnailResult::fail();
		}

		// Directory: Call RomDataFactory::create() with the filename.
		romData = RomDataFactory::create(s_local_filename.c_str());
	} else {
		// File: Open the file and call RomDataFactory::create() with the opened file.

		// Attempt to open the ROM file.
		const IRpFilePtr file(openQUrl(url, true));
		if (!file) {
			return KIO::ThumbnailResult::fail();
		}

		// Get the appropriate RomData class for this ROM.
		// RomData class *must* support at least one image type.
		romData = RomDataFactory::create(file, RomDataFactory::RDA_HAS_THUMBNAIL);
	}

	if (!romData) {
		// Not a supported RomData object.
		return KIO::ThumbnailResult::fail();
	}

	// Assuming width and height are the same.
	// TODO: What if they aren't?
	Q_D(RomThumbnailCreator);
	const int width = request.targetSize().width();
	RomThumbnailCreatorPrivate::GetThumbnailOutParams_t outParams;
	int ret = d->getThumbnail(romData, width, &outParams);

	if (ret != 0) {
		return KIO::ThumbnailResult::fail();
	}

	QImage img = outParams.retImg;
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

	return KIO::ThumbnailResult::pass(img);
}
