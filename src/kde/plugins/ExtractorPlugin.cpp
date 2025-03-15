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
#include "libromdata/RomDataFactory.hpp"
using namespace LibRpBase;
using namespace LibRpFile;
using namespace LibRomData;

// C++ STL classes
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

ExtractorPlugin::ExtractorPlugin(QObject *parent)
	: super(parent)
{ }

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
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name), ivalue);
				break;
			}

			case PropertyType::UnsignedInteger: {
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					prop.data.uvalue);
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
					result->add(static_cast<KFileMetaData::Property::Property>(prop_name), U82Q(prop.data.str));
				}
				break;
			}

			case PropertyType::Timestamp: {
				// TODO: Verify timezone handling.
				// NOTE: Some properties might need the full QDateTime.
				// CreationDate seems to work fine with just QDate.
				const QDateTime dateTime = unixTimeToQDateTime(prop.data.timestamp, true);
				result->add(static_cast<KFileMetaData::Property::Property>(prop.name),
					dateTime.date());
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
	const string s_local_filename = localUrl.toLocalFile().toUtf8().constData();
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
	static_assert(static_cast<size_t>(RomData::FileType::Max) == static_cast<size_t>(RomData::FileType::Ticket) + 1, "Update KFileMetaData file types!");
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
