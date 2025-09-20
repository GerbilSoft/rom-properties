/***************************************************************************
 * ROM Properties Page shell extension. (KDE)                              *
 * ExtractorPlugin.cpp: KFileMetaData extractor plugin.                    *
 *                                                                         *
 * NOTE: This file is compiled as a separate .so file. Originally, a       *
 * forwarder plugin was used, since Qt's plugin system prevents a single   *
 * shared library from exporting multiple plugins, but as of RP 2.0,       *
 * most of the important code is split out into libromdata.so, so the      *
 * forwarder version is unnecessary.                                       *
 *                                                                         *
 * Copyright (c) 2018-2025 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "check-uid.hpp"

#include <QtCore/qglobal.h>
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
#  error Update for new Qt!
#elif QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  include "ExtractorPluginKF6.hpp"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#  include "ExtractorPluginKF5.hpp"
#else
#  error Qt is too old!
#endif

#include "RpQUrl.hpp"

// Other rom-properties libraries
#include "librpbase/RomMetaData.hpp"
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
using std::array;
using std::string;
using std::vector;

// KDE includes
// NOTE: kfilemetadata_version.h was added in 5.94.0, so we can't use it.
// Using kcoreaddons_version.h instead.
#include <kfileitem.h>
//#include <kfilemetadata_version.h>
#include <kcoreaddons_version.h>
#include <kfilemetadata/extractorplugin.h>
#include <kfilemetadata/properties.h>
using KFileMetaData::ExtractionResult;
using namespace KFileMetaData::Property;

namespace RomPropertiesKDE {

// Mqpping of LibRpBase::Property to KFileMetaData::Property.

// LibRpBase::Property used to be identical to LibRpBase::Property,
// but a lot of the properties don't make sense for rom-properties, and
// we want to be able to add custom properties for certain systems, too.

// - Index: RomMetaData::Property
// - Value: KFileMetaData::Property (as uint8_t)
static array<uint8_t, static_cast<size_t>(LibRpBase::Property::PropertyCount)> kfmd_PropIdxMap = {{
	KFileMetaData::Property::Empty,

	// Audio
	KFileMetaData::Property::BitRate,		// integer: kbit/sec
	KFileMetaData::Property::Channels,		// integer: channels
	KFileMetaData::Property::Duration,		// integer: duration, in milliseconds
	KFileMetaData::Property::Genre,			// string
	KFileMetaData::Property::SampleRate,		// integer: Hz
	KFileMetaData::Property::TrackNumber,		// unsigned integer: track number
	KFileMetaData::Property::ReleaseYear,		// unsigned integer: year
	KFileMetaData::Property::Comment,		// string: comment
	KFileMetaData::Property::Artist,		// string: artist
	KFileMetaData::Property::Album,			// string: album
	KFileMetaData::Property::AlbumArtist,		// string: album artist
	KFileMetaData::Property::Composer,		// string: composer
	KFileMetaData::Property::Lyricist,		// string: lyricist

	// Document
	KFileMetaData::Property::Author,		// string: author
	KFileMetaData::Property::Title,			// string: title
	KFileMetaData::Property::Subject,		// string: subject
	KFileMetaData::Property::Generator,		// string: application used to create this file
	KFileMetaData::Property::PageCount,		// integer: page count
	KFileMetaData::Property::WordCount,		// integer: word count
	KFileMetaData::Property::LineCount,		// integer: line count
	KFileMetaData::Property::Language,		// string: language
	KFileMetaData::Property::Copyright,		// string: copyright
	KFileMetaData::Property::Publisher,		// string: publisher
	KFileMetaData::Property::CreationDate,		// timestamp: creation date
	KFileMetaData::Property::Keywords,		// FIXME: What's the type?

	// Media
	KFileMetaData::Property::Width,			// integer: width, in pixels
	KFileMetaData::Property::Height,		// integer: height, in pixels
	KFileMetaData::Property::AspectRatio,		// FIXME: Float?
	KFileMetaData::Property::FrameRate,		// integer: number of frames per second

	// Images
	KFileMetaData::Property::Manufacturer,		// string
	KFileMetaData::Property::Model,			// string
	KFileMetaData::Property::ImageDateTime,		// FIXME
	KFileMetaData::Property::ImageOrientation,	// FIXME
	KFileMetaData::Property::PhotoFlash,		// FIXME

	// Origin
	KFileMetaData::Property::OriginUrl,		// string: origin URL
	KFileMetaData::Property::OriginEmailSubject,	// string: subject of origin email
	KFileMetaData::Property::OriginEmailSender,	// string: sender of origin email
	KFileMetaData::Property::OriginEmailMessageId,	// string: message ID of origin email

	// Audio
	KFileMetaData::Property::DiscNumber,		// integer: disc number of multi-disc set
	KFileMetaData::Property::Location,		// string: location where audio was recorded
	KFileMetaData::Property::Performer,		// string: (lead) performer
	KFileMetaData::Property::Ensemble,		// string: ensemble
	KFileMetaData::Property::Arranger,		// string: arranger
	KFileMetaData::Property::Conductor,		// string: conductor
	KFileMetaData::Property::Opus,			// string: opus

	// Other
	KFileMetaData::Property::Label,			// string: label
	KFileMetaData::Property::Compilation,		// string: compilation
	KFileMetaData::Property::License,		// string: license information

	// Added in KF5 5.48
	KFileMetaData::Property::Rating,		// integer: [0,100]
	KFileMetaData::Property::Lyrics,		// string

	// Added in KF5 5.53
	KFileMetaData::Property::Description,		// string
}};

ExtractorPlugin::ExtractorPlugin(QObject *parent)
	: super(parent)
{
	assert(kfmd_PropIdxMap[kfmd_PropIdxMap.size()-1] != 0);
}

QStringList ExtractorPlugin::mimetypes(void) const
{
	// Get the MIME types from RomDataFactory.
	const vector<const char*> &vec_mimeTypes = RomDataFactory::supportedMimeTypes();

	// Convert to QStringList.
	QStringList mimeTypes;
	mimeTypes.reserve(static_cast<int>(vec_mimeTypes.size()));
	for (const char *mimeType : vec_mimeTypes) {
		mimeTypes += QLatin1String(mimeType);
	}
	return mimeTypes;
}

void ExtractorPlugin::extract_properties(KFileMetaData::ExtractionResult *result, RomData *romData)
{
	const RomMetaData *const metaData = romData->metaData();
	if (!metaData || metaData->empty()) {
		// No metadata properties.
		return;
	}

	// Process the metadata.
	for (const RomMetaData::MetaData &prop : *metaData) {
		// RomMetaData's property indexes match KFileMetaData.
		// No conversion is necessary.
		switch (prop.type) {
			case PropertyType::Integer: {
				int ivalue = prop.data.ivalue;
				switch (prop.name) {
					case LibRpBase::Property::Duration:
						// rom-properties: milliseconds
						// KFileMetaData: seconds
						ivalue /= 1000;
						break;
					case LibRpBase::Property::Rating:
						// rom-properties: [0,100]
						// KFileMetaData: [0,10]
						ivalue /= 10;
						break;
					default:
						break;
				}
				result->add(static_cast<KFileMetaData::Property::Property>(
					kfmd_PropIdxMap[static_cast<size_t>(prop.name)]), ivalue);
				break;
			}

			case PropertyType::UnsignedInteger: {
				result->add(static_cast<KFileMetaData::Property::Property>(
					kfmd_PropIdxMap[static_cast<size_t>(prop.name)]), prop.data.uvalue);
				break;
			}

			case PropertyType::String: {
				LibRpBase::Property prop_name = prop.name;
				// NOTE: kfilemetadata_version.h was added in KF5 5.94.0.
				// Using kcoreaddons_version.h instead.
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(5, 53, 0)
				if (prop_name == LibRpBase::Property::Description) {
					// KF5 5.53 added Description.
					// Fall back to Subject since Description isn't available.
					prop_name = LibRpBase::Property::Subject;
				}
#endif /* KCOREADDONS_VERSION < QT_VERSION_CHECK(5, 53, 0) */

				if (prop.data.str && prop.data.str[0] != '\0') {
					result->add(static_cast<KFileMetaData::Property::Property>(
					kfmd_PropIdxMap[static_cast<size_t>(prop_name)]), U82Q(prop.data.str));
				}
				break;
			}

			case PropertyType::Timestamp: {
				// TODO: Verify timezone handling.
				// NOTE: Some properties might need the full QDateTime.
				// CreationDate seems to work fine with just QDate.
				const QDateTime dateTime = unixTimeToQDateTime(prop.data.timestamp, true);
				result->add(static_cast<KFileMetaData::Property::Property>(
					kfmd_PropIdxMap[static_cast<size_t>(prop.name)]), dateTime.date());
				break;
			}

			case PropertyType::Double: {
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					prop.data.dvalue);
				break;
			}

			default:
				// ERROR!
				assert(!"Unsupported RomMetaData PropertyType.");
				break;
		}
	}
}

#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0)
void ExtractorPlugin::extract_image(KFileMetaData::ExtractionResult *result, RomData *romData)
{
	// TODO: Get external images. For now, only using internal images.
	// Image data should be a PNG or JPEG file.

	// File Icon (IMG_INT_ICON)
	// - If not available: IMG_INT_BANNER

	// Front Cover (IMG_EXT_COVER)
	// Media (IMG_EXT_MEDIA)

	// For "Other", use the following, if available:
	// - IMG_INT_IMAGE
	// - IMG_EXT_TITLE_SCREEN

	/** TODO: Implement this. Placeholder for now... **/
	Q_UNUSED(result)
	Q_UNUSED(romData)
}
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0) */

void ExtractorPlugin::extract(ExtractionResult *result)
{
	const ExtractionResult::Flags flags = result->inputFlags();
	if (unlikely(flags == ExtractionResult::ExtractNothing)) {
		// Nothing to extract...
		return;
	}

	// Which attributes are required?
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0)
	static constexpr unsigned int mask = ExtractionResult::ExtractMetaData | ExtractionResult::ExtractImageData;
#else /* KCOREADDONS_VERSION < QT_VERSION_CHECK(5, 76, 0) */
	static constexpr unsigned int mask = ExtractionResult::ExtractMetaData;
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0) */
	unsigned int attrs;
	switch (flags & mask) {
		case ExtractionResult::ExtractMetaData:
			// Only extract metadata.
			attrs = RomDataFactory::RDA_HAS_METADATA;
			break;
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0)
		case ExtractionResult::ExtractImageData:
			// Only extract images.
			attrs = RomDataFactory::RDA_HAS_THUMBNAIL;
			break;
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0) */
		default:
			// Multiple things to extract.
			attrs = 0;
			break;
	}

	const QUrl inputUrl(QUrl(result->inputUrl()));
	RomDataPtr romData;

	// Check if this is a directory.
	const QUrl localUrl = localizeQUrl(inputUrl);
	const string s_local_filename = Q2U8_StdString(localUrl.toLocalFile());
	if (unlikely(!s_local_filename.empty() && FileSystem::is_directory(s_local_filename))) {
		const Config *const config = Config::instance();
		if (!config->getBoolConfigOption(Config::BoolConfig::Options_ThumbnailDirectoryPackages)) {
			// Directory package thumbnailing is disabled.
			return;
		}

		// Directory: Call RomDataFactory::create() with the filename.
		romData = RomDataFactory::create(s_local_filename);
	} else {
		// File: Open the file and call RomDataFactory::create() with the opened file.
		IRpFilePtr file(openQUrl(localUrl, false));
		if (!file) {
			// Could not open the file.
			return;
		}

		// Get the appropriate RomData class for this ROM.
		romData = RomDataFactory::create(file, attrs);
	}

	if (!romData) {
		// ROM is not supported.
		return;
	}

	// File type
	// NOTE: KFileMetaData has a limited set of file types as of v5.107.
	static_assert(static_cast<size_t>(RomData::FileType::Max) == static_cast<size_t>(RomData::FileType::ConfigurationFile) + 1, "Update KFileMetaData file types!");
	switch (romData->fileType()) {
		default:
			// No KFileMetaData::Type is applicable here.
			break;

		case RomData::FileType::IconFile:
		case RomData::FileType::BannerFile:
		case RomData::FileType::TextureFile:
			result->addType(KFileMetaData::Type::Image);
			break;

		case RomData::FileType::ContainerFile:
		case RomData::FileType::Bundle:
			result->addType(KFileMetaData::Type::Archive);
			break;

		case RomData::FileType::AudioFile:
			result->addType(KFileMetaData::Type::Audio);
			break;
	}

	// Metadata properties
	if (flags & ExtractionResult::ExtractMetaData) {
		extract_properties(result, romData.get());
	}

#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0)
	// KFileMetaData 5.76.0 added images.
	if (flags & ExtractionResult::ExtractImageData) {
		extract_image(result, romData.get());
	}
#endif /* KCOREADDONS_VERSION >= QT_VERSION_CHECK(5, 76, 0) */

	// Finished extracting metadata.
}

} // namespace RomPropertiesKDE
